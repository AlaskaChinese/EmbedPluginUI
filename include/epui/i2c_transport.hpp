#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include "epui/oled.hpp"

namespace epui {

class CallbackI2cTransport final : public OledTransport {
public:
    using WriteFn = bool (*)(void* user, std::uint8_t address7, const std::uint8_t* data, std::size_t size);
    using DelayFn = void (*)(void* user, std::uint32_t ms);
    static constexpr std::size_t MaxPayload = 31;

    CallbackI2cTransport(void* user, std::uint8_t address7, WriteFn write, DelayFn delay = nullptr)
        : user_(user), address7_(address7), write_(write), delay_(delay) {}

    bool write_command(const std::uint8_t* data, std::size_t size) override {
        return write_prefixed(0x00, data, size);
    }
    bool write_data(const std::uint8_t* data, std::size_t size) override {
        return write_prefixed(0x40, data, size);
    }
    void delay_ms(std::uint32_t ms) override {
        if (delay_) delay_(user_, ms);
    }

    std::uint8_t address() const { return address7_; }

private:
    bool write_prefixed(std::uint8_t control, const std::uint8_t* data, std::size_t size) {
        if (!write_ || (!data && size != 0)) return false;
        std::uint8_t packet[MaxPayload + 1]{};
        packet[0] = control;
        while (size > 0) {
            const std::size_t chunk = std::min(size, MaxPayload);
            for (std::size_t i = 0; i < chunk; ++i) packet[i + 1] = data[i];
            if (!write_(user_, address7_, packet, chunk + 1)) return false;
            data += chunk;
            size -= chunk;
        }
        return true;
    }

    void* user_{};
    std::uint8_t address7_{0x3C};
    WriteFn write_{};
    DelayFn delay_{};
};

} // namespace epui
