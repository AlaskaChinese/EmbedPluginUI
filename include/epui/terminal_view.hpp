#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include "epui/canvas.hpp"

namespace epui {

// A small line-oriented terminal renderer. It handles ASCII text, CR/LF,
// backspace, hard wrapping, and skips ESC/CSI/OSC control sequences. It is not
// a VT100 emulator and intentionally does not support cursor-addressed apps.
template <std::size_t HistoryLines = 64, std::size_t Columns = 21>
class TerminalView {
public:
    static_assert(HistoryLines > 0, "TerminalView requires at least one history line");
    static_assert(Columns > 0, "TerminalView requires at least one column");

    static constexpr int CellWidth = 6;
    static constexpr int CellHeight = 7;
    static constexpr std::size_t StorageBytes = HistoryLines * (Columns + 1);

    TerminalView() { clear(); }

    void feed(char ch) {
        const auto byte = static_cast<unsigned char>(ch);
        switch (parser_state_) {
        case ParserState::Text:
            if (byte == 0x1b) parser_state_ = ParserState::Escape;
            else consume_text(byte);
            break;
        case ParserState::Escape:
            if (byte == '[') parser_state_ = ParserState::Csi;
            else if (byte == ']') parser_state_ = ParserState::Osc;
            else if (byte == 0x1b) parser_state_ = ParserState::Escape;
            else if (byte >= 0x30 && byte <= 0x7e) parser_state_ = ParserState::Text;
            else if (byte < 0x20 || byte > 0x2f) parser_state_ = ParserState::Text;
            break;
        case ParserState::Csi:
            if (byte == 0x1b) parser_state_ = ParserState::Escape;
            else if (byte >= 0x40 && byte <= 0x7e) parser_state_ = ParserState::Text;
            break;
        case ParserState::Osc:
            if (byte == 0x07) parser_state_ = ParserState::Text;
            else if (byte == 0x1b) parser_state_ = ParserState::OscEscape;
            break;
        case ParserState::OscEscape:
            if (byte == '\\') parser_state_ = ParserState::Text;
            else if (byte != 0x1b) parser_state_ = ParserState::Osc;
            break;
        }
    }

    void feed(const char* data, std::size_t size) {
        if (!data) return;
        for (std::size_t i = 0; i < size; ++i) feed(data[i]);
    }

    void clear() {
        for (auto& row : lines_) row.fill(0);
        lengths_.fill(0);
        first_ = 0;
        count_ = 1;
        column_ = 0;
        scroll_offset_ = 0;
        parser_state_ = ParserState::Text;
    }

    // Positive values move toward older output; negative values move back to
    // the live cursor. New terminal output returns to the live cursor.
    void scroll(int lines) {
        const std::size_t maximum = count_ - 1;
        if (lines > 0) {
            const auto amount = static_cast<std::size_t>(lines);
            scroll_offset_ = amount > maximum - scroll_offset_
                ? maximum : scroll_offset_ + amount;
        } else if (lines < 0) {
            const auto amount = static_cast<std::size_t>(-(lines + 1)) + 1;
            scroll_offset_ = amount > scroll_offset_ ? 0 : scroll_offset_ - amount;
        }
    }

    void set_cursor_visible(bool visible) { cursor_visible_ = visible; }
    bool cursor_visible() const { return cursor_visible_; }
    std::size_t cursor_column() const { return column_; }
    std::size_t line_count() const { return count_; }
    std::size_t scroll_offset() const { return scroll_offset_; }

    // Lines are indexed oldest first and remain valid until the next feed or
    // clear operation.
    const char* line(std::size_t index) const {
        return index < count_ ? lines_[physical_index(index)].data() : "";
    }

    void draw(Canvas& canvas, int x, int y, int width, int height,
              std::uint32_t now_ms) const {
        if (width <= 0 || height <= 0) return;
        canvas.set_clip_rect(x, y, width, height);

        const std::size_t visible_columns = std::min<std::size_t>(
            Columns, static_cast<std::size_t>((width + 1) / CellWidth));
        const std::size_t visible_rows = static_cast<std::size_t>(height / CellHeight);
        if (visible_columns == 0 || visible_rows == 0) {
            canvas.reset_clip();
            return;
        }

        const std::size_t end = count_ - std::min(scroll_offset_, count_ - 1);
        const std::size_t shown = std::min(visible_rows, end);
        const std::size_t begin = end - shown;
        int row_y = y + static_cast<int>((visible_rows - shown) * CellHeight);
        for (std::size_t row = begin; row < end; ++row, row_y += CellHeight) {
            const std::size_t physical = physical_index(row);
            const std::size_t chars = std::min(visible_columns, lengths_[physical]);
            for (std::size_t column = 0; column < chars; ++column) {
                canvas.glyph5x7(x + static_cast<int>(column * CellWidth), row_y,
                                lines_[physical][column]);
            }
        }

        if (cursor_visible_ && scroll_offset_ == 0 && shown != 0
            && ((now_ms / 500u) & 1u) == 0u) {
            std::size_t cursor_column = column_;
            if (cursor_column == Columns && visible_columns == Columns) --cursor_column;
            if (cursor_column < visible_columns) {
                const int cursor_y = y + static_cast<int>((visible_rows - 1) * CellHeight);
                canvas.invert_rect(x + static_cast<int>(cursor_column * CellWidth),
                                   cursor_y, CellWidth, CellHeight);
            }
        }

        // Canvas currently has one top-level clip rather than a clip stack.
        canvas.reset_clip();
    }

private:
    enum class ParserState : std::uint8_t { Text, Escape, Csi, Osc, OscEscape };

    std::size_t physical_index(std::size_t logical) const {
        return (first_ + logical) % HistoryLines;
    }

    std::size_t current_index() const { return physical_index(count_ - 1); }

    void consume_text(unsigned char byte) {
        if (byte == '\n') {
            push_line();
        } else if (byte == '\r') {
            column_ = 0;
            scroll_offset_ = 0;
        } else if (byte == '\b') {
            if (column_ > 0) --column_;
            scroll_offset_ = 0;
        } else if (byte >= 0x20 && byte <= 0x7e) {
            if (column_ >= Columns) push_line();
            const std::size_t current = current_index();
            lines_[current][column_] = static_cast<char>(byte);
            ++column_;
            if (column_ > lengths_[current]) {
                lengths_[current] = column_;
                lines_[current][column_] = 0;
            }
            scroll_offset_ = 0;
        }
    }

    void push_line() {
        if (count_ < HistoryLines) {
            ++count_;
        } else {
            first_ = (first_ + 1) % HistoryLines;
        }
        const std::size_t current = current_index();
        lines_[current].fill(0);
        lengths_[current] = 0;
        column_ = 0;
        scroll_offset_ = 0;
    }

    std::array<std::array<char, Columns + 1>, HistoryLines> lines_{};
    std::array<std::size_t, HistoryLines> lengths_{};
    std::size_t first_{0};
    std::size_t count_{1};
    std::size_t column_{0};
    std::size_t scroll_offset_{0};
    ParserState parser_state_{ParserState::Text};
    bool cursor_visible_{true};
};

} // namespace epui
