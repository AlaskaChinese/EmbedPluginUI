# EmbedPluginUI

**A plugin-first embedded UI framework.**

EmbedPluginUI is a portable C++17 UI framework for small embedded displays. The first target is the common 128x64 monochrome OLED, while display controllers, input devices, pages, widgets and platform integrations are designed as replaceable plugins.

## Everything is a plugin

- Display plugins: SSD1306, SH1106, simulators, future display controllers.
- Input plugins: GPIO buttons, encoders, keyboards and future input devices.
- Page plugins: dashboards, terminal, settings and application pages.
- Widget plugins: cards, metrics, progress bars, icons and custom components.
- Platform plugins: STM32, ESP32, Raspberry Pi, Ubuntu/Linux and Windows.

The portable core owns rendering, navigation and animation. Platform-specific code stays at the edge.

## Current features

- 128x64 1-bit Canvas with a 1024-byte framebuffer.
- Smooth 220 ms cubic ease-out page transitions without a second framebuffer.
- SSD1306 and SH1106 support.
- Callback transport for MCU integrations.
- Ubuntu 22.04 X11 simulator as the primary desktop development target.
- Native Win32 simulator.
- Raspberry Pi 5 dashboard for temperature, network, IP, user, uptime, disk, PMIC status and terminal output.

## Public API naming

- Project: `EmbedPluginUI`
- Headers: `epui/...`
- Namespace: `epui`
- CMake library: `epui`
- Build option/macro prefix: `EPUI_*`
- Simulator: `epui_sim`
- Raspberry Pi app: `epui_rpi`

The historical `openoledui` implementation remains available as an internal compatibility layer during the v0.1 migration.

## Ubuntu 22.04 development

```bash
sudo apt update
sudo apt install -y build-essential cmake libx11-dev

cmake -S . -B build
cmake --build build -j
./build/epui_sim
```

Controls: Left/A = previous page, Right/D = next page, Enter/Space = select, Esc = back.

## Raspberry Pi 5

```bash
cmake -S . -B build-pi -DEPUI_BUILD_RPI=ON
cmake --build build-pi -j4
./build-pi/epui_rpi /dev/i2c-1 0x3c ssd1306
```

## MCU integration

```cpp
#include <epui/callback_transport.hpp>
#include <epui/canvas.hpp>
#include <epui/oled.hpp>
#include <epui/page.hpp>

epui::CallbackTransport transport(ctx, write_fn, delay_fn);
epui::Oled128x64 oled(transport, epui::OledController::SSD1306);
epui::Canvas canvas;
epui::Ui ui;
```

The original STM32F429 OLED example remains under `Examples/0.96_inch_oled_f429` as a legacy hardware reference.
