# Graphics, Popup & Terminal / 图形、弹窗与终端

[中文](#中文) · [English](#english)

---

## 中文

## 1. 文本与 UTF-8 小型符号集

`Canvas::text()` 保持传统 `const char*` API，同时能解码一小组适合嵌入式状态界面的 UTF-8 符号：

```cpp
canvas.text(3, 12, u8"42°C");
canvas.text(3, 22, u8"80℃ ±2°");
canvas.text(3, 32, u8"5µA  10Ω  ←↑→↓  × ÷");
```

内置扩展字符包含：

```text
°  ℃  ±  µ  Ω  ← ↑ → ↓  ×  ÷
```

这不是完整 Unicode/中文字库。未知字符会显示为 `?`。如果需要中文，应另外加入中文字模/字体系统，而不是依赖当前 5×7 ASCII + extension table。

## 2. 基础图形

```cpp
canvas.pixel(...);
canvas.line(...);
canvas.rect(...);
canvas.fill_rect(...);
canvas.round_rect(...);
canvas.fill_round_rect(...);
canvas.circle(...);
canvas.fill_circle(...);
```

这些都是 1-bit framebuffer 上的整数光栅操作。

## 3. 曲线与数学图形

头文件：

```cpp
#include <epui/plot.hpp>
```

函数图：

```cpp
#include <cmath>

epui::draw_function_plot(
    canvas,
    {2, 18, 124, 36},
    {0.0f, 6.2831853f, -1.0f, 1.0f},
    [](float x) { return std::sin(x); }
);
```

还支持：

- `draw_parametric_plot()`：参数曲线；
- `draw_series()`：已有采样数组/传感器历史数据。

绘图 helper 不要求先申请 point buffer，也不要求 `std::function`。

## 4. `PopupPlugin`

```cpp
#include <epui/popup_plugin.hpp>

epui::PopupPlugin popup(ui);
registry.add(popup);

popup.show(
    u8"THERMAL",
    u8"CPU reached 80℃ ±2°",
    epui::PopupButtons::OkCancel,
    on_popup_result,
    user_context
);
```

回调：

```cpp
void on_popup_result(void* user, epui::PopupResult result);
```

输入：

```text
Left/Right 或 Prev/Next -> 改按钮
Select/Enter            -> 确认
Back/Escape             -> 取消
```

Popup 在 opening / visible / closing 阶段都是 modal input target，底层 Page 不会误收按键或字符。

动画：

- 从屏幕上方弹簧进入；
- 关闭时反向弹回上方；
- 中途反向时保留当前 velocity，避免跳变；
- velocity 会给 panel 加少量纵向 stretch；
- callback 在关闭动画完全结束后执行。

标题与正文复制进固定容量缓冲区，正文最多自动换成两行。

## 5. `TerminalView`

```cpp
#include <epui/terminal_view.hpp>

epui::TerminalView<64, 21> terminal;
terminal.feed(data, size);
terminal.scroll(1);      // 看更旧输出
terminal.scroll(-1);     // 回到更新输出
terminal.draw(canvas, 1, 15, 126, 42, now_ms);
```

模板参数：

```text
HistoryLines = 历史行数
Columns      = 每行列数
```

字符存储大致为：

```text
HistoryLines * (Columns + 1)
```

再加每行长度和少量 ring-buffer 状态。小 MCU 应显式降低模板容量。

### 支持的终端输入

- ASCII 可打印字符；
- `\n`；
- `\r`；
- `\b`；
- 硬换行；
- ESC / CSI / OSC 控制序列会被过滤，不画出来；
- parser 状态可跨多次 `feed()` 保持。

它不是 VT100 emulator：不支持完整光标寻址、alternate screen、颜色、全屏 ncurses 等。PTY 端建议 `TERM=dumb`。

### Cursor 与滚动

```cpp
terminal.set_cursor_visible(false); // 只读日志页
```

有新输出时 viewport 自动回到 live cursor。

## 6. `TerminalControls`

```cpp
#include <epui/terminal_controls.hpp>

epui::TerminalControls controls;
controls.cursor_left = epui::Key::Left;
controls.cursor_right = epui::Key::Right;
controls.history_previous = epui::Key::Up;
controls.history_next = epui::Key::Down;
controls.output_up = epui::Key::ScrollUp;
controls.output_down = epui::Key::ScrollDown;
```

默认语义：

```text
Enter        -> 聚焦 / 执行
Esc          -> 取消聚焦
Left/Right   -> 编辑命令行光标
Up/Down      -> 命令历史
Ctrl+Up/Down -> 输出 viewport（平台映射成 ScrollUp/ScrollDown）
```

## 7. `TerminalLineEditor`

固定容量命令编辑器支持：

- 插入；
- 删除；
- 光标移动；
- 历史记录；
- 连续重复命令抑制；
- 从历史回到最新时恢复原先未提交 draft。

Generic simulator 使用 8 条历史，Raspberry Pi 应用使用 16 条。

---

## English

## 1. Text and compact UTF-8 symbols

`Canvas::text()` decodes a deliberately small UTF-8 extension set while preserving the regular `const char*` API:

```cpp
canvas.text(3, 12, u8"42°C");
canvas.text(3, 22, u8"80℃ ±2°");
canvas.text(3, 32, u8"5µA  10Ω  ←↑→↓  × ÷");
```

The built-in extensions include degrees, Celsius, plus/minus, micro, ohm, arrows, multiplication, and division. This is not a complete Unicode or CJK font system; unknown codepoints become `?`.

## 2. Graphics primitives

The Canvas provides integer 1-bit raster operations for pixels, lines, rectangles, rounded rectangles, outline/filled circles, and text.

## 3. Mathematical plots

`epui/plot.hpp` provides allocation-free helpers for sampled function plots, parametric curves, and existing data series. Functions are sampled directly into the requested rectangle, so no intermediate point buffer or `std::function` is required.

## 4. `PopupPlugin`

`PopupPlugin` is a fixed-capacity modal overlay for alerts and confirmations. It supports OK and OK/Cancel layouts, captures input throughout opening/visible/closing states, and calls the result callback only after the closing animation has finished.

The panel enters from above the framebuffer with spring motion and retracts the same way. Reversing direction preserves velocity, and velocity adds a small capped jelly stretch.

## 5. `TerminalView`

```cpp
epui::TerminalView<64, 21> terminal;
terminal.feed(data, size);
terminal.draw(canvas, 1, 15, 126, 42, now_ms);
```

It is a fixed-capacity line-oriented ASCII terminal renderer. Printable ASCII, newline, carriage return, backspace, hard wrapping, and filtering of ESC/CSI/OSC sequences are supported. Parser state survives across feed chunks.

It is intentionally not a VT100 emulator. Full cursor addressing, alternate screens, colors, and full-screen TUI applications are out of scope; PTY integrations should advertise `TERM=dumb`.

## 6. Terminal controls and line editing

`TerminalControls` maps semantic framework keys to focus, execute, cursor, history, and output-scroll actions. `TerminalLineEditor<CommandCapacity, HistoryCapacity>` provides a fixed-capacity editable command line with history and draft restoration without dynamic allocation.
