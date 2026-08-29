#pragma once

#include <cstddef>
#include <cstdint>
#include "epui/input_plugin.hpp"

namespace epui {

template <std::size_t MaxButtons = 4, std::size_t QueueCapacity = 8>
class GpioButtonPlugin final : public InputPlugin {
public:
    using ReadFn = bool (*)(void* user, int pin);

    GpioButtonPlugin(const char* plugin_name, void* user, ReadFn read,
                     std::uint32_t debounce_ms = 25)
        : name_(plugin_name), user_(user), read_(read), debounce_ms_(debounce_ms) {}

    const char* name() const override { return name_; }

    bool add_button(int pin, Key key, bool active_low = true) {
        if (count_ >= MaxButtons) return false;
        buttons_[count_++] = {pin, key, active_low};
        return true;
    }

    bool start() override {
        if (!read_) return false;
        for (std::size_t i = 0; i < count_; ++i) {
            Button& b = buttons_[i];
            const bool raw = pressed(b);
            b.stable = raw;
            b.candidate = raw;
            b.initialized = true;
        }
        return true;
    }

    void tick(std::uint32_t now_ms) override {
        if (!read_) return;
        for (std::size_t i = 0; i < count_; ++i) {
            Button& b = buttons_[i];
            const bool raw = pressed(b);
            if (!b.initialized) {
                b.stable = b.candidate = raw;
                b.initialized = true;
                continue;
            }
            if (raw != b.candidate) {
                b.candidate = raw;
                b.changed_ms = now_ms;
                continue;
            }
            if (b.candidate == b.stable) continue;
            if (static_cast<std::uint32_t>(now_ms - b.changed_ms) < debounce_ms_) continue;
            b.stable = b.candidate;
            queue_.push({b.key, b.stable});
        }
    }

    bool poll(InputEvent& event) override { return queue_.pop(event); }
    std::size_t button_count() const { return count_; }

private:
    struct Button {
        int pin{0};
        Key key{Key::Select};
        bool active_low{true};
        bool stable{false};
        bool candidate{false};
        bool initialized{false};
        std::uint32_t changed_ms{0};
    };

    bool pressed(const Button& b) const {
        const bool level = read_(user_, b.pin);
        return b.active_low ? !level : level;
    }

    const char* name_;
    void* user_{};
    ReadFn read_{};
    std::uint32_t debounce_ms_{25};
    Button buttons_[MaxButtons]{};
    std::size_t count_{0};
    InputQueue<QueueCapacity> queue_;
};

} // namespace epui
