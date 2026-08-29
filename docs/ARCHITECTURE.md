# OpenOledUI architecture

OpenOledUI is split into four layers so UI code can move between STM32, ESP32, Raspberry Pi and Windows without being rewritten.

```text
Application pages / widgets / animation
                |
            Canvas + Ui
                |
        128 x 64 1-bit framebuffer
                |
        Oled128x64 panel driver
          /                 \
 SSD1306 / SH1106       future panels
                |
           OledTransport
      /          |           \
 STM32 HAL     Linux I2C    ESP-IDF / Arduino
```

## Core rules

- The core library is C++17 and has no OS dependency.
- The framebuffer is fixed at 1024 bytes for 128x64 monochrome displays.
- Rendering uses no dynamic allocation.
- Page storage is fixed at eight pages and uses references owned by the application.
- Animation is time based. A 220 ms cubic ease-out transition gives smooth page movement at 30-60 FPS.
- Display hardware is hidden behind `OledTransport`; UI pages never know whether they run on STM32, Raspberry Pi or Windows.

## Controller support

`Oled128x64` currently supports SSD1306 and SH1106, including the common two-column SH1106 RAM offset used by 128x64 modules.

## Input

The portable input vocabulary is `Key::Next`, `Key::Prev`, `Key::Select`, `Key::Back`. Each platform maps buttons, encoders, keyboard keys or touch events to those events.

## Memory budget

Typical core RAM usage is dominated by the 1024-byte framebuffer. The UI transition renders both pages into the same framebuffer using translated drawing coordinates, so slide animation does not need a second 1024-byte buffer.
