#include "evdev_input.hpp"
#include <cerrno>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

namespace epui::rpi {

bool EvdevInputPlugin::start() {
    if (fd_ >= 0) return true;
    fd_ = ::open(device_.c_str(), O_RDONLY | O_NONBLOCK);
    return fd_ >= 0;
}

void EvdevInputPlugin::stop() {
    if (fd_ >= 0) ::close(fd_);
    fd_ = -1;
    shift_ = false;
    control_ = false;
    caps_lock_ = false;
}

bool EvdevInputPlugin::poll(epui::InputEvent& out) {
    if (fd_ < 0) return false;
    input_event raw{};
    while (true) {
        const ssize_t size = ::read(fd_, &raw, sizeof(raw));
        if (size != static_cast<ssize_t>(sizeof(raw))) {
            if (size < 0 && errno == EINTR) continue;
            return false;
        }
        if (raw.type != EV_KEY) continue;

        const bool down = raw.value != 0;
        if (raw.code == KEY_LEFTSHIFT || raw.code == KEY_RIGHTSHIFT) {
            shift_ = down;
            continue;
        }
        if (raw.code == KEY_LEFTCTRL || raw.code == KEY_RIGHTCTRL) {
            control_ = down;
            continue;
        }
        if (raw.code == KEY_CAPSLOCK) {
            if (raw.value == 1) caps_lock_ = !caps_lock_;
            continue;
        }
        if (!down) continue;

        out = epui::InputEvent{};
        if (raw.code == KEY_RIGHT || raw.code == KEY_PAGEDOWN) {
            out.key = epui::Key::Right;
            return true;
        }
        if (raw.code == KEY_LEFT || raw.code == KEY_PAGEUP) {
            out.key = epui::Key::Left;
            return true;
        }
        if (control_ && (raw.code == KEY_UP || raw.code == KEY_DOWN)) {
            out.key = raw.code == KEY_UP ? epui::Key::ScrollUp : epui::Key::ScrollDown;
            return true;
        }
        if (raw.code == KEY_UP || raw.code == KEY_DOWN) {
            out.key = raw.code == KEY_UP ? epui::Key::Up : epui::Key::Down;
            return true;
        }
        if (raw.code == KEY_ENTER || raw.code == KEY_KPENTER) {
            out.key = epui::Key::Select;
            return true;
        }
        if (raw.code == KEY_ESC) {
            out.key = epui::Key::Back;
            return true;
        }

        out.ch = key_to_char(raw.code);
        if (out.ch != 0) return true;
    }
}

char EvdevInputPlugin::key_to_char(unsigned int code) const {
    char ch = 0;
    switch (code) {
    case KEY_A: ch = 'a'; break; case KEY_B: ch = 'b'; break;
    case KEY_C: ch = 'c'; break; case KEY_D: ch = 'd'; break;
    case KEY_E: ch = 'e'; break; case KEY_F: ch = 'f'; break;
    case KEY_G: ch = 'g'; break; case KEY_H: ch = 'h'; break;
    case KEY_I: ch = 'i'; break; case KEY_J: ch = 'j'; break;
    case KEY_K: ch = 'k'; break; case KEY_L: ch = 'l'; break;
    case KEY_M: ch = 'm'; break; case KEY_N: ch = 'n'; break;
    case KEY_O: ch = 'o'; break; case KEY_P: ch = 'p'; break;
    case KEY_Q: ch = 'q'; break; case KEY_R: ch = 'r'; break;
    case KEY_S: ch = 's'; break; case KEY_T: ch = 't'; break;
    case KEY_U: ch = 'u'; break; case KEY_V: ch = 'v'; break;
    case KEY_W: ch = 'w'; break; case KEY_X: ch = 'x'; break;
    case KEY_Y: ch = 'y'; break; case KEY_Z: ch = 'z'; break;
    case KEY_1: ch = shift_ ? '!' : '1'; break;
    case KEY_2: ch = shift_ ? '@' : '2'; break;
    case KEY_3: ch = shift_ ? '#' : '3'; break;
    case KEY_4: ch = shift_ ? '$' : '4'; break;
    case KEY_5: ch = shift_ ? '%' : '5'; break;
    case KEY_6: ch = shift_ ? '^' : '6'; break;
    case KEY_7: ch = shift_ ? '&' : '7'; break;
    case KEY_8: ch = shift_ ? '*' : '8'; break;
    case KEY_9: ch = shift_ ? '(' : '9'; break;
    case KEY_0: ch = shift_ ? ')' : '0'; break;
    case KEY_MINUS: ch = shift_ ? '_' : '-'; break;
    case KEY_EQUAL: ch = shift_ ? '+' : '='; break;
    case KEY_LEFTBRACE: ch = shift_ ? '{' : '['; break;
    case KEY_RIGHTBRACE: ch = shift_ ? '}' : ']'; break;
    case KEY_BACKSLASH: ch = shift_ ? '|' : '\\'; break;
    case KEY_SEMICOLON: ch = shift_ ? ':' : ';'; break;
    case KEY_APOSTROPHE: ch = shift_ ? '"' : '\''; break;
    case KEY_GRAVE: ch = shift_ ? '~' : '`'; break;
    case KEY_COMMA: ch = shift_ ? '<' : ','; break;
    case KEY_DOT: ch = shift_ ? '>' : '.'; break;
    case KEY_SLASH: ch = shift_ ? '?' : '/'; break;
    case KEY_SPACE: return ' ';
    case KEY_TAB: return '\t';
    case KEY_BACKSPACE: return '\b';
    default: return 0;
    }

    if (ch >= 'a' && ch <= 'z') {
        if (control_) return static_cast<char>(ch - 'a' + 1);
        if (shift_ != caps_lock_) ch = static_cast<char>(ch - 'a' + 'A');
    }
    return ch;
}

} // namespace epui::rpi
