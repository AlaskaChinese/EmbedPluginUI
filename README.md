# EmbedPluginUI

**A plugin-first embedded UI framework.**

EmbedPluginUI is a portable C++17 UI framework for small embedded displays. The first target is the common 128x64 monochrome OLED, while display controllers, input devices, pages, widgets and platform integrations are designed as replaceable plugins.

> **Documentation / 文档:** [GitHub Wiki](https://github.com/AlaskaChinese/EmbedPluginUI/wiki) — bilingual English/中文 usage guide, API manual, porting guide, diagnostics, Raspberry Pi 5, and licensing notes.
>
> **License:** source-available dual licensing. Permitted non-commercial use is available under [`LICENSE`](LICENSE); **commercial use requires a separate written license obtained before commercial use begins**. This project is not OSI Open Source. See [Commercial Licensing](COMMERCIAL-LICENSE.md).

## Everything is a plugin

- Display plugins: SSD1306, SH1106, simulators, future display controllers.
- Input plugins: GPIO buttons, encoders, keyboards and future input devices.
- Page plugins: dashboards, terminal, settings and application pages.
- Widget plugins: cards, metrics, progress bars, icons and custom components.
- Overlay plugins: modal popups, diagnostics and transient UI layers.
- Platform plugins: STM32, ESP32, Raspberry Pi, Ubuntu/Linux and Windows.
- Debug plugins: FPS, frame interval, render time, memory and display-transfer diagnostics.

The portable core owns rendering, navigation and animation. Platform-specific code stays at the edge.

## Current features

- 128x64 1-bit Canvas with a 1024-byte framebuffer.
- Damped-spring jelly page transitions with configurable stiffness and damping, without a second framebuffer.
- Arbitrary-depth static menu trees with smooth selection, scrolling and submenu motion.
- Four menu selection animations: spring `Indicator`, jelly `GlideFrame`, jelly full-width `SlideFrame`, and motion-highlight `LiquidGlass`.
- Pixel-safe sticky menu viewport with protected footer/header areas and partially clipped rows.
- 11-pixel rounded selection frames with application-configurable geometry and symmetric content insets.
- Smooth animated scrolling for menus longer than one visible page.
- `Choice` menu items for static enum-like settings without heap allocation.
- Fixed-capacity `UiOverlay` support and heap-free `DiagnosticsPlugin` for FPS/timing/memory/transfer metrics.
- Heap-free modal `PopupPlugin` with top-down spring entry, upward jelly exit and modal key/character routing.
- UTF-8 decoding for a compact embedded symbol set, including `°`, `℃`, `±`, `µ`, `Ω`, `×`, `÷` and arrows.
- Outline/filled circles plus allocation-free function, parametric and sampled series plotting helpers.
- SSD1306 and SH1106 support.
- Callback transport for MCU integrations.
- Dependency-aware, fixed-capacity plugin runtime.
- Typed sensors, services, event bus, themes, widgets and animations.
- GPIO button and quadrature encoder input plugins.
- Character input routing through `InputEvent` and `Page::on_char()`.
- Explicit `Up`/`Down`/`Left`/`Right` keyboard semantics while retaining `Next`/`Prev` for encoders and existing integrations.
- Configurable `TerminalControls` with conventional focus, cursor, execute, exit, history and output-scroll bindings.
- Fixed-capacity `TerminalView` with ASCII line history, ANSI sequence filtering, scrolling and a blinking cursor.
- STM32 HAL and ESP-IDF platform adapters.
- Ubuntu 22.04 X11 simulator as the primary desktop development target.
- Native Win32 simulator.
- Raspberry Pi 5 dashboard for temperature, network, IP, user, uptime, disk, PMIC status and terminal output.
- Raspberry Pi 5 headless console application with evdev keyboard input and an interactive PTY shell.

## Documentation

The canonical user-manual source lives under `docs/wiki/` and is automatically synchronized to the GitHub Wiki after changes land on `main`.

Recommended reading order:

1. [Quick Start](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Quick-Start)
2. [Core API](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Core-API)
3. [Navigation & Input](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Navigation-and-Input)
4. [Menu & Animation](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Menu-and-Animation)
5. [Plugin Runtime](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Plugin-Runtime)
6. [Graphics, Popup & Terminal](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Graphics-Popup-Terminal)
7. [Diagnostics](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Diagnostics)
8. [Porting & Hardware](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Porting-and-Hardware)
9. [Raspberry Pi 5](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Raspberry-Pi-5)
10. [Licensing](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Licensing)

Existing design notes under `docs/` remain useful implementation references.

## Repository layout

- `include/epui` and `src`: portable framework core.
- `ports`: reusable OS and hardware adapters.
- `apps`: complete runnable products built with the framework.
- `examples`: focused framework demonstrations.
- `tests`: headless core and composition tests.
- `docs/wiki`: version-controlled bilingual GitHub Wiki source.

The Raspberry Pi product lives in `apps/rpi5_oled_console`; reusable I2C, system-monitor, evdev, PTY and FIFO adapters remain under `ports/raspberry_pi`.

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
├── menu_demo.hpp / menu_demo.cpp
├── terminal_page.hpp / terminal_page.cpp
└── graphics_page.hpp / graphics_page.cpp
```

Edit the page files to develop the simulated OLED UI. `app.cpp` is the page composition point; `ports/linux/simulator.cpp` and `ports/windows/simulator.cpp` only implement the desktop window/input/display adapters.

Top-level left/right page changes use a lightweight damped-spring transition. Developers can tune it globally through `epui::PageTransitionStyle` and `Ui::set_transition_style()`.

The menu demo exposes:

```text
Jelly Menu -> Display -> Theme -> Cursor
```

Press `Select` to cycle among `Indicator`, `Glide`, `Slide`, and `Glass`. `Glide` and `Slide` keep deterministic pixel-stepped virtual paths while real spring-follow frames overshoot/rebound and share the same velocity-driven jelly deformation as `Glass`. `Glass` adds a motion-only sheen that disappears completely at rest. The root menu and `Long Menu` demonstrate pixel-safe sticky scrolling beyond one page.

Diagnostics are available under:

```text
Jelly Menu -> System -> Debug
├── Diag Overlay
└── Metric -> Summary / FPS / Timing / Memory / Transfer
```

The `Graphics` page demonstrates UTF-8 symbols, filled circles, animated sine and cosine plots, and the modal popup. Press Enter there to open the popup; Left/Right changes its selection and Enter confirms it.

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
Controls: Left/Right changes pages. In the menu, Enter focuses, Up/Down selects items, Right or Enter opens/activates, and Left or Esc returns. In a focused terminal, the top row is a local command editor, Left/Right moves its cursor, Up/Down selects command history, Ctrl+Up/Down scrolls output one line per key event, and Enter runs the command. Ctrl-C is sent to the foreground process.

To run the complete Raspberry Pi 5 application with X11 replacing the physical OLED and evdev keyboard:

```bash
cmake -S . -B build -DEPUI_BUILD_RPI_SIM=ON
cmake --build build -j
./build/epui_rpi_sim
```

`epui_sim` is the framework component gallery; `epui_rpi_sim` is the actual five-page Raspberry Pi application. Both reuse the same X11 display and input port.

## Raspberry Pi 5

```bash
cmake -S . -B build-pi -DEPUI_BUILD_RPI=ON
cmake --build build-pi -j4
./build-pi/epui_rpi /dev/i2c-1 0x3c ssd1306 /dev/input/event0 /bin/bash
```

Use a stable `/dev/input/by-id/...-event-kbd` path for deployment. The application uses only the I2C OLED and keyboard; no HDMI display or desktop session is required. Installation and systemd templates are documented in `apps/rpi5_oled_console/README.md`.

## MCU integration

The embedded core does not require Linux, X11, Win32, RTTI, dynamic allocation or dynamic plugin loading. Platform adapters expose callback-based hooks so MCU applications can bind their existing I2C/GPIO/delay functions without importing vendor headers into the portable core.

See the [Porting & Hardware Wiki page](https://github.com/AlaskaChinese/EmbedPluginUI/wiki/Porting-and-Hardware) for the recommended integration path.

## Licensing

EmbedPluginUI is **source-available**, not OSI Open Source.

- Personal study, hobby projects, education, academic/non-commercial research and other permitted non-commercial uses: see [`LICENSE`](LICENSE).
- Commercial use: obtain a separate written commercial license **before commercial use begins**. See [`COMMERCIAL-LICENSE.md`](COMMERCIAL-LICENSE.md).
- A later commercial license does not automatically retroactively authorize earlier unlicensed commercial use.
- Chinese explanatory translation: [`LICENSE.zh-CN.md`](LICENSE.zh-CN.md).

Commercial licensing: `alaskachinese@outlook.com`

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md). Because the project uses public non-commercial + commercial dual licensing, accepted external contributions need the inbound license described there so the project can continue offering both licensing paths.
