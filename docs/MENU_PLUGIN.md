# Multi-level menu plugin

`MenuPagePlugin` is EmbedPluginUI's static, heap-free hierarchical menu component. A menu tree can be nested to any application-defined depth; the only runtime limit is the template parameter used for the navigation stack.

```cpp
bool wifi = true;
int brightness = 70;

const epui::MenuItem display_items[] = {
    epui::MenuItem::value("Brightness", brightness, 0, 100, 5),
};
const epui::Menu display = epui::make_menu("Display", display_items);

const epui::MenuItem root_items[] = {
    epui::MenuItem::submenu("Display", display),
    epui::MenuItem::toggle("WiFi", wifi),
};
const epui::Menu root = epui::make_menu("Settings", root_items);

epui::Ui ui;
epui::MenuPagePlugin<16> settings(ui, root, "settings-menu");
settings.start();
```

## Item types

- `Action`: invokes a `void (*)(void*)` callback.
- `Submenu`: points to another static `Menu`.
- `Toggle`: edits a referenced `bool` and can invoke a change callback.
- `Value`: edits a referenced integer with min/max/step constraints and can invoke a change callback.
- `Choice`: cycles through a static string option table and stores the selected index in a referenced `uint8_t`.

Example choice item:

```cpp
std::uint8_t style_index = 0;
const char* const styles[] = {"Indicator", "Glide", "Slide"};

const epui::MenuItem items[] = {
    epui::MenuItem::choice("Cursor", style_index, styles),
};
```

No `std::vector`, `std::function`, runtime allocation or dynamic plugin loading is required.

## Input behavior

While the menu is focused:

- `Next`: next item, or increase a value while editing.
- `Prev`: previous item, or decrease a value while editing.
- `Select`: enter submenu, run action, toggle, cycle a choice, or enter/leave value editing.
- `Back`: leave value editing, go to the parent menu, or unfocus at the root.

When the root menu is unfocused, `Next/Prev` return to normal top-level `Ui` page navigation. `Select` focuses the menu again. This lets the same three-button or encoder input device drive both pages and arbitrary menu depth.

## Layout freedom

The framework owns the layout parameters; an application or demo chooses values that suit its display and font. Menu text and right-side values/arrows are positioned from the selection-frame edges:

```cpp
epui::MenuStyle style;
style.glass_x = 3;
style.glass_width = 122;
style.content_inset_left = 8;
style.content_inset_right = 8;
```

With equal insets, the frame has the same visual breathing room on both sides. Applications can deliberately make the values different. `text_x` and `right_margin` remain for source compatibility, but new projects should prefer `content_inset_left/right`.

Vertical spacing is also application-defined:

```cpp
style.row_height = 11;
style.glass_height = 9;
```

The Ubuntu demo uses an 11-pixel row pitch and a 9-pixel frame, leaving a compact 2-pixel gap. The framework does not force those values.

## Three selection animations

### Indicator

`Indicator` is the original left-side marker. Its selection and scrolling use the damped spring model and intentionally overshoot slightly before settling.

```cpp
menu.set_selection_style(epui::MenuSelectionStyle::Indicator);
```

Spring feel is configurable:

```cpp
style.spring_stiffness = 0.22f;
style.spring_damping = 0.35f;
```

### GlideFrame

`GlideFrame` is based on the classic small-U8g2-menu approach: each integer state moves quickly while far from its target, then approaches one pixel at a time near the target.

It animates three independent quantities:

- frame Y position;
- frame width, optionally fitted to the selected row content;
- long-menu scroll offset.

```cpp
style.selection_style = epui::MenuSelectionStyle::GlideFrame;
style.glide_fit_content = true;
style.glide_position_fast_step = 5;
style.glide_position_slow_zone = 4;
style.glide_width_fast_step = 10;
style.glide_width_slow_zone = 5;
style.glide_scroll_fast_step = 4;
style.glide_scroll_slow_zone = 4;
style.glide_tick_ms = 16;
```

There is no metaball, refraction, dither trail or geometry distortion in this renderer.

### SlideFrame

`SlideFrame` uses the same clean two-speed Y and long-menu scrolling as `GlideFrame`, but the rounded frame keeps its full configured width. This is useful when a consistent full-row selection box is visually preferable.

```cpp
menu.set_selection_style(epui::MenuSelectionStyle::SlideFrame);
```

`LiquidGlass` remains as a source-compatible alias of `GlideFrame` so older applications still compile, but new code should use the three names above.

## Smooth scrolling beyond one page

A menu can contain more items than `visible_rows`. Once selection moves beyond the visible window, frame styles animate the list scroll offset with the same two-speed logic instead of jumping one row at a time.

```cpp
style.visible_rows = 4;
style.row_height = 11;
style.glide_scroll_fast_step = 4;
style.glide_scroll_slow_zone = 4;
```

The Ubuntu simulator contains both a root menu longer than one page and a dedicated ten-item `Long Menu` for exercising this behavior.

## Ubuntu demo

The simulator exposes all three selection animations under:

```text
Jelly Menu -> Display -> Theme -> Cursor
```

Press `Select` on `Cursor` to cycle through `Indicator`, `Glide` and `Slide`. The demo defaults to `Glide`.
