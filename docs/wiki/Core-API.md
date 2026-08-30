# Core API / 核心 API

[中文](#中文) · [English](#english)

---

## 中文

### 核心对象关系

```text
InputEvent
    |
    v
   Ui --------------------> UiOverlay
    |
    +---- Page / PagePlugin
    |
    v
 Canvas (128x64, 1-bit, 1024 B)
    |
    v
DisplayPlugin / Oled128x64
    |
    v
OledTransport / platform adapter
```

### `Canvas`

头文件：

```cpp
#include <epui/canvas.hpp>
```

`Canvas` 是所有 UI 绘制的共同目标。当前固定为 128×64、1-bit framebuffer：

```cpp
static constexpr int Width = 128;
static constexpr int Height = 64;
static constexpr std::size_t BufferSize = 1024;
```

常用绘制接口：

```cpp
canvas.clear();
canvas.pixel(x, y);
canvas.line(x0, y0, x1, y1);
canvas.rect(x, y, w, h);
canvas.fill_rect(x, y, w, h);
canvas.round_rect(x, y, w, h, radius);
canvas.fill_round_rect(x, y, w, h, radius);
canvas.circle(cx, cy, radius);
canvas.fill_circle(cx, cy, radius);
canvas.text(x, y, "HELLO");
canvas.text(x, y, u8"42°C ±2°");
```

坐标变换与裁剪：

```cpp
canvas.set_origin(dx, dy);
canvas.reset_origin();
canvas.set_clip_rect(x, y, w, h);
canvas.reset_clip();
```

`set_origin()` 用于页面滑动/子面板组合，`set_clip_rect()` 用于菜单 viewport、终端窗口等局部裁剪。

Framebuffer：

```cpp
const std::uint8_t* data = canvas.data();
```

### `Page`

```cpp
#include <epui/page.hpp>

class MyPage : public epui::Page {
public:
    void draw(epui::Canvas& canvas, std::uint32_t now_ms) override;
    void on_key(epui::Key key) override;
    void on_char(char ch) override;
    bool captures_key(epui::Key key) const override;
};
```

页面对象由应用持有，`Ui` 只保存引用/指针，不接管对象生命周期。

### `Ui`

`Ui` 管理：

- 最多 8 个顶层页面；
- 最多 4 个 overlay；
- 输入路由；
- 顶层页面切换；
- 弹簧过渡；
- 页面圆点；
- overlay 绘制。

```cpp
epui::Ui ui;
ui.add_page(page_a);
ui.add_page(page_b);
ui.handle(event, now_ms);
ui.render(canvas, now_ms);
```

页面切换动画：

```cpp
epui::PageTransitionStyle motion;
motion.spring_stiffness = 0.30f;
motion.spring_damping = 0.55f;
ui.set_transition_style(motion);
```

### `UiOverlay`

Overlay 用于跨页面 UI，例如：

- Popup；
- Diagnostics；
- Toast；
- 未来的全局状态层。

```cpp
class Overlay : public epui::UiOverlay {
public:
    void draw_overlay(epui::Canvas&, std::uint32_t) override;
    bool captures_input() const override { return modal_; }
    void on_key(epui::Key) override;
    void on_char(char) override;
};
```

最上层且 `captures_input()==true` 的 overlay 优先获得输入。

### `Oled128x64`

```cpp
#include <epui/oled.hpp>

epui::Oled128x64 display(
    transport,
    epui::OledController::SSD1306
);

display.init();
display.set_contrast(0xCF);
display.present(canvas.data(), epui::Canvas::BufferSize);
display.power(false);
```

当前控制器：

```cpp
epui::OledController::SSD1306
epui::OledController::SH1106
```

### 公共模块速查

| 模块 | 头文件 | 用途 |
|---|---|---|
| Canvas | `epui/canvas.hpp` | framebuffer 与基础绘图 |
| Ui/Page | `epui/page.hpp` | 页面、输入、过渡、overlay |
| OLED | `epui/oled.hpp` | SSD1306/SH1106 |
| Display plugin | `epui/display_plugin.hpp` | 显示插件边界 |
| Input | `epui/input_plugin.hpp` | 输入事件与插件 |
| Menu | `epui/menu_plugin.hpp` | 多级菜单 |
| Popup | `epui/popup_plugin.hpp` | 模态弹窗 |
| Terminal | `epui/terminal_view.hpp` | 固定容量终端视图 |
| Diagnostics | `epui/diagnostics_plugin.hpp` | FPS/时间/内存/传输诊断 |
| Plot | `epui/plot.hpp` | 函数/参数/序列绘图 |
| Plugin registry | `epui/plugin_registry.hpp` | 插件依赖与生命周期 |
| Event bus | `epui/event_bus.hpp` | 同步类型消息 |
| Sensor | `epui/sensor_plugin.hpp` | 周期类型快照 |
| Service | `epui/service_plugin.hpp` | 后台/周期服务 |
| Animation | `epui/animation_plugin.hpp` | 固定容量 tween |
| Theme | `epui/theme_plugin.hpp` | 主题参数 |
| Widgets | `epui/widget_plugin.hpp` | 组件组合 |

---

## English

### Core object model

```text
InputEvent
    |
    v
   Ui --------------------> UiOverlay
    |
    +---- Page / PagePlugin
    |
    v
 Canvas (128x64, 1-bit, 1024 B)
    |
    v
DisplayPlugin / Oled128x64
    |
    v
OledTransport / platform adapter
```

### `Canvas`

```cpp
#include <epui/canvas.hpp>
```

`Canvas` is the common drawing target. The current geometry is a fixed 128×64 1-bit framebuffer using exactly 1024 bytes.

Common primitives:

```cpp
canvas.clear();
canvas.pixel(x, y);
canvas.line(x0, y0, x1, y1);
canvas.rect(x, y, w, h);
canvas.fill_rect(x, y, w, h);
canvas.round_rect(x, y, w, h, radius);
canvas.fill_round_rect(x, y, w, h, radius);
canvas.circle(cx, cy, radius);
canvas.fill_circle(cx, cy, radius);
canvas.text(x, y, "HELLO");
canvas.text(x, y, u8"42°C ±2°");
```

Composition and clipping:

```cpp
canvas.set_origin(dx, dy);
canvas.reset_origin();
canvas.set_clip_rect(x, y, w, h);
canvas.reset_clip();
```

Use origins for translated pages/panels and clipping for scrollable viewports such as menus and terminals.

### `Page`

```cpp
class MyPage : public epui::Page {
public:
    void draw(epui::Canvas& canvas, std::uint32_t now_ms) override;
    void on_key(epui::Key key) override;
    void on_char(char ch) override;
    bool captures_key(epui::Key key) const override;
};
```

Pages are application-owned; `Ui` stores references and does not own their lifetime.

### `Ui`

`Ui` owns navigation state, input routing, spring page transitions, page dots, and overlay composition. Capacity is currently eight pages and four overlays.

```cpp
epui::Ui ui;
ui.add_page(page_a);
ui.add_page(page_b);
ui.handle(event, now_ms);
ui.render(canvas, now_ms);
```

Page motion can be tuned with `PageTransitionStyle` / `SpringStyle` and `set_transition_style()`:

```cpp
epui::PageTransitionStyle motion;
motion.spring_stiffness = 0.30f;
motion.spring_damping = 0.55f;
ui.set_transition_style(motion);
```

### `UiOverlay`

Overlays are appropriate for popups, diagnostics, toasts, and global UI. The topmost overlay returning `captures_input()==true` receives key and character input before the page underneath.

### `Oled128x64`

```cpp
epui::Oled128x64 display(transport, epui::OledController::SSD1306);
display.init();
display.set_contrast(0xCF);
display.present(canvas.data(), epui::Canvas::BufferSize);
display.power(false);
```

Supported controller modes are `SSD1306` and `SH1106`.

### Public module map

| Module | Header | Purpose |
|---|---|---|
| Canvas | `epui/canvas.hpp` | framebuffer and graphics |
| Ui/Page | `epui/page.hpp` | pages, routing, transitions, overlays |
| OLED | `epui/oled.hpp` | SSD1306/SH1106 driver |
| Display plugin | `epui/display_plugin.hpp` | display plugin boundary |
| Input | `epui/input_plugin.hpp` | input events/plugins |
| Menu | `epui/menu_plugin.hpp` | hierarchical menus |
| Popup | `epui/popup_plugin.hpp` | modal popup overlay |
| Terminal | `epui/terminal_view.hpp` | fixed-capacity terminal view |
| Diagnostics | `epui/diagnostics_plugin.hpp` | FPS/timing/memory/transfer data |
| Plot | `epui/plot.hpp` | function/parametric/series plotting |
| Plugin registry | `epui/plugin_registry.hpp` | dependency/lifecycle runtime |
| Event bus | `epui/event_bus.hpp` | synchronous typed messages |
| Sensor | `epui/sensor_plugin.hpp` | periodic typed snapshots |
| Service | `epui/service_plugin.hpp` | background/periodic work |
| Animation | `epui/animation_plugin.hpp` | fixed-capacity tween tracks |
| Theme | `epui/theme_plugin.hpp` | compact theme values |
| Widgets | `epui/widget_plugin.hpp` | widget composition |
