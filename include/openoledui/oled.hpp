#pragma once

#include <cstddef>
#include <cstdint>

namespace openoledui {

enum class OledController : std::uint8_t { SSD1306, SH1106 };

class OledTransport {
public:
    virtual ~OledTransport() = default;
    virtual bool write_command(const std::uint8_t* data, std::size_t size) = 0;
    virtual bool write_data(const std::uint8_t* data, std::size_t size) = 0;
    virtual void delay_ms(std::uint32_t ms) = 0;
};

class Oled128x64 {
public:
    explicit Oled128x64(OledTransport& transport, OledController controller = OledController::SSD1306, std::uint8_t contrast = 0xCF);
    bool init();
    bool present(const std::uint8_t* framebuffer, std::size_t size);
    bool set_contrast(std::uint8_t contrast);
    bool power(bool on);

private:
    bool command(std::uint8_t value);
    bool commands(const std::uint8_t* data, std::size_t size);
    OledTransport& transport_;
    OledController controller_;
    std::uint8_t contrast_;
};

} // namespace openoledui
