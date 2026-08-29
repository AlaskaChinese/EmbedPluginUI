#include "linux_i2c.hpp"
#include "rpi_pages.hpp"
#include "system_monitor.hpp"
#include "terminal_feed.hpp"
#include "epui/canvas.hpp"
#include "epui/input_plugin.hpp"
#include "epui/oled.hpp"
#include "epui/oled_display_plugin.hpp"
#include "epui/page.hpp"
#include "epui/plugin_registry.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>

using namespace epui;
using namespace openoledui::rpi;

namespace {
std::uint32_t now_ms(){return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}

class TerminalInputPlugin final : public InputPlugin {
public:
    const char* name() const override { return "terminal-keyboard"; }
    bool start() override {
        if (!::isatty(STDIN_FILENO) || ::tcgetattr(STDIN_FILENO, &old_) < 0) return true;
        termios t = old_;
        t.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
        if (::tcsetattr(STDIN_FILENO, TCSANOW, &t) != 0) return false;
        old_flags_ = ::fcntl(STDIN_FILENO, F_GETFL, 0);
        ::fcntl(STDIN_FILENO, F_SETFL, old_flags_ | O_NONBLOCK);
        active_ = true;
        return true;
    }
    void stop() override {
        if (!active_) return;
        ::tcsetattr(STDIN_FILENO, TCSANOW, &old_);
        ::fcntl(STDIN_FILENO, F_SETFL, old_flags_);
        active_ = false;
    }
    bool poll(InputEvent& event) override {
        unsigned char c{};
        if (::read(STDIN_FILENO, &c, 1) != 1) return false;
        if (c == 'd' || c == 'l') event.key = Key::Next;
        else if (c == 'a' || c == 'h') event.key = Key::Prev;
        else if (c == '\n' || c == ' ') event.key = Key::Select;
        else if (c == 27) {
            unsigned char seq[2]{};
            if (::read(STDIN_FILENO, seq, 2) == 2 && seq[0] == '[' && seq[1] == 'C') event.key = Key::Next;
            else if (seq[0] == '[' && seq[1] == 'D') event.key = Key::Prev;
            else event.key = Key::Back;
        } else {
            last_char_ = c;
            return false;
        }
        event.pressed = true;
        return true;
    }
    bool quit_requested() const { return last_char_ == 'q'; }
private:
    termios old_{};
    int old_flags_{0};
    bool active_{false};
    unsigned char last_char_{0};
};
}

int main(int argc,char**argv){
    const char* dev=argc>1?argv[1]:"/dev/i2c-1";
    const int address=argc>2?std::strtol(argv[2],nullptr,0):0x3C;
    const bool sh1106=argc>3&&std::string(argv[3])=="sh1106";
    LinuxI2cTransport bus(dev,static_cast<std::uint8_t>(address));
    if(!bus.open_bus()){std::perror("Open I2C");return 2;}
    Oled128x64 oled(bus,sh1106?OledController::SH1106:OledController::SSD1306);
    OledDisplayPlugin display(oled, sh1106 ? "sh1106-display" : "ssd1306-display");
    TerminalInputPlugin input;
    PluginRegistry plugins;
    if (!plugins.add(display) || !plugins.add(input) || !plugins.start_all()) {
        std::fprintf(stderr,"Plugin startup failed\n");
        return 3;
    }

    Canvas canvas;Ui ui;SystemMonitor monitor;StatusSnapshot status=monitor.sample();TerminalFeed terminal;terminal.open_feed();
    OverviewPage p1(status);NetworkPage p2(status);PowerPage p3(status);SystemPage p4(status);TerminalPage p5(terminal);
    ui.add_page(p1);ui.add_page(p2);ui.add_page(p3);ui.add_page(p4);ui.add_page(p5);
    std::uint32_t last=0;
    std::fprintf(stderr,"EmbedPluginUI Pi5 running. a/left=prev, d/right=next, q=quit. FIFO: %s\n",terminal.path().c_str());
    while(!input.quit_requested()){
        const std::uint32_t now=now_ms();
        if(now-last>=1000){status=monitor.sample();last=now;}
        terminal.poll();
        InputEvent event{};
        while(input.poll(event)){if(event.pressed)ui.handle(event.key,now);}
        ui.render(canvas,now);
        if(!display.present(canvas)){std::fprintf(stderr,"OLED write failed\n");break;}
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    plugins.stop_all();
    return 0;
}
