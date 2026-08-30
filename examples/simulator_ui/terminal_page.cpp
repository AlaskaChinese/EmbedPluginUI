#include "terminal_page.hpp"
#include "epui/widgets.hpp"
#include <algorithm>

namespace epui::demo {

TerminalDemoPage::TerminalDemoPage(TerminalControls controls)
    : controls_(controls) {
    constexpr char intro[] = "Terminal demo ready\n";
    view_.feed(intro, sizeof(intro) - 1);
    view_.set_cursor_visible(false);
}

void TerminalDemoPage::draw(Canvas& canvas, std::uint32_t now_ms) {
    draw_header(canvas, focused_ ? "Terminal *" : "Terminal", 5, 6);
    canvas.text(1, 15, ">");

    std::size_t first = 0;
    if (editor_.cursor() >= VisibleColumns) first = editor_.cursor() - VisibleColumns + 1;
    const std::size_t last = std::min(editor_.length(), first + VisibleColumns);
    for (std::size_t i = first; i < last; ++i) {
        canvas.glyph5x7(8 + static_cast<int>((i - first) * 6), 15,
                        editor_.command()[i]);
    }
    if (focused_ && ((now_ms / 500u) & 1u) == 0u) {
        const std::size_t visible_cursor = editor_.cursor() - first;
        canvas.invert_rect(8 + static_cast<int>(visible_cursor * 6), 15, 6, 7);
    }

    canvas.line(0, 23, 127, 23);
    view_.draw(canvas, 1, 25, 126, 32, now_ms);
}

bool TerminalDemoPage::captures_key(Key key) const {
    return controls_.captures(key, focused_);
}

void TerminalDemoPage::on_key(Key key) {
    const TerminalAction action = controls_.action_for(key, focused_);
    if (action == TerminalAction::Focus) {
        focused_ = true;
    } else if (action == TerminalAction::Unfocus) {
        focused_ = false;
    } else if (action == TerminalAction::Execute) {
        execute();
    } else if (action == TerminalAction::CursorLeft) {
        editor_.move_left();
    } else if (action == TerminalAction::CursorRight) {
        editor_.move_right();
    } else if (action == TerminalAction::HistoryPrevious) {
        editor_.previous_history();
    } else if (action == TerminalAction::HistoryNext) {
        editor_.next_history();
    } else if (action == TerminalAction::OutputUp) {
        view_.scroll(1);
    } else if (action == TerminalAction::OutputDown) {
        view_.scroll(-1);
    }
}

void TerminalDemoPage::on_char(char ch) {
    if (!focused_) return;
    if (ch == '\b' || ch == '\x7f') {
        editor_.erase_before_cursor();
    } else if (ch == '\t') {
        for (int i = 0; i < 4; ++i) editor_.insert(' ');
    } else if (static_cast<unsigned char>(ch) >= 0x20
               && static_cast<unsigned char>(ch) <= 0x7e) {
        editor_.insert(ch);
    }
}

void TerminalDemoPage::execute() {
    view_.feed("$ ", 2);
    if (editor_.length() != 0) view_.feed(editor_.command(), editor_.length());
    view_.feed('\n');
    editor_.commit();
}

} // namespace epui::demo
