#include "linux_i2c.hpp"
#include "rpi_pages.hpp"
#include "system_monitor.hpp"
#include "terminal_feed.hpp"
#include "openoledui/canvas.hpp"
#include "openoledui/oled.hpp"
#include "openoledui/page.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>
using namespace openoledui;using namespace openoledui::rpi;namespace {std::uint32_t now_ms(){return static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}class RawTerminal{public:RawTerminal(){if(!::isatty(STDIN_FILENO)||::tcgetattr(STDIN_FILENO,&old_)<0)return;termios t=old_;t.c_lflag&=static_cast<tcflag_t>(~(ICANON|ECHO));t.c_cc[VMIN]=0;t.c_cc[VTIME]=0;if(::tcsetattr(STDIN_FILENO,TCSANOW,&t)==0){old_flags_=::fcntl(STDIN_FILENO,F_GETFL,0);::fcntl(STDIN_FILENO,F_SETFL,old_flags_|O_NONBLOCK);active_=true;}}~RawTerminal(){if(active_){::tcsetattr(STDIN_FILENO,TCSANOW,&old_);::fcntl(STDIN_FILENO,F_SETFL,old_flags_);}}int read_key(){unsigned char c;return ::read(STDIN_FILENO,&c,1)==1?c:-1;}private:termios old_{};int old_flags_{0};bool active_{false};};}
int main(int argc,char**argv){const char*dev=argc>1?argv[1]:"/dev/i2c-1";const int address=argc>2?std::strtol(argv[2],nullptr,0):0x3C;const bool sh1106=argc>3&&std::string(argv[3])=="sh1106";LinuxI2cTransport bus(dev,static_cast<std::uint8_t>(address));if(!bus.open_bus()){std::perror("Open I2C");return 2;}Oled128x64 oled(bus,sh1106?OledController::SH1106:OledController::SSD1306);if(!oled.init()){std::fprintf(stderr,"OLED init failed\n");return 3;}Canvas canvas;Ui ui;SystemMonitor monitor;StatusSnapshot status=monitor.sample();TerminalFeed terminal;terminal.open_feed();OverviewPage p1(status);NetworkPage p2(status);PowerPage p3(status);SystemPage p4(status);TerminalPage p5(terminal);ui.add_page(p1);ui.add_page(p2);ui.add_page(p3);ui.add_page(p4);ui.add_page(p5);RawTerminal keys;std::uint32_t last=0;std::fprintf(stderr,"OpenOledUI Pi5 running. a/left=prev, d/right=next, q=quit. FIFO: %s\n",terminal.path().c_str());while(true){const std::uint32_t now=now_ms();if(now-last>=1000){status=monitor.sample();last=now;}terminal.poll();int k=keys.read_key();if(k=='q')break;if(k=='d'||k=='l')ui.handle(Key::Next,now);else if(k=='a'||k=='h')ui.handle(Key::Prev,now);else if(k=='\n'||k==' ')ui.handle(Key::Select,now);else if(k==27){int a=keys.read_key(),b=keys.read_key();if(a=='['&&b=='C')ui.handle(Key::Next,now);else if(a=='['&&b=='D')ui.handle(Key::Prev,now);}ui.render(canvas,now);if(!oled.present(canvas.data(),Canvas::BufferSize)){std::fprintf(stderr,"OLED write failed\n");break;}std::this_thread::sleep_for(std::chrono::milliseconds(33));}oled.power(false);return 0;}
