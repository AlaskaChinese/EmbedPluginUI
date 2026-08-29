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

The original left-side indicator remains the default. `LiquidGlass` is the OLED-native liquid capsule style and can be switched at runtime:

```cpp
menu.set_selection_style(epui::MenuSelectionStyle::Indicator);
menu.set_selection_style(epui::MenuSelectionStyle::LiquidGlass);
```

`LiquidGlass` no longer tries to imitate an LCD blur layer. It is designed around a 1-bit 128x64 OLED:

- the capsule position follows the same damped spring as the menu selection;
- velocity squeezes the capsule horizontally instead of filling the row with a rigid rectangle;
- a metaball-like pair of side bridges stretches back toward the previous selection anchor;
- small landing lobes pull the capsule toward the target row;
- the long top/bottom edges open while moving so the shape reads as a deforming membrane;
- a short specular edge highlight moves on the leading side;
- optional checker/dither trail pixels suggest translucent persistence without grayscale;
- nearby labels and values receive a temporary 1-pixel horizontal displacement to mimic lens refraction instead of being inverted by a solid bar.

The effect is still heap-free and uses the existing 1024-byte framebuffer.

```cpp
epui::MenuStyle style;
style.selection_style = epui::MenuSelectionStyle::LiquidGlass;
style.glass_width = 122;
style.glass_height = 9;
style.glass_radius = 4;
style.glass_max_stretch = 5;
style.glass_stretch_per_velocity = 0.35f;
style.liquid_bridge_width = 2;
style.liquid_bridge_max_span = 18;
style.liquid_refraction_px = 1;
style.liquid_refraction_radius = 6;
style.liquid_dither_trail = true;
style.liquid_trail_length = 6;
```

The Ubuntu simulator demonstrates runtime switching under `Jelly Menu -> Display -> Theme -> Liquid Cursor`.
