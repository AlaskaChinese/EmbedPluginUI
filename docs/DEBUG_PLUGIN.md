# Diagnostics debug plugin

`DiagnosticsPlugin` is a heap-free `UiOverlay` debug plugin for runtime performance diagnostics. It keeps the original FPS functionality and adds frame cadence, render timing, memory, and display/transport measurements.

The original names remain source-compatible:

```cpp
using FpsDebugPlugin = DiagnosticsPlugin;
using FpsDebugStyle = DiagnosticsStyle;
```

New code should include `epui/diagnostics_plugin.hpp` directly.

## Metrics

The plugin can track:

- FPS over a configurable sampling window.
- average frame interval (`frame_time_ms`).
- latest UI render duration (`record_render_time_us`).
- memory used/total bytes from direct values or a platform memory probe.
- latest transfer/present duration, logical bytes, derived bytes/second, transfer count, and failure count.

No dynamic allocation, RTTI, `std::function`, or background thread is required.

## Basic use

```cpp
epui::Ui ui;
epui::DiagnosticsPlugin diagnostics(ui);
diagnostics.start();

// Ui::render() automatically counts frames while auto_sample is enabled.
ui.render(canvas, now_ms);
```

## Render and transfer instrumentation

The framework deliberately does not assume a timer API. Platforms measure around their own calls and feed the result to the plugin:

```cpp
const auto render_begin_us = micros();
ui.render(canvas, now_ms);
diagnostics.record_render_time_us(micros() - render_begin_us);

const auto tx_begin_us = micros();
const bool ok = display.present(canvas);
diagnostics.record_transfer(micros() - tx_begin_us,
                            epui::Canvas::BufferSize,
                            ok);
```

For a physical OLED, `bytes` can be the actual framebuffer/display payload. For the desktop simulator it is the logical 1024-byte framebuffer while the duration measures the backend present call.

Available transfer getters include:

```cpp
diagnostics.transfer_time_us();
diagnostics.transfer_bytes();
diagnostics.transfer_rate_bps();
diagnostics.transfer_count();
diagnostics.transfer_failures();
diagnostics.last_transfer_ok();
```

## Memory sources

Applications can push memory values directly:

```cpp
diagnostics.set_memory_bytes(used_bytes, total_bytes);
```

or install a zero-allocation platform callback:

```cpp
bool read_memory(void*, epui::DebugMemoryStats& out) {
    out.used_bytes = current_used_sram();
    out.total_bytes = total_sram();
    return true;
}

diagnostics.set_memory_probe(read_memory);
```

The probe is refreshed when an FPS sampling window closes and can also be refreshed explicitly with `refresh_memory()`.

Typical platform mappings are:

- STM32: SRAM usage or RTOS heap statistics.
- ESP-IDF: heap capability/free-heap APIs.
- FreeRTOS: heap usage/high-water information supplied by the application.
- Linux/Windows simulator: process resident/working-set memory.

## Overlay views

A single compact line can be switched at runtime:

```cpp
diagnostics.set_view(epui::DebugMetricView::Summary);
diagnostics.set_view(epui::DebugMetricView::Fps);
diagnostics.set_view(epui::DebugMetricView::Timing);
diagnostics.set_view(epui::DebugMetricView::Memory);
diagnostics.set_view(epui::DebugMetricView::Transfer);
```

Typical output is intentionally compact for 128x64 monochrome OLEDs:

```text
F60.0 16.7ms
FPS 60.0
R0.4 T1.2ms
MEM 23.4M
TX 1.2ms 1.0K
```

A failed last transfer uses the `TX!` prefix.

## Presented FPS instead of render FPS

By default the overlay counts `Ui::render()` calls. To define FPS by successful physical presents instead:

```cpp
diagnostics.set_auto_sample(false);

ui.render(canvas, now_ms);
if (display.present(canvas)) {
    diagnostics.mark_frame(now_ms);
}
```

## Demo integration

The desktop simulator measures real render and backend-present durations and installs a process-memory probe. The menu exposes:

```text
Jelly Menu
└── System
    └── Debug
        ├── Diag Overlay  ON/OFF
        ├── Metric        Summary/FPS/Timing/Memory/Transfer
        └── Verbose       ON/OFF
```

The demo menu page itself starts unfocused. Left/right therefore continue switching top-level pages until Enter/Select explicitly focuses the menu; Back at the root releases focus again.

## Styling

```cpp
epui::DiagnosticsStyle style;
style.x = -1;                 // negative = right aligned
style.y = 54;
style.background = true;
style.padding = 1;
style.sample_window_ms = 500;
style.view = epui::DebugMetricView::Summary;

epui::DiagnosticsPlugin diagnostics(ui, "diagnostics", style);
```
