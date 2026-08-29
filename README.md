# OpenOledUI

A small, portable UI framework for 128x64 monochrome OLEDs, designed first for common 0.96-inch modules and kept hardware-independent enough to run on STM32, ESP32, Raspberry Pi and Windows.

The repository still contains the original STM32F429 OLED example under `Examples/0.96_inch_oled_f429`. The new framework lives at the repository root and can be adopted incrementally.

## What is included

- 128x64 1-bit canvas with a 1024-byte framebuffer.
- Lines, rectangles, rounded cards, circles, progress bars and a compact 5x7 ASCII font.
- Page framework with `Next / Prev / Select / Back` input events.
- Smooth 220 ms cubic ease-out slide transitions without a second framebuffer.
- SSD1306 and SH1106 128x64 panel drivers.
- Hardware transport abstraction plus a function-callback adapter for MCU projects.
- Native Windows OLED simulator using Win32/GDI, with no OLED hardware or third-party GUI dependency.
- Raspberry Pi 5 status dashboard with system, network, PMIC power and terminal-output pages.

## Repository layout

```text
include/openoledui/        portable public headers
src/                       portable rendering/UI/panel code
examples/                  reusable demo pages
ports/windows/             native Windows OLED simulator
ports/raspberry_pi/        Raspberry Pi 5 I2C + monitoring application
docs/                      architecture and porting notes
Examples/0.96_inch_oled_f429/  original STM32 example
```

## Windows simulator

Requirements: Visual Studio 2022 Build Tools or another CMake-compatible Windows C++ compiler.

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\openoledui_sim.exe
```

Controls:

- Left / `A`: previous page
- Right / `D`: next page
- Enter / Space: select
- Esc: back

The simulator drives the exact same framebuffer and page animation code used by real OLED targets, so UI work can be done without hardware.

## Raspberry Pi 5

```bash
cmake -S . -B build-pi -DOPENOLEDUI_BUILD_RPI=ON
cmake --build build-pi -j4
./build-pi/openoledui_rpi /dev/i2c-1 0x3c ssd1306
```

See [`ports/raspberry_pi/README.md`](ports/raspberry_pi/README.md) for system pages, terminal FIFO usage and button guidance.

## MCU integration

The UI core does not include STM32 HAL, ESP-IDF, Arduino, FreeRTOS or Linux headers. Implement `OledTransport` around the board's I2C/SPI driver, or use `CallbackTransport` to wrap existing C functions.

```cpp
openoledui::CallbackTransport transport(ctx, write_fn, delay_fn);
openoledui::Oled128x64 oled(transport, openoledui::OledController::SSD1306);
openoledui::Canvas canvas;
openoledui::Ui ui;

oled.init();
// add Page objects to ui
ui.render(canvas, now_ms);
oled.present(canvas.data(), openoledui::Canvas::BufferSize);
```

Read [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) and [`docs/PORTING.md`](docs/PORTING.md) before adding a new platform.
