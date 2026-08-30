# PopupPlugin

`epui::PopupPlugin` is a fixed-capacity modal overlay for alerts and
confirmations on a 128x64 display. It allocates no heap memory and works with
the same `Ui`, input plugins and framebuffer on MCU, Linux, Windows and
Raspberry Pi targets.

```cpp
#include <epui/popup_plugin.hpp>

epui::Ui ui;
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

The callback signature is:

```cpp
void on_popup_result(void* user, epui::PopupResult result);
```

`Next`/`Prev` or Left/Right changes the selected button, `Select` or Enter
accepts it, and `Back` or Escape cancels. While the popup is opening, visible
or closing, it is the top-level input target. The underlying page cannot
navigate or receive characters until the popup is fully hidden.

## Animation

Opening moves the panel from above the framebuffer to `resting_y`. Closing
reverses the same spring and retracts the panel above the framebuffer. Current
velocity is retained when direction changes, so closing during the opening
motion does not jump. Velocity also adds a small, capped vertical stretch to
both directions.

`PopupStyle::spring` uses the same `SpringStyle` as page transitions. Frame
time is capped by `max_frame_ms`, which prevents a delayed desktop or I2C frame
from making the panel teleport.

Callbacks run after the closing animation has completed. The popup keeps
capturing input during that animation to prevent the closing key from reaching
the page underneath.

Titles and messages are copied into fixed buffers. Messages are wrapped to at
most two lines. The first component intentionally supports only `OK` and
`OK/CANCEL`; applications needing arbitrary interactive content can implement
another `UiOverlay` using the same modal input hooks.
