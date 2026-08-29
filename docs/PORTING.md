# Porting EmbedPluginUI to a new board

Implement `epui::OledTransport`, or use `epui::CallbackTransport` to wrap existing board functions. Keep STM32 HAL, ESP-IDF, Arduino, FreeRTOS and Linux headers outside the portable core.

```cpp
#include <epui/callback_transport.hpp>
#include <epui/oled.hpp>

bool bus_write(void* ctx, bool data_mode, const uint8_t* data, size_t size) {
    // HAL_I2C_Master_Transmit, ESP-IDF I2C, Arduino Wire, ...
    return true;
}

void delay_ms(void*, uint32_t ms) {
    // platform delay
}

epui::CallbackTransport bus(nullptr, bus_write, delay_ms);
epui::Oled128x64 oled(bus, epui::OledController::SSD1306);
oled.init();
```

Translate platform input into `epui::Key::Next`, `Prev`, `Select` and `Back`, then call `epui::Ui::handle()`.

A 30 FPS loop is a good MCU default. The Ubuntu/Windows simulators target roughly 60 FPS for animation development.
