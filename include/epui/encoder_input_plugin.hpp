#pragma once

#include <cstddef>
#include <cstdint>
#include "epui/input_plugin.hpp"

namespace epui {

template <std::size_t QueueCapacity = 8>
class EncoderInputPlugin final : public InputPlugin {
public:
    using ReadFn = bool (*)(void* user, int pin);

    EncoderInputPlugin(const char* plugin_name, void* user, ReadFn read,
                       int pin_a, int pin_b, Key clockwise = Key::Next, Key counter_clockwise = Key::Prev)
        : name_(plugin_name), user_(user), read_(read), pin_a_(pin_a), pin_b_(pin_b),
          clockwise_(clockwise), counter_clockwise_(counter_clockwise) {}

    const char* name() const override { return name_; }

    bool start() override {
        if (!read_) return false;
        previous_ = state();
        accumulator_ = 0;
        return true;
    }

    void tick(std::uint32_t) override {
        const std::uint8_t current = state();
        const std::uint8_t transition = static_cast<std::uint8_t>((previous_ << 2u) | current);
        previous_ = current;
        static constexpr std::int8_t kDelta[16] = {
             0, -1,  1,  0,
             1,  0,  0, -1,
            -1,  0,  0,  1,
             0,  1, -1,  0,
        };
        accumulator_ = static_cast<std::int8_t>(accumulator_ + kDelta[transition]);
        if (accumulator_ >= 4) {
            queue_.push({clockwise_, true});
            accumulator_ = 0;
        } else if (accumulator_ <= -4) {
            queue_.push({counter_clockwise_, true});
            accumulator_ = 0;
        }
    }

    bool poll(InputEvent& event) override { return queue_.pop(event); }

private:
    std::uint8_t state() const {
        return static_cast<std::uint8_t>((read_(user_, pin_a_) ? 2u : 0u) |
                                         (read_(user_, pin_b_) ? 1u : 0u));
    }

    const char* name_;
    void* user_{};
    ReadFn read_{};
    int pin_a_{0};
    int pin_b_{0};
    Key clockwise_{Key::Next};
    Key counter_clockwise_{Key::Prev};
    std::uint8_t previous_{0};
    std::int8_t accumulator_{0};
    InputQueue<QueueCapacity> queue_;
};

} // namespace epui
