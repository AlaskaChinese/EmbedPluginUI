# Menu & Animation / 菜单与果冻动画

[中文](#中文) · [English](#english)

---

## 中文

`epui::MenuPagePlugin` 是固定容量、无堆分配的多级菜单组件，适合 128×64 OLED 上的设置页、系统菜单与设备配置。

### 1. 定义菜单树

```cpp
#include <epui/menu_plugin.hpp>

bool wifi_enabled = true;
int brightness = 72;
std::uint8_t cursor = 1;
const char* const cursor_options[] = {
    "Indicator", "Glide", "Slide", "Glass"
};

const epui::MenuItem display_items[] = {
    epui::MenuItem::value("Brightness", brightness, 0, 100, 5),
    epui::MenuItem::choice("Cursor", cursor, cursor_options),
};
const epui::Menu display_menu = epui::make_menu("Display", display_items);

const epui::MenuItem root_items[] = {
    epui::MenuItem::toggle("WiFi", wifi_enabled),
    epui::MenuItem::submenu("Display", display_menu),
};
const epui::Menu root_menu = epui::make_menu("Settings", root_items);
```

支持的项目类型：

```text
Action   -> 调用 callback
Submenu  -> 进入静态子菜单
Toggle   -> 修改 bool
Value    -> 修改整数范围值
Choice   -> 在静态字符串选项中循环
```

### 2. 创建菜单 Page Plugin

```cpp
epui::Ui ui;
epui::MenuStyle style;

epui::MenuPagePlugin<12> menu(
    ui,
    root_menu,
    "settings-menu",
    style,
    false  // auto_focus
);

menu.start();
```

`MaxDepth` 是运行时导航栈的编译期上限。菜单树本身由静态对象组成，不需要动态分配。

如果菜单是多个顶层 Page 之一，推荐 `auto_focus=false`：

```text
滑到 Menu 页面 -> Left/Right 仍翻 Page
Enter           -> 菜单聚焦
Back at root    -> 取消聚焦
```

### 3. 四种选框风格

```cpp
style.selection_style = epui::MenuSelectionStyle::Indicator;
style.selection_style = epui::MenuSelectionStyle::GlideFrame;
style.selection_style = epui::MenuSelectionStyle::SlideFrame;
style.selection_style = epui::MenuSelectionStyle::LiquidGlass;
```

#### Indicator

左侧短条，使用弹簧位置运动，适合最简洁的设置菜单。

#### GlideFrame

- 框宽可以根据内容自适应；
- U8g2 风格“两速像素路径”只作为虚拟 target；
- 实际选框由弹簧跟随该 target；
- 有真实 overshoot / rebound / settle；
- 同时用速度驱动轻微 squeeze/stretch。

#### SlideFrame

与 GlideFrame 共用运动模型，但保持整行宽的圆角框。

#### LiquidGlass

- 直接使用弹簧 selection/scroll 运动；
- 与 Glide/Slide 共用果冻形变；
- 额外具有运动时高光/反相 sheen；
- **静止后高光完全消失**，只保留干净圆角框。

### 4. 统一弹簧参数

```cpp
style.spring_stiffness = 0.22f;
style.spring_damping = 0.35f;
style.max_frame_ms = 48;
```

果冻形变：

```cpp
style.glass_max_stretch = 5;
style.glass_stretch_per_velocity = 0.35f;
style.glass_motion_threshold = 0.12f;
```

Glide 的虚拟路径：

```cpp
style.glide_position_fast_step = 5;
style.glide_position_slow_zone = 4;
style.glide_width_fast_step = 10;
style.glide_width_slow_zone = 5;
style.glide_scroll_fast_step = 4;
style.glide_scroll_slow_zone = 4;
style.glide_tick_ms = 16;
```

### 5. 圆角框布局

当前 Demo 常用：

```cpp
style.row_height = 13;
style.glass_height = 11;
style.glass_radius = 4;
style.content_inset_left = 8;
style.content_inset_right = 8;
```

5×7 文字在 11px 框中上下有留白，13px 行距保证相邻框之间有约 2px 空隙。

### 6. 长菜单 viewport 与安全区

```cpp
style.viewport_top = 13;
style.viewport_bottom = 60; // exclusive
style.allow_partial_rows = true;
```

128×64 Demo 中，菜单只完整占用 `y=13..59`，`y=60..63` 留给底部 Page 提示。

关键规则：

```text
选中框还能在 viewport 内移动
    -> 列表不动，只移动框

选中框将撞到底部安全边界
    -> 只把列表向上抬必要的像素
    -> 保证当前选项完整显示
    -> 顶部旧选项允许残缺

向上返回
    -> 只要框仍在舒适范围，先移动框
    -> 框碰到顶部边界后，列表才开始回落
```

`Canvas` 使用真实 clip rect，因此被裁掉的行会像被“横切一刀”，而不是整行突然消失。

Demo 几何下，第 4 行原本会侵入底部安全区，因此 scroll target 先从 0 变成 4px，而不是直接跳一整行；继续往下才是 17、30……

### 7. Submenu 动画

进入/退出子菜单使用横向面板弹簧动画，同一 framebuffer 上通过 origin/panel offset 合成，不需要第二块 1024-byte buffer。

### 8. Callback

Action、Toggle、Value、Choice 可绑定：

```cpp
void on_changed(void* user) {
    // apply setting
}
```

回调是裸函数指针 + `void*` user context，不依赖 `std::function`。

---

## English

`epui::MenuPagePlugin` is a fixed-capacity, heap-free hierarchical menu component for settings, configuration, and system menus on small OLEDs.

### 1. Define a static menu tree

```cpp
bool wifi_enabled = true;
int brightness = 72;
std::uint8_t cursor = 1;
const char* const cursor_options[] = {
    "Indicator", "Glide", "Slide", "Glass"
};

const epui::MenuItem display_items[] = {
    epui::MenuItem::value("Brightness", brightness, 0, 100, 5),
    epui::MenuItem::choice("Cursor", cursor, cursor_options),
};
const epui::Menu display_menu = epui::make_menu("Display", display_items);

const epui::MenuItem root_items[] = {
    epui::MenuItem::toggle("WiFi", wifi_enabled),
    epui::MenuItem::submenu("Display", display_menu),
};
const epui::Menu root_menu = epui::make_menu("Settings", root_items);
```

Item kinds are `Action`, `Submenu`, `Toggle`, `Value`, and `Choice`.

### 2. Create the menu page plugin

```cpp
epui::MenuPagePlugin<12> menu(
    ui, root_menu, "settings-menu", style, false
);
menu.start();
```

Use `auto_focus=false` when the menu is one of several top-level pages. The user then navigates to the page first, presses Enter/Select to focus it, and presses Back at the root to return control to page navigation.

### 3. Selection styles

`Indicator` is the original spring-driven left marker.

`GlideFrame` uses a deterministic two-speed pixel path as a moving virtual target. A real damped spring follows that target, giving position overshoot/rebound while retaining predictable OLED motion. Its width can fit the selected content.

`SlideFrame` uses the same motion model but keeps a full-width rounded frame.

`LiquidGlass` uses direct spring selection/scroll motion, shares the same velocity-driven jelly geometry, and adds motion-only inverse sheen/highlight. The highlight disappears completely at rest.

### 4. Motion tuning

```cpp
style.spring_stiffness = 0.22f;
style.spring_damping = 0.35f;
style.glass_max_stretch = 5;
style.glass_stretch_per_velocity = 0.35f;
style.glide_tick_ms = 16;
```

### 5. Pixel viewport and sticky long-menu scrolling

```cpp
style.viewport_top = 13;
style.viewport_bottom = 60; // exclusive
style.allow_partial_rows = true;
```

The selected resting frame must remain fully inside the safe viewport. The list scrolls only when the frame would cross an edge. Older rows are allowed to become partially pixel-clipped, which visually hints that more content exists beyond the boundary.

Moving upward is frame-first: if the selected frame can move upward inside the current viewport, the list remains fixed. Only when the frame reaches the top comfort edge does the list scroll down again.

### 6. Submenus and callbacks

Submenus slide horizontally with spring motion while sharing the same framebuffer. Actions/settings use plain callback functions plus `void*` user context; `std::function` is not required.
