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
    if (cursor_ >= VisibleColumns) first = cursor_ - VisibleColumns + 1;
    const std::size_t last = std::min(length_, first + VisibleColumns);
    for (std::size_t i = first; i < last; ++i) {
        canvas.glyph5x7(8 + static_cast<int>((i - first) * 6), 15, command_[i]);
    }
    if (focused_ && ((now_ms / 500u) & 1u) == 0u) {
        const std::size_t visible_cursor = cursor_ - first;
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
        if (cursor_ > 0) --cursor_;
    } else if (action == TerminalAction::CursorRight) {
        if (cursor_ < length_) ++cursor_;
    } else if (action == TerminalAction::OutputUp) {
        view_.scroll(1);
    } else if (action == TerminalAction::OutputDown) {
        view_.scroll(-1);
    }
}

void TerminalDemoPage::on_char(char ch) {
    if (!focused_) return;
    if (ch == '\b' || ch == '\x7f') {
        erase_before_cursor();
    } else if (ch == '\t') {
        for (int i = 0; i < 4; ++i) insert(' ');
    } else if (static_cast<unsigned char>(ch) >= 0x20
               && static_cast<unsigned char>(ch) <= 0x7e) {
        insert(ch);
    }
}

void TerminalDemoPage::insert(char ch) {
    if (length_ >= CommandCapacity) return;
    for (std::size_t i = length_; i > cursor_; --i) command_[i] = command_[i - 1];
    command_[cursor_++] = ch;
    command_[++length_] = 0;
}

void TerminalDemoPage::erase_before_cursor() {
    if (cursor_ == 0) return;
    for (std::size_t i = cursor_ - 1; i < length_; ++i) command_[i] = command_[i + 1];
    --cursor_;
    --length_;
}

void TerminalDemoPage::execute() {
    view_.feed("$ ", 2);
    if (length_ != 0) view_.feed(command_, length_);
    view_.feed('\n');
    length_ = 0;
    cursor_ = 0;
    command_[0] = 0;
}

} // namespace epui::demo
