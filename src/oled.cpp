#include "openoledui/oled.hpp"

namespace openoledui {
Oled128x64::Oled128x64(OledTransport& transport, OledController controller, std::uint8_t contrast) : transport_(transport), controller_(controller), contrast_(contrast) {}
bool Oled128x64::command(std::uint8_t value) { return transport_.write_command(&value, 1); }
bool Oled128x64::commands(const std::uint8_t* data, std::size_t size) { return transport_.write_command(data, size); }
bool Oled128x64::init() {
    transport_.delay_ms(20);
    if (controller_ == OledController::SSD1306) {
        const std::uint8_t init_seq[] = {0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0x8D,0x14,0x20,0x02,0xA1,0xC8,0xDA,0x12,0x81,contrast_,0xD9,0xF1,0xDB,0x40,0xA4,0xA6,0xAF};
        return commands(init_seq, sizeof(init_seq));
    }
    const std::uint8_t init_seq[] = {0xAE,0xD5,0x80,0xA8,0x3F,0xD3,0x00,0x40,0xAD,0x8B,0xA1,0xC8,0xDA,0x12,0x81,contrast_,0xD9,0x22,0xDB,0x35,0xA4,0xA6,0xAF};
    return commands(init_seq, sizeof(init_seq));
}
bool Oled128x64::present(const std::uint8_t* framebuffer, std::size_t size) {
    if (!framebuffer || size < 1024) return false;
    for (std::uint8_t page=0; page<8; ++page) {
        if (controller_ == OledController::SSD1306) { const std::uint8_t cmd[] = {static_cast<std::uint8_t>(0xB0|page),0x00,0x10}; if (!commands(cmd,sizeof(cmd))) return false; }
        else { const std::uint8_t cmd[] = {static_cast<std::uint8_t>(0xB0|page),0x02,0x10}; if (!commands(cmd,sizeof(cmd))) return false; }
        if (!transport_.write_data(framebuffer + static_cast<std::size_t>(page)*128, 128)) return false;
    }
    return true;
}
bool Oled128x64::set_contrast(std::uint8_t contrast) { contrast_ = contrast; const std::uint8_t cmd[] = {0x81,contrast_}; return commands(cmd,sizeof(cmd)); }
bool Oled128x64::power(bool on) { return command(on ? 0xAF : 0xAE); }
} // namespace openoledui
