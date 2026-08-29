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

Rows near the physical bottom edge are no longer discarded by a conservative fixed margin. If a row begins inside the 128x64 framebuffer, it is rendered and `Canvas` clips any pixels that extend below the display. This means a fourth row at `y = 55` remains visible instead of disappearing entirely. The same intersection rule is used for selection frames, so partial bottom-edge frames can be shown while scrolling.

## Four selection animations

### Indicator

`Indicator` is the original left-side marker. Its selection and scrolling use the damped spring model and intentionally overshoot slightly before settling.

```cpp
menu.set_selection_style(epui::MenuSelectionStyle::Indicator);
```

### GlideFrame

`GlideFrame` keeps the classic small-U8g2 two-speed path, but that path is now only a **virtual target**:

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

The visible frame no longer sits directly on `glide_position - glide_scroll`. Instead, a damped spring follows every intermediate relative position produced by that deterministic path:

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

Both `GlideFrame` and `SlideFrame` therefore have real position overshoot/rebound now, not only velocity-driven geometric deformation.

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

## Smooth scrolling beyond one page

A menu can contain more items than `visible_rows`. GlideFrame and SlideFrame keep the two-speed list scroll path, while the visible selection frame springs around the **relative** item-minus-scroll target. The list therefore stays predictable while the cursor still rebounds naturally. Indicator and LiquidGlass use their spring scroll path.

```cpp
style.visible_rows = 4;
style.row_height = 13;
style.glide_scroll_fast_step = 4;
style.glide_scroll_slow_zone = 4;
```

The Ubuntu simulator contains both a root menu longer than one page and a dedicated ten-item `Long Menu`. With the demo's 13-pixel row pitch, the fourth row intentionally sits near the bottom edge so partial/bottom-edge rendering can be inspected directly.

## Ubuntu demo

The simulator exposes all four styles under:

```text
Jelly Menu -> Display -> Theme -> Cursor
```

Press `Select` on `Cursor` to cycle through `Indicator`, `Glide`, `Slide` and `Glass`. The demo defaults to `Glide`, so the new spring-follow behavior is visible immediately.
