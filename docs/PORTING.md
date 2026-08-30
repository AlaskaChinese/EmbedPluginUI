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

Keyboard ports should translate the four arrows into `epui::Key::Up`, `Down`,
`Left` and `Right`. `Ui` uses unclaimed Left/Right for page navigation, while
menus use Up/Down for selection and Left/Right for hierarchy. Encoders can
continue producing `Next`/`Prev`; those remain compatible with page and menu
navigation. Map explicit terminal viewport actions to `ScrollUp` and
`ScrollDown`; the Raspberry Pi keyboard maps Ctrl+Up/Down and its terminal
page scrolls one output line per event.
Text-capable inputs set `InputEvent::ch`; key events leave it as zero. Always
overwrite the complete event before returning it from `InputPlugin::poll()`.

```cpp
epui::InputEvent event{};
while (input.poll(event)) ui.handle(event, now_ms);
```

Release events are ignored by `Ui`. The topmost input-capturing `UiOverlay`
receives keys and characters first. With no modal overlay, character input goes
directly to the current stable page through `Page::on_char()` and does not use
`captures_key()`. Pages that accept text should gate `on_char()` with their own
focus state.

Source files may use UTF-8 literals for the compact symbol set documented in
`GRAPHICS.md`; no locale or OS text service is required at runtime.

A 30 FPS loop is a good MCU default. The Ubuntu/Windows simulators target roughly 60 FPS for animation development.
