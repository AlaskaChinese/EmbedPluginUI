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

```cpp
std::uint8_t style_index = 0;
const char* const styles[] = {"Indicator", "Glide", "Slide", "Glass"};

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

When the root menu is unfocused, `Next/Prev` return to normal top-level `Ui` page navigation. `Select` focuses the menu again.

## Layout freedom

Text and right-side values/arrows are positioned from the selection-frame edges:

```cpp
epui::MenuStyle style;
style.glass_x = 3;
style.glass_width = 122;
style.content_inset_left = 8;
style.content_inset_right = 8;
```

Equal insets produce equal visual breathing room on both sides. `text_x` and `right_margin` remain for source compatibility, but new projects should prefer `content_inset_left/right`.

The rounded-frame resting height is 11 pixels. Vertical spacing remains application-defined:

```cpp
style.glass_height = 11;
style.row_height = 13;
```

The Ubuntu demo uses a 13-pixel row pitch so neighboring 11-pixel frames retain a clean 2-pixel gap.

## Pixel viewport and safe zones

Menu body rendering has its own configurable pixel viewport:

```cpp
style.viewport_top = 13;
style.viewport_bottom = 60;  // exclusive
style.allow_partial_rows = true;
```

`viewport_bottom` is exclusive. On the 128x64 demo, the menu can therefore draw only in `y = 13..59`; `Ui` draws its page navigation at `y = 61..62`, so `y = 60..63` becomes a protected bottom band.

The viewport uses the reusable `Canvas::set_clip_rect()` primitive. Text, rounded frames and `LiquidGlass` inverse sheen are all clipped at the pixel level. A row crossing the boundary is not discarded: with `allow_partial_rows = true`, only the pixels inside the viewport remain visible. This creates the intended "horizontal cut" hint that more content exists beyond the edge.

For the demo geometry:

```text
content_top      = 16
row_height       = 13
glass_height     = 11
viewport_bottom  = 60
```

selecting the fourth row would normally put its text at `y = 55` and its 11-pixel frame at roughly `y = 53..63`. The viewport follower therefore raises the list by exactly 4 pixels. The selected frame settles at `y = 49..59`, fully visible above the safe zone, while the first row moves to `y = 12` and is partially clipped by the `y = 13` top boundary.

Applications are free to change these values for another OLED geometry or for a different footer/header layout.

## Four selection animations

### Indicator

`Indicator` is the original left-side marker. Its selection and scrolling use the damped spring model and intentionally overshoot slightly before settling.

```cpp
menu.set_selection_style(epui::MenuSelectionStyle::Indicator);
```

### GlideFrame

`GlideFrame` keeps the classic small-U8g2 two-speed path, but that path is only a **virtual target**:

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

The visible frame no longer sits directly on `glide_position - glide_scroll`. A damped spring follows every intermediate relative position produced by that deterministic path:

```text
selected item
   -> two-speed glide target
   -> frame spring target
   -> visible frame position
   -> overshoot
   -> rebound
   -> settle
```

This preserves the clean U8g2 timing while giving the cursor the same physical spring character as `LiquidGlass`.

### SlideFrame

`SlideFrame` uses the same two-speed virtual target and the same visible spring follower as `GlideFrame`, but keeps a full-width rounded frame instead of fitting the selected content.

```cpp
menu.set_selection_style(epui::MenuSelectionStyle::SlideFrame);
```

Both `GlideFrame` and `SlideFrame` therefore have real position overshoot/rebound, not only velocity-driven geometric deformation.

### Shared rounded-frame spring and jelly

`GlideFrame`, `SlideFrame` and `LiquidGlass` use the same spring constants for visible motion:

```cpp
style.spring_stiffness = 0.22f;
style.spring_damping = 0.35f;
```

They also share the same velocity-driven frame deformation:

```cpp
style.glass_max_stretch = 5;
style.glass_stretch_per_velocity = 0.35f;
style.glass_motion_threshold = 0.12f;
```

Conceptually:

```text
visible spring velocity
        -> stretch amount
        -> horizontal squeeze
        -> vertical stretch
```

For Glide/Slide, the spring follows the deterministic glide target. For Glass, the selection/scroll position is itself spring-driven. The resulting visible frame velocity goes through the same deformation formula in all three styles.

The older `frame_jelly_*` fields remain in `MenuStyle` for source compatibility but no longer drive a separate animation path.

### LiquidGlass

`LiquidGlass` keeps its direct spring-driven selection motion and adds the motion-only inverse sheen and edge highlight:

```cpp
menu.set_selection_style(epui::MenuSelectionStyle::LiquidGlass);
style.glass_sheen_height = 2;
style.glass_max_stretch = 5;
style.glass_stretch_per_velocity = 0.35f;
style.glass_motion_threshold = 0.12f;
```

Once the spring settles, LiquidGlass becomes a plain full-width rounded frame with no permanent sheen or highlight line.

## Sticky scrolling beyond one page

Long menus combine two layers of viewport state:

1. `first_visible` keeps a sticky logical row window, so moving back to an item already visible does not immediately drag the page.
2. A per-menu pixel `scroll_target` adds only the extra offset required to keep the selected resting frame inside `viewport_top..viewport_bottom`.

The resulting rule is:

```text
selected frame fits inside current pixel viewport
    -> keep list scroll fixed
    -> move only the cursor

selected frame would cross bottom safe edge
    -> raise the list only as many pixels as required
    -> keep the selected frame fully visible
    -> allow older top rows to become partially clipped

moving upward while selected frame still fits
    -> keep list fixed
    -> move the cursor upward first

selected frame would cross top comfort edge
    -> scroll the list downward only as much as required
```

For the demo, selecting row four changes the exact target from `0` to `4`, not a whole 13-pixel row. Continuing down then produces `17`, `30`, and so on. From a deeper position, moving upward keeps `30` while the cursor moves from item six to five to four; only when the frame reaches the top comfort edge does the target begin decreasing (`30 -> 26 -> 13 -> 0`).

GlideFrame and SlideFrame keep the deterministic two-speed list scroll path, while Indicator and LiquidGlass use the spring scroll path:

```cpp
style.visible_rows = 4;
style.row_height = 13;
style.glide_scroll_fast_step = 4;
style.glide_scroll_slow_zone = 4;
```

When the viewport target changes, rounded-frame styles retain the shared edge-handoff spring:

```cpp
style.scroll_handoff_kick = 2.0f;
```

The handoff uses the same `spring_stiffness` and `spring_damping` as the rest of the frame motion. Set it lower for a calmer edge transition or to `0.0f` if an application wants the cursor perfectly pinned while the content scrolls.

`first_visible_index()` and `viewport_scroll_target()` are exposed for diagnostics/tests.

## Ubuntu demo

The simulator contains both a root menu longer than one page and a dedicated ten-item `Long Menu`, so the fourth-row lift, clipped top row, protected footer and frame-first upward navigation can be inspected directly.

All four cursor styles remain selectable under:

```text
Jelly Menu -> Display -> Theme -> Cursor
```

Press `Select` on `Cursor` to cycle through `Indicator`, `Glide`, `Slide` and `Glass`. The demo defaults to `Glide`.
