# Porting OpenOledUI to a new board

Implement `openoledui::OledTransport`, or use `CallbackTransport` to wrap existing board functions. Keep STM32 HAL, ESP-IDF, Arduino, FreeRTOS and Linux headers outside the core.

```cpp
#include <openoledui/callback_transport.hpp>
bool bus_write(void* ctx, bool data_mode, const uint8_t* data, size_t size) {
    // Call HAL_I2C_Master_Transmit, ESP-IDF I2C, Arduino Wire, etc.
    return true;
}
void delay_ms(void*, uint32_t ms) { /* platform delay */ }

openoledui::CallbackTransport bus(nullptr, bus_write, delay_ms);
openoledui::Oled128x64 oled(bus, openoledui::OledController::SSD1306);
oled.init();
```

Create pages once: pages depend only on `Canvas` and time. Translate hardware events into `Key::Next`, `Key::Prev`, `Key::Select`, `Key::Back`, then call `Ui::handle()`.

A 30 FPS render loop is a good MCU default; 60 FPS is useful in the Windows simulator.

```cpp
ui.render(canvas, millis);
oled.present(canvas.data(), openoledui::Canvas::BufferSize);
```
