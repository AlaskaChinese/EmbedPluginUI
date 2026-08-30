# Navigation & Input / 页面、输入与导航

[中文](#中文) · [English](#english)

---

## 中文

### `Key` 语义

当前核心定义的语义键：

```cpp
enum class Key : std::uint8_t {
    Next,
    Prev,
    Select,
    Back,
    ScrollUp,
    ScrollDown,
    Up,
    Down,
    Left,
    Right,
};
```

推荐约定：

- 键盘/方向键：`Up/Down/Left/Right`；
- 旋转编码器：`Next/Prev`；
- Enter/确认键：`Select`；
- Esc/返回键：`Back`；
- 终端输出滚动：`ScrollUp/ScrollDown`。

### `InputEvent`

输入插件应把平台键码转换成框架语义事件，再交给 `Ui`：

```cpp
epui::InputEvent event{};
while (input.poll(event)) {
    ui.handle(event, now_ms);
}
```

字符输入通过 `InputEvent::ch` 传递；普通按键事件把 `ch` 保持为 0。每次 `poll()` 返回前应完整覆盖事件内容，不依赖上一次残留值。

### `Ui` 的输入优先级

```text
InputEvent
   |
   v
Top modal UiOverlay?
   | yes -> overlay on_key/on_char
   |
   no
   v
Current stable Page
   |
   +-> captures_key(key) ? page handles key
   |
   +-> otherwise Ui may use Left/Right/Next/Prev for page navigation
```

字符输入不会通过 `captures_key()` 判断；无模态 overlay 时直接发送到当前稳定页面的 `Page::on_char()`。接受文字的页面应自己维护 focus 状态。

### 顶层 Page 导航

最常见的交互是：

```text
Home <-> Sensors <-> Graphics <-> Menu
```

当当前页面不捕获 Left/Right 或 Next/Prev 时，`Ui` 使用它们切换顶层 Page，并通过共享弹簧动画移动画面。

### 页面内部 focus

一个页面如果需要“进入后才接管输入”，推荐使用显式 focus：

```cpp
bool captures_key(epui::Key key) const override {
    if (!focused_) return key == epui::Key::Select;
    return true;
}

void on_key(epui::Key key) override {
    if (!focused_ && key == epui::Key::Select) {
        focused_ = true;
        return;
    }
    if (focused_ && key == epui::Key::Back) {
        focused_ = false;
        return;
    }
}
```

这也是 Demo 菜单/终端采用的交互原则：

```text
滑到页面 -> 尚未聚焦 -> Left/Right 继续翻页
Enter     -> 聚焦页面内部交互
Esc/Back  -> 退出焦点 -> Left/Right 恢复翻页
```

### 输入插件

#### GPIO 按钮

`GpioButtonPlugin` 通过平台无关的 pin-read callback 读取按钮，负责 debounce，并输出固定容量 `InputEvent` 队列。

适合：

- `Prev / Select / Next` 三键；
- 四方向 + OK；
- 板载按键。

#### 旋转编码器

`EncoderInputPlugin` 解码正交编码器，默认很适合输出：

```text
顺时针 -> Next
逆时针 -> Prev
按压   -> Select（由额外按钮插件提供）
```

### 菜单输入约定

菜单对键盘和编码器同时兼容：

```text
未聚焦：Enter/Select 聚焦
聚焦后：
  Up/Down 或 Prev/Next -> 选择项目
  Right/Enter/Select   -> 进入/执行
  Left/Esc/Back        -> 返回
根菜单 Back            -> 退出菜单焦点
```

具体见 [Menu & Animation](Menu-and-Animation)。

### 终端输入约定

终端使用 `TerminalControls` 将按键映射到逻辑动作：

- Enter：聚焦/执行；
- Esc：退出焦点；
- Left/Right：移动本地命令编辑器光标；
- Up/Down：命令历史；
- `ScrollUp/ScrollDown`：滚动输出；
- Raspberry Pi / desktop 默认用 Ctrl+Up/Down 产生输出滚动动作。

具体见 [Graphics, Popup & Terminal](Graphics-Popup-Terminal)。

---

## English

### Semantic keys

The core defines platform-independent key semantics:

```cpp
enum class Key : std::uint8_t {
    Next, Prev, Select, Back,
    ScrollUp, ScrollDown,
    Up, Down, Left, Right,
};
```

Recommended mapping:

- keyboards/d-pads -> `Up/Down/Left/Right`;
- rotary encoders -> `Next/Prev`;
- Enter/OK -> `Select`;
- Escape/Back -> `Back`;
- explicit terminal viewport movement -> `ScrollUp/ScrollDown`.

### `InputEvent`

Platform input plugins translate native events into `InputEvent` before the event reaches `Ui`:

```cpp
epui::InputEvent event{};
while (input.poll(event)) {
    ui.handle(event, now_ms);
}
```

Text-capable inputs set `InputEvent::ch`; pure key events keep it zero. Always overwrite the whole event before returning from `poll()`.

### Routing priority

```text
InputEvent
   |
   v
Top modal UiOverlay?
   | yes -> overlay on_key/on_char
   |
   no
   v
Current stable Page
   |
   +-> captures_key(key) ? page handles it
   |
   +-> otherwise Ui may use navigation keys for top-level page changes
```

Character input does not use `captures_key()`. With no modal overlay it goes to the stable page through `Page::on_char()`; text pages should gate that callback with their own focus state.

### Top-level page navigation

Typical application layout:

```text
Home <-> Sensors <-> Graphics <-> Menu
```

If the current page does not capture Left/Right or Next/Prev, `Ui` uses those keys to change the top-level page with the configured spring transition.

### Explicit page focus

Interactive pages should not automatically steal page-navigation keys unless that behavior is intentional. A good pattern is:

```text
arrive on page -> unfocused -> Left/Right still changes pages
Enter          -> focused -> page owns its controls
Back/Escape    -> unfocused -> page navigation returns
```

The simulator menu and terminal pages follow this model.

### Input plugins

`GpioButtonPlugin` wraps a platform-independent pin-read callback, debounces buttons, and emits events through a fixed-capacity queue.

`EncoderInputPlugin` decodes quadrature and naturally maps clockwise/counter-clockwise steps to `Next`/`Prev`.

### Menu convention

Menus support both directional keyboards and encoders:

```text
unfocused: Enter/Select -> focus
focused:
  Up/Down or Prev/Next -> select item
  Right/Enter/Select   -> enter/activate
  Left/Escape/Back     -> return
Back at root           -> unfocus menu
```

See [Menu & Animation](Menu-and-Animation).

### Terminal convention

`TerminalControls` maps semantic keys to terminal actions. Defaults use Enter to focus/execute, Escape to unfocus, Left/Right for the local input cursor, Up/Down for command history, and `ScrollUp/ScrollDown` for output movement. Desktop and Raspberry Pi keyboard ports generate the scroll actions from Ctrl+Up/Down.
