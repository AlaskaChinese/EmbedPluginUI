# Plugin runtime

EmbedPluginUI treats every replaceable subsystem as a plugin boundary while keeping the runtime suitable for microcontrollers.

## Design constraints

- No dynamic plugin loading.
- No heap allocation in the registry.
- No ownership transfer: the application owns plugin objects.
- Fixed registry capacity: 16 plugins by default.
- Deterministic startup order and reverse shutdown order.
- Startup failure rolls back already-started plugins.

## Core types

- `epui::Plugin`: common lifecycle and metadata contract.
- `epui::PluginRegistry`: fixed-capacity lifecycle manager.
- `epui::DisplayPlugin`: framebuffer output contract.
- `epui::InputPlugin`: normalized `Key` event source.
- `epui::OledDisplayPlugin`: adapter for `Oled128x64` (SSD1306/SH1106).
- `epui::CallbackInputPlugin`: wraps board-specific polling callbacks.
- `epui::QueuedInputPlugin<N>`: tiny fixed-size event queue for GPIO, encoders and simulators.

## Example

```cpp
#include <epui/plugin_registry.hpp>
#include <epui/oled_display_plugin.hpp>
#include <epui/input_plugin.hpp>

epui::Oled128x64 oled(bus, epui::OledController::SSD1306);
epui::OledDisplayPlugin display(oled);
epui::QueuedInputPlugin<8> buttons("gpio-buttons");

epui::PluginRegistry plugins;
plugins.add(display);
plugins.add(buttons);

if (!plugins.start_all()) {
    // Every plugin started before the failure has already been stopped.
}

// Render / poll loop...
plugins.stop_all();
```

## Runtime model

The registry is intentionally not a desktop-style dynamic loader. STM32, ESP32 and Raspberry Pi applications instantiate the required plugins at compile time, then register them with a common lifecycle manager. This preserves deterministic memory use while keeping platform and device code replaceable.
