# Diagnostics / 性能诊断

[中文](#中文) · [English](#english)

---

## 中文

`DiagnosticsPlugin` 是一个 heap-free `UiOverlay` 调试插件，用于实时观察 UI 帧率、帧间隔、渲染耗时、内存和显示传输。

```cpp
#include <epui/diagnostics_plugin.hpp>

epui::DiagnosticsPlugin diagnostics(ui);
diagnostics.start();
```

### 1. 诊断视图

```cpp
epui::DebugMetricView::Summary
epui::DebugMetricView::Fps
epui::DebugMetricView::Timing
epui::DebugMetricView::Memory
epui::DebugMetricView::Transfer
```

切换：

```cpp
diagnostics.set_view(epui::DebugMetricView::Timing);
```

### 2. FPS 与 Frame Time

默认情况下 Overlay 每次随 `Ui::render()` 自动调用 `mark_frame(now_ms)`，按 sample window 统计：

```cpp
diagnostics.fps();
diagnostics.fps_x10();
diagnostics.frame_time_ms();
diagnostics.frame_ms_x10();
```

例如：

```text
FPS 60.0
Frame ~16.7 ms
```

如果你更关心“成功送到物理屏幕的 FPS”，可以：

```cpp
diagnostics.set_auto_sample(false);

ui.render(canvas, now_ms);
if (display.present(canvas)) {
    diagnostics.mark_frame(now_ms);
}
```

### 3. Render Time

测量 `ui.render()` 自身 CPU 时间：

```cpp
const auto t0 = micros();
ui.render(canvas, now_ms);
const auto t1 = micros();

diagnostics.record_render_time_us(t1 - t0);
```

读取：

```cpp
diagnostics.render_time_us();
diagnostics.render_time_ms();
```

这能区分：

```text
总 Frame Time
├── UI Render Time
├── Display Transfer Time
└── app/sleep/other
```

### 4. Memory

直接上报：

```cpp
diagnostics.set_memory_bytes(used_bytes, total_bytes);
```

或注册平台 probe：

```cpp
bool probe_memory(void* user, epui::DebugMemoryStats& out) {
    out.used_bytes = ...;
    out.total_bytes = ...;
    return true;
}

diagnostics.set_memory_probe(probe_memory, context);
```

适配示例：

```text
Linux / Raspberry Pi -> process RSS 或系统数据
Windows              -> Working Set
STM32 + FreeRTOS      -> heap / SRAM 统计
ESP32                 -> ESP-IDF heap APIs
Bare metal MCU        -> 应用自己维护的静态/堆使用量
```

读取：

```cpp
diagnostics.memory_valid();
diagnostics.memory_used_bytes();
diagnostics.memory_total_bytes();
```

### 5. Display / Transport

每次物理 present 或传输完成后：

```cpp
const auto t0 = micros();
const bool ok = oled.present(canvas.data(), epui::Canvas::BufferSize);
const auto t1 = micros();

diagnostics.record_transfer(
    t1 - t0,
    epui::Canvas::BufferSize,
    ok
);
```

可读取：

```cpp
diagnostics.transfer_time_us();
diagnostics.transfer_time_ms();
diagnostics.transfer_bytes();
diagnostics.transfer_rate_bps();
diagnostics.transfer_count();
diagnostics.transfer_failures();
diagnostics.last_transfer_ok();
```

适合比较：

- I2C 100/400k/1MHz；
- SPI；
- full framebuffer vs dirty/partial update；
- 不同控制器；
- 单次失败与长期失败率。

### 6. Overlay 样式

```cpp
epui::DiagnosticsStyle style;
style.x = -1; // 右对齐
style.y = 54;
style.background = true;
style.padding = 1;
style.sample_window_ms = 500;
style.view = epui::DebugMetricView::Summary;

epui::DiagnosticsPlugin diagnostics(ui, "diag", style);
```

运行时：

```cpp
diagnostics.set_visible(false);
diagnostics.set_visible(true);
```

隐藏 overlay 时仍可继续采集数据。

### 7. 兼容旧 FPS API

旧代码：

```cpp
#include <epui/fps_debug_plugin.hpp>
epui::FpsDebugPlugin fps(ui);
```

仍然可以继续使用；它保留为兼容别名/入口。新代码推荐直接使用 `DiagnosticsPlugin`。

### 8. 调试建议

遇到动画卡顿时按这个顺序看：

1. FPS / frame time 是否波动；
2. render time 是否升高；
3. transfer time 是否逼近 frame budget；
4. I2C/SPI 是否出现失败；
5. memory 是否持续增长；
6. 平台主循环是否存在不必要的 sleep/blocking IO。

---

## English

`DiagnosticsPlugin` is a heap-free `UiOverlay` for runtime performance visibility.

### Views

```cpp
Summary
Fps
Timing
Memory
Transfer
```

### FPS and frame interval

With automatic sampling enabled, the overlay counts `Ui::render()` cadence over the configured sample window. Use `fps()` and `frame_time_ms()` to inspect frame rate and average frame interval.

To measure successful physical presents instead, disable automatic sampling and call `mark_frame()` only after a successful display present.

### Render time

Instrument `ui.render()` with the platform's microsecond timer and call `record_render_time_us()`. This separates UI CPU time from display transfer and other application work.

### Memory

Call `set_memory_bytes()` directly, or register a `DebugMemoryProbe` callback. The portable core does not assume a particular allocator or OS, so Linux RSS, Windows Working Set, FreeRTOS heap, ESP-IDF heap, or custom bare-metal accounting can all feed the same interface.

### Display transfer

Call `record_transfer(elapsed_us, bytes, success)` after a framebuffer transfer. The plugin exposes last transfer duration, bytes, derived bytes/second, total transfer count, failure count, and last success state.

This is useful for comparing I2C/SPI speeds, full vs partial updates, controller implementations, and real display bottlenecks.

### Styling and compatibility

`DiagnosticsStyle` configures position, background, padding, sample window, and active view. `FpsDebugPlugin` remains available as a source-compatible legacy entry point; new code should prefer `DiagnosticsPlugin`.
