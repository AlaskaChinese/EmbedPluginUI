#pragma once

#include <cstddef>
#include <cstdint>
#include "epui/oled.hpp"

namespace epui {

class CallbackTransport final : public OledTransport {
public:
    using WriteFn = bool (*)(void* user, bool data_mode, const std::uint8_t* data, std::size_t size);
    using DelayFn = void (*)(void* user, std::uint32_t ms);
    CallbackTransport(void* user, WriteFn write, DelayFn delay) : user_(user), write_(write), delay_(delay) {}
    bool write_command(const std::uint8_t* data, std::size_t size) override { return write_ && write_(user_, false, data, size); }
    bool write_data(const std::uint8_t* data, std::size_t size) override { return write_ && write_(user_, true, data, size); }
    void delay_ms(std::uint32_t ms) override { if (delay_) delay_(user_, ms); }
private:
    void* user_{};
    WriteFn write_{};
    DelayFn delay_{};
};

} // namespace epui
