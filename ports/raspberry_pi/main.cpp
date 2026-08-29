#include "linux_i2c.hpp"
#include "rpi_pages.hpp"
#include "rpi_plugins.hpp"
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
using namespace epui::rpi;

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

    Canvas canvas;
    Ui ui;
    Oled128x64 oled(bus,sh1106?OledController::SH1106:OledController::SSD1306);
    OledDisplayPlugin display(oled, sh1106 ? "sh1106-display" : "ssd1306-display");
    TerminalInputPlugin input;
    SystemMonitorPlugin system;
    TerminalFeedPlugin terminal;
    OverviewPage p1(ui, system);
    NetworkPage p2(ui, system);
    PowerPage p3(ui, system);
    SystemPage p4(ui, system);
    TerminalPage p5(ui, terminal);

    PluginRegistry plugins;
    if (!plugins.add(display) || !plugins.add(input) ||
        !plugins.add(p1) || !plugins.add(p2) || !plugins.add(p3) || !plugins.add(p4) || !plugins.add(p5) ||
        !plugins.add(system) || !plugins.add(terminal) || !plugins.start_all()) {
        std::fprintf(stderr,"Plugin startup failed\n");
        return 3;
    }

    std::fprintf(stderr,"EmbedPluginUI Pi5 running. plugins=%zu, a/left=prev, d/right=next, q=quit. FIFO: %s\n",
                 plugins.size(), terminal.feed().path().c_str());

    while(!input.quit_requested()){
        const std::uint32_t now=now_ms();
        plugins.tick_all(now);

        InputEvent event{};
        while(input.poll(event)) {
            if(event.pressed) ui.handle(event.key,now);
        }

        ui.render(canvas,now);
        if(!display.present(canvas)){
            std::fprintf(stderr,"OLED write failed\n");
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    plugins.stop_all();
    return 0;
}
