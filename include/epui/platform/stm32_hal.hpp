#pragma once

#include <cstddef>
#include <cstdint>
#include "epui/i2c_transport.hpp"
#include "epui/platform_plugin.hpp"

namespace epui::platform {

struct Stm32HalHooks {
    void* user{};
    bool (*init)(void* user){};
    void (*deinit)(void* user){};
    bool (*i2c_write)(void* user, std::uint8_t address7, const std::uint8_t* data, std::size_t size){};
    void (*delay_ms)(void* user, std::uint32_t ms){};
    bool (*gpio_read)(void* user, int pin){};
};

class Stm32HalPlugin final : public PlatformPlugin {
public:
    explicit Stm32HalPlugin(Stm32HalHooks hooks, std::uint8_t oled_address = 0x3C,
                            const char* plugin_name = "stm32-hal")
        : hooks_(hooks), name_(plugin_name), transport_(hooks.user, oled_address, hooks.i2c_write, hooks.delay_ms) {}

    const char* name() const override { return name_; }
    bool start() override { return !hooks_.init || hooks_.init(hooks_.user); }
    void stop() override { if (hooks_.deinit) hooks_.deinit(hooks_.user); }

    CallbackI2cTransport& oled_transport() { return transport_; }
    const CallbackI2cTransport& oled_transport() const { return transport_; }

    bool read_gpio(int pin) const { return hooks_.gpio_read && hooks_.gpio_read(hooks_.user, pin); }
    static bool gpio_reader(void* user, int pin) {
        return user && static_cast<Stm32HalPlugin*>(user)->read_gpio(pin);
    }

private:
    Stm32HalHooks hooks_;
    const char* name_;
    CallbackI2cTransport transport_;
};

} // namespace epui::platform
