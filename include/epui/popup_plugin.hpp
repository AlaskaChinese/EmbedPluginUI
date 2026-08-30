#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "epui/page.hpp"
#include "epui/plugin.hpp"
#include "epui/spring.hpp"

namespace epui {

enum class PopupButtons : std::uint8_t { Ok, OkCancel };
enum class PopupResult : std::uint8_t { Accepted, Cancelled };
enum class PopupState : std::uint8_t { Hidden, Opening, Visible, Closing };
using PopupCallback = void (*)(void* user, PopupResult result);

struct PopupStyle {
    int x{8};
    int resting_y{11};
    int width{112};
    int height{42};
    int radius{4};
    SpringStyle spring{};
    float stretch_per_velocity{0.12f};
    int max_stretch_px{3};
};

class PopupPlugin final : public Plugin, public UiOverlay {
public:
    explicit PopupPlugin(Ui& ui, const char* plugin_name = "popup",
                         PopupStyle style = PopupStyle{})
        : ui_(ui), name_(plugin_name), style_(style) {
        spring_.reset(hidden_y());
    }

    const char* name() const override { return name_; }
    PluginKind kind() const override { return PluginKind::Overlay; }

    bool start() override {
        if (attached_) return true;
        if (!ui_.add_overlay(*this)) return false;
        attached_ = true;
        return true;
    }

    void stop() override {
        if (attached_) ui_.remove_overlay(*this);
        attached_ = false;
        state_ = PopupState::Hidden;
        callback_ = nullptr;
        callback_user_ = nullptr;
        spring_.reset(hidden_y());
        has_time_ = false;
    }

    void show(const char* title, const char* message,
              PopupButtons buttons = PopupButtons::Ok,
              PopupCallback callback = nullptr, void* user = nullptr) {
        copy_text(title_, sizeof(title_), title);
        copy_text(message_, sizeof(message_), message);
        buttons_ = buttons;
        selected_ = 0;
        callback_ = callback;
        callback_user_ = user;
        pending_result_ = PopupResult::Cancelled;
        if (state_ == PopupState::Hidden) spring_.reset(hidden_y());
        spring_.set_target(static_cast<float>(style_.resting_y));
        state_ = PopupState::Opening;
        has_time_ = false;
    }

    void close(PopupResult result = PopupResult::Cancelled) {
        if (state_ == PopupState::Hidden || state_ == PopupState::Closing) return;
        pending_result_ = result;
        spring_.set_target(hidden_y());
        state_ = PopupState::Closing;
    }

    bool attached() const { return attached_; }
    bool visible() const { return state_ != PopupState::Hidden; }
    bool animating() const {
        return state_ == PopupState::Opening || state_ == PopupState::Closing;
    }
    PopupState state() const { return state_; }
    std::size_t selected_index() const { return selected_; }
    float position() const { return spring_.position(); }
    float velocity() const { return spring_.velocity(); }
    int stretch_pixels() const {
        return std::max(0, std::min(style_.max_stretch_px,
            round_to_int(absf(spring_.velocity()) * style_.stretch_per_velocity)));
    }
    const PopupStyle& style() const { return style_; }

    bool captures_input() const override { return visible(); }

    void on_key(Key key) override {
        if (state_ == PopupState::Closing || state_ == PopupState::Hidden) return;
        if (buttons_ == PopupButtons::OkCancel
            && (key == Key::Next || key == Key::Prev
                || key == Key::Left || key == Key::Right)) {
            selected_ = selected_ == 0 ? 1 : 0;
        } else if (key == Key::Select) {
            close(selected_ == 0 ? PopupResult::Accepted : PopupResult::Cancelled);
        } else if (key == Key::Back) {
            close(PopupResult::Cancelled);
        }
    }

    void on_char(char) override {}

    void draw_overlay(Canvas& canvas, std::uint32_t now_ms) override {
        if (state_ == PopupState::Hidden) return;
        advance(now_ms);
        if (state_ == PopupState::Hidden) return;

        const int y = round_to_int(spring_.position());
        const int height = style_.height + stretch_pixels();

        canvas.fill_round_rect(style_.x, y, style_.width, height,
                               style_.radius, false);
        canvas.round_rect(style_.x, y, style_.width, height,
                          style_.radius, true);
        canvas.set_clip_rect(style_.x + 1, y + 1, style_.width - 2, height - 2);
        canvas.text(style_.x + 5, y + 4, title_);
        canvas.line(style_.x + 3, y + 12, style_.x + style_.width - 4, y + 12);
        draw_message(canvas, y);
        draw_buttons(canvas, y, height);
        canvas.reset_clip();
    }

private:
    static constexpr std::size_t TitleCapacity = 32;
    static constexpr std::size_t MessageCapacity = 96;

