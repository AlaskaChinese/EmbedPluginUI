#include "terminal_page.hpp"
#include "epui/widgets.hpp"

namespace epui::demo {

TerminalDemoPage::TerminalDemoPage() {
    const char* intro = "Terminal demo\nEnter to focus\n$ ";
    view_.feed(intro, 31);
    view_.set_cursor_visible(false);
}

void TerminalDemoPage::draw(Canvas& canvas, std::uint32_t now_ms) {
    draw_header(canvas, focused_ ? "Terminal *" : "Terminal", 5, 5);
    view_.draw(canvas, 1, 15, 126, 42, now_ms);
}

bool TerminalDemoPage::captures_key(Key key) const {
    if (key == Key::Select) return true;
    return focused_ && (key == Key::Next || key == Key::Prev || key == Key::Back
                        || key == Key::ScrollUp || key == Key::ScrollDown);
}

void TerminalDemoPage::on_key(Key key) {
    if (!focused_) {
        if (key == Key::Select) {
            focused_ = true;
            view_.set_cursor_visible(true);
        }
        return;
    }
    if (key == Key::Back) {
        focused_ = false;
        view_.set_cursor_visible(false);
    } else if (key == Key::Select) {
        const char* prompt = "\n$ ";
        view_.feed(prompt, 3);
    } else if (key == Key::Prev) {
        view_.scroll(1);
    } else if (key == Key::Next) {
        view_.scroll(-1);
    } else if (key == Key::ScrollUp) {
        view_.scroll(6);
    } else if (key == Key::ScrollDown) {
        view_.scroll(-6);
    }
}

void TerminalDemoPage::on_char(char ch) {
    if (!focused_) return;
    if (ch == '\b') {
        const char erase[] = {'\b', ' ', '\b'};
        view_.feed(erase, sizeof(erase));
    } else if (ch == '\t') {
        view_.feed("    ", 4);
    } else if (static_cast<unsigned char>(ch) >= 0x20
               && static_cast<unsigned char>(ch) <= 0x7e) {
        view_.feed(ch);
    }
}

} // namespace epui::demo
