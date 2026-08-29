# EmbedPluginUI

**A plugin-first embedded UI framework.**

EmbedPluginUI is a portable C++17 UI framework for small embedded displays. The first target is the common 128x64 monochrome OLED, while display controllers, input devices, pages, widgets and platform integrations are designed as replaceable plugins.

## Everything is a plugin

- Display plugins: SSD1306, SH1106, simulators, future display controllers.
- Input plugins: GPIO buttons, encoders, keyboards and future input devices.
- Page plugins: dashboards, terminal, settings and application pages.
- Widget plugins: cards, metrics, progress bars, icons and custom components.
- Platform plugins: STM32, ESP32, Raspberry Pi, Ubuntu/Linux and Windows.
- Debug plugins: FPS overlay today, with room for frame-time/RAM/transport diagnostics later.

The portable core owns rendering, navigation and animation. Platform-specific code stays at the edge.

## Current features

- 128x64 1-bit Canvas with a 1024-byte framebuffer.
- Damped-spring jelly page transitions with configurable stiffness and damping, without a second framebuffer.
- Arbitrary-depth static menu trees with jelly selection, scrolling and submenu motion.
- OLED-native `LiquidGlass` selection using liquid bridges, target lobes, moving edge highlights, dither trails and 1-pixel refraction rather than LCD-style blur.
- Fixed-capacity `UiOverlay` support and a heap-free FPS debug overlay plugin.
- SSD1306 and SH1106 support.
- Callback transport for MCU integrations.
- Dependency-aware, fixed-capacity plugin runtime.
- Typed sensors, services, event bus, themes, widgets and animations.
- GPIO button and quadrature encoder input plugins.
- STM32 HAL and ESP-IDF platform adapters.
- Ubuntu 22.04 X11 simulator as the primary desktop development target.
- Native Win32 simulator.
- Raspberry Pi 5 dashboard for temperature, network, IP, user, uptime, disk, PMIC status and terminal output.

## Naming

- Project: `EmbedPluginUI`
- Headers: `epui/...`
- Namespace: `epui`
- CMake library: `epui`
- Build option/macro prefix: `EPUI_*`
- Simulator: `epui_sim`
- Raspberry Pi app: `epui_rpi`

The implementation and public API use the same `epui` namespace and include tree.

## Simulator UI development

The Ubuntu and Windows simulators share the same UI pages under `examples/simulator_ui/`:

```text
examples/simulator_ui/
├── app.hpp / app.cpp
├── home_page.hpp / home_page.cpp
├── sensor_page.hpp / sensor_page.cpp
├── about_page.hpp / about_page.cpp
└── menu_demo.hpp / menu_demo.cpp
```

Edit the page files to develop the simulated OLED UI. `app.cpp` is the page composition point; `ports/linux/simulator.cpp` and `ports/windows/simulator.cpp` only implement the desktop window/input/display adapters.

Top-level left/right page changes use the same lightweight damped-spring motion language as the menu plugin. Developers can tune it globally through `epui::PageTransitionStyle` and `Ui::set_transition_style()`.

The demo exposes `Jelly Menu -> Display -> Theme -> Liquid Cursor` and `Jelly Menu -> System -> Debug -> FPS Overlay` for live runtime switching.

## Ubuntu 22.04 development

```bash
sudo apt update
sudo apt install -y build-essential cmake libx11-dev

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/epui_sim
```

The simulator window title is `EmbedPluginUI - 128x64 Simulator`.
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
