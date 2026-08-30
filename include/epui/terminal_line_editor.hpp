#pragma once

#include <cstddef>
#include <cstring>

namespace epui {

template <std::size_t Capacity = 128, std::size_t HistoryCapacity = 16>
class TerminalLineEditor {
public:
    static_assert(Capacity > 0, "TerminalLineEditor requires a command capacity");
    static_assert(HistoryCapacity > 0, "TerminalLineEditor requires history storage");

    const char* command() const { return command_; }
    std::size_t length() const { return length_; }
    std::size_t cursor() const { return cursor_; }
    std::size_t history_count() const { return history_count_; }

    bool insert(char ch) {
        if (length_ >= Capacity) return false;
        detach_history();
        for (std::size_t i = length_; i > cursor_; --i) command_[i] = command_[i - 1];
        command_[cursor_++] = ch;
        command_[++length_] = 0;
        return true;
    }

    bool erase_before_cursor() {
        if (cursor_ == 0) return false;
        detach_history();
        for (std::size_t i = cursor_ - 1; i < length_; ++i) command_[i] = command_[i + 1];
        --cursor_;
        --length_;
        return true;
    }

    bool move_left() {
        if (cursor_ == 0) return false;
        --cursor_;
        return true;
    }

    bool move_right() {
        if (cursor_ >= length_) return false;
        ++cursor_;
        return true;
    }

    bool previous_history() {
        if (history_count_ == 0 || history_position_ == 0) return false;
        if (history_position_ == history_count_) copy_line(draft_, command_, length_);
        --history_position_;
        load(history_[history_position_]);
        return true;
    }

    bool next_history() {
        if (history_position_ >= history_count_) return false;
        ++history_position_;
        load(history_position_ == history_count_ ? draft_ : history_[history_position_]);
        return true;
    }

    void commit() {
        if (length_ != 0
            && (history_count_ == 0
                || std::strcmp(history_[history_count_ - 1], command_) != 0)) {
            if (history_count_ == HistoryCapacity) {
                for (std::size_t i = 1; i < HistoryCapacity; ++i) {
                    copy_line(history_[i - 1], history_[i], std::strlen(history_[i]));
                }
                --history_count_;
            }
            copy_line(history_[history_count_++], command_, length_);
        }
        clear();
    }

    void clear() {
        command_[0] = 0;
        draft_[0] = 0;
        length_ = 0;
        cursor_ = 0;
        history_position_ = history_count_;
    }

private:
    static void copy_line(char (&destination)[Capacity + 1],
                          const char* source, std::size_t length) {
        if (length != 0) std::memcpy(destination, source, length);
        destination[length] = 0;
    }

    void load(const char* text) {
        length_ = std::strlen(text);
        copy_line(command_, text, length_);
        cursor_ = length_;
    }

    void detach_history() { history_position_ = history_count_; }

    char command_[Capacity + 1]{};
    char draft_[Capacity + 1]{};
    char history_[HistoryCapacity][Capacity + 1]{};
    std::size_t length_{0};
    std::size_t cursor_{0};
    std::size_t history_count_{0};
    std::size_t history_position_{0};
};

} // namespace epui
