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
- Arbitrary-depth static menu trees with smooth selection, scrolling and submenu motion.
- Four menu selection animations: spring `Indicator`, jelly `GlideFrame`, jelly full-width `SlideFrame`, and motion-highlight `LiquidGlass`.
- 11-pixel rounded selection frames with application-configurable geometry and symmetric content insets.
- Smooth animated scrolling for menus longer than one visible page.
- `Choice` menu items for static enum-like settings without heap allocation.
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

Top-level left/right page changes use a lightweight damped-spring transition. Developers can tune it globally through `epui::PageTransitionStyle` and `Ui::set_transition_style()`.

The menu demo exposes:

```text
Jelly Menu -> Display -> Theme -> Cursor
```

Press `Select` to cycle among `Indicator`, `Glide`, `Slide`, and `Glass`. `Glide` and `Slide` retain clean pixel-stepped paths while their frames briefly lag/stretch and settle like jelly. `Glass` restores the first-generation spring/stretch cursor, but its sheen is visible only while moving and disappears completely at rest. The root menu and `Long Menu` demonstrate smooth scrolling beyond one page. FPS can be toggled under `Jelly Menu -> System -> Debug -> FPS Overlay`.

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

The embedded core does not require Linux, X11, Win32, RTTI, dynamic allocation or dynamic plugin loading. Platform adapters expose callback-based hooks so MCU applications can bind their existing I2C/GPIO/delay functions without importing vendor headers into the portable core.

See `docs/ARCHITECTURE.md` and `docs/MENU_PLUGIN.md` for the plugin and menu design.
