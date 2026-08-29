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

No `std::vector`, `std::function`, runtime allocation or dynamic plugin loading is required.

## Input behavior

While the menu is focused:

- `Next`: next item, or increase a value while editing.
- `Prev`: previous item, or decrease a value while editing.
- `Select`: enter submenu, run action, toggle, or enter/leave value editing.
- `Back`: leave value editing, go to the parent menu, or unfocus at the root.

When the root menu is unfocused, `Next/Prev` return to normal top-level `Ui` page navigation. `Select` focuses the menu again. This lets the same three-button or encoder input device drive both pages and arbitrary menu depth.

## Layout freedom

The framework owns the layout parameters; an application or demo chooses values that suit its display and font. In particular, menu text and right-side values/arrows are positioned from the selection-frame edges rather than from unrelated screen margins:

```cpp
epui::MenuStyle style;
style.glass_x = 3;
style.glass_width = 122;
style.content_inset_left = 8;
style.content_inset_right = 8;
```

With equal insets, the resting frame has the same visual breathing room on both sides. Applications can deliberately make the two values different when needed. `text_x` and `right_margin` remain in the struct for source compatibility with older code, but new projects should prefer `content_inset_left/right`.

Vertical spacing is also application-defined:

```cpp
style.row_height = 12;
style.glass_height = 9;
```

This leaves a visible gap between adjacent selection capsules. A compact menu can keep `row_height = 10`; a metaball-heavy animation generally looks better around 11-13 pixels with a 9-pixel capsule. The framework does not force either choice.

## Jelly animation

Selection, list scrolling and submenu slide-in use a small damped spring integrator. `MenuStyle::spring_stiffness` and `spring_damping` control the feel without requiring a second framebuffer or heap allocation.

```cpp
epui::MenuStyle style;
style.spring_stiffness = 0.22f;
style.spring_damping = 0.35f;
epui::MenuPagePlugin<12> menu(ui, root, "menu", style);
```

The default parameters intentionally overshoot slightly before settling, producing the OLED "jelly" motion demonstrated by the Ubuntu simulator.

## Selection styles

The original left-side indicator remains the default. `LiquidGlass` is the OLED-native liquid/metaball style and can be switched at runtime:

```cpp
menu.set_selection_style(epui::MenuSelectionStyle::Indicator);
menu.set_selection_style(epui::MenuSelectionStyle::LiquidGlass);
```

`LiquidGlass` does not try to reproduce alpha blur or desktop compositing. It is designed around a 1-bit 128x64 OLED. During a selection move, each end of the capsule behaves like a small liquid droplet. The previous/current pair first separates through a narrow neck, then the current/target pair fuses into the destination. The bridge width narrows in the middle like a simplified 2D metaball, while the main text band stays open and readable.

The effect combines:

- damped-spring position and overshoot;
- a configurable capsule outline;
- filled metaball droplets at the left and right capsule ends;
- a variable-width neck between the two droplet centers;
- a two-phase source-to-current then current-to-target fusion;
- a short leading-edge highlight;
- optional sparse trail pixels;
- optional 1-pixel label/value refraction near the moving capsule.

```cpp
epui::MenuStyle style;
style.selection_style = epui::MenuSelectionStyle::LiquidGlass;
style.row_height = 12;
style.glass_width = 122;
style.glass_height = 9;
style.glass_radius = 4;
style.content_inset_left = 8;
style.content_inset_right = 8;
style.liquid_metaball_radius = 3;
style.liquid_bridge_width = 1;
style.liquid_bridge_max_span = 16;
style.liquid_refraction_px = 1;
style.liquid_refraction_radius = 4;
style.liquid_dither_trail = false;
```

The Ubuntu simulator uses those values as one recommended 0.96-inch OLED preset, not as framework-wide constants. Runtime switching remains under `Jelly Menu -> Display -> Theme -> Liquid Cursor`.