    static float absf(float value) { return value < 0.0f ? -value : value; }
    static int round_to_int(float value) {
        return static_cast<int>(value >= 0.0f ? value + 0.5f : value - 0.5f);
    }

    float hidden_y() const { return static_cast<float>(-style_.height - 1); }

    static void copy_text(char* destination, std::size_t size, const char* source) {
        if (size == 0) return;
        if (!source) source = "";
        std::strncpy(destination, source, size - 1);
        destination[size - 1] = 0;
    }

    static std::size_t utf8_bytes(unsigned char lead) {
        if ((lead & 0x80u) == 0u) return 1;
        if ((lead & 0xe0u) == 0xc0u) return 2;
        if ((lead & 0xf0u) == 0xe0u) return 3;
        if ((lead & 0xf8u) == 0xf0u) return 4;
        return 1;
    }

    void draw_message(Canvas& canvas, int y) const {
        const char* cursor = message_;
        const int available = style_.width - 10;
        for (int row = 0; row < 2 && *cursor; ++row) {
            char line[MessageCapacity + 1]{};
            std::size_t used = 0;
            while (*cursor && *cursor != '\n') {
                std::size_t bytes = utf8_bytes(static_cast<unsigned char>(*cursor));
                std::size_t actual = 0;
                while (actual < bytes && cursor[actual]) ++actual;
                if (actual == 0 || used + actual >= sizeof(line)) break;
                std::memcpy(line + used, cursor, actual);
                line[used + actual] = 0;
                if (canvas.text_width(line) > available && used != 0) {
                    line[used] = 0;
                    break;
                }
                used += actual;
                cursor += actual;
            }
            if (*cursor == '\n') ++cursor;
            canvas.text(style_.x + 5, y + 15 + row * 8, line);
        }
    }

    void draw_button(Canvas& canvas, int center_x, int y,
                     const char* label, bool selected) const {
        const int text_width = canvas.text_width(label);
        const int width = text_width + 6;
        const int x = center_x - width / 2;
        if (selected) {
            canvas.fill_round_rect(x, y, width, 9, 2, true);
            canvas.text(x + 3, y + 1, label, false);
        } else {
            canvas.round_rect(x, y, width, 9, 2, true);
            canvas.text(x + 3, y + 1, label);
        }
    }

    void draw_buttons(Canvas& canvas, int y, int height) const {
        const int button_y = y + height - 11;
        if (buttons_ == PopupButtons::Ok) {
            draw_button(canvas, style_.x + style_.width / 2, button_y,
                        "OK", true);
            return;
        }
        draw_button(canvas, style_.x + style_.width / 3, button_y,
                    "OK", selected_ == 0);
        draw_button(canvas, style_.x + style_.width * 2 / 3, button_y,
                    "CANCEL", selected_ == 1);
    }

    void advance(std::uint32_t now_ms) {
        if (!has_time_) {
            last_ms_ = now_ms;
            has_time_ = true;
            return;
        }
        spring_.step(now_ms - last_ms_, style_.spring);
        last_ms_ = now_ms;
        if (!spring_.settled(style_.spring)) return;
        if (state_ == PopupState::Opening) {
            state_ = PopupState::Visible;
            return;
        }
        if (state_ != PopupState::Closing) return;

        state_ = PopupState::Hidden;
        has_time_ = false;
        PopupCallback callback = callback_;
        void* user = callback_user_;
        const PopupResult result = pending_result_;
        callback_ = nullptr;
        callback_user_ = nullptr;
        if (callback) callback(user, result);
    }

    Ui& ui_;
    const char* name_;
    PopupStyle style_{};
    Spring1D spring_{};
    PopupCallback callback_{nullptr};
    void* callback_user_{nullptr};
    PopupResult pending_result_{PopupResult::Cancelled};
    PopupButtons buttons_{PopupButtons::Ok};
    PopupState state_{PopupState::Hidden};
    char title_[TitleCapacity + 1]{};
    char message_[MessageCapacity + 1]{};
    std::uint32_t last_ms_{0};
    std::size_t selected_{0};
    bool attached_{false};
    bool has_time_{false};
};

} // namespace epui
