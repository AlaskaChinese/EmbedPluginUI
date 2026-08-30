# TerminalView

`epui::TerminalView` is a fixed-capacity, line-oriented ASCII terminal model
and renderer for small monochrome displays.

```cpp
#include <epui/terminal_view.hpp>

epui::TerminalView<64, 21> terminal;
terminal.feed(data, size);
terminal.scroll(1);       // older output
terminal.scroll(-1);      // live output
terminal.draw(canvas, 1, 15, 126, 42, now_ms);
```

The template parameters are history lines and columns. Character storage is
`HistoryLines * (Columns + 1)` bytes, plus one `size_t` length per line and a
small amount of ring-buffer state. Applications targeting smaller MCUs should
choose a smaller history capacity explicitly.

## Supported input

- Printable ASCII (`0x20` through `0x7e`).
- `\n`: starts a new line at column zero.
- `\r`: returns to column zero without clearing the line.
- `\b`: moves the cursor left without erasing; a terminal or local echo can
  send backspace-space-backspace to erase visibly.
- Hard wrapping at `Columns`.
- ESC, CSI and OSC sequences are consumed without being rendered. Parser
  state is retained across `feed()` calls, so sequences may cross read chunks.

This is not a VT100 emulator. Cursor addressing, alternate screens, terminal
colors and full-screen applications are intentionally unsupported. A PTY
adapter should advertise `TERM=dumb`.

New output returns the viewport to the live cursor. The cursor can be disabled
for read-only log views with `set_cursor_visible(false)` and blinks from the
`now_ms` value passed to `draw()`.

`Canvas` currently has one top-level clip rather than a clip stack.
`TerminalView::draw()` sets its requested clip and resets it when finished, so
it should not be nested inside another active clip.

## Terminal controls

`TerminalView` remains an input-agnostic renderer. Applications can use the
small value-type `TerminalControls` mapping to keep terminal key behavior
separate from page and menu navigation:

```cpp
#include <epui/terminal_controls.hpp>

epui::TerminalControls controls;  // conventional defaults
controls.cursor_left = epui::Key::Left;
controls.cursor_right = epui::Key::Right;
controls.history_previous = epui::Key::Up;
controls.history_next = epui::Key::Down;
controls.output_up = epui::Key::ScrollUp;
controls.output_down = epui::Key::ScrollDown;

const epui::TerminalAction action = controls.action_for(key, focused);
```

The default mapping uses Enter to focus/execute, Escape to unfocus,
Left/Right for the local input cursor, Up/Down for command history, and
`ScrollUp`/`ScrollDown` for output.
The desktop and Raspberry Pi keyboard ports produce those scroll actions from
Ctrl+Up/Down. The supplied terminal pages scroll one output line per event;
normal keyboard repeat provides continuous movement when the keys are held.

`TerminalControls` is copied into the terminal page and can be supplied to its
constructor or replaced with `set_controls()`. No virtual interface, heap
allocation or platform key code is required in the core.

`TerminalLineEditor<CommandCapacity, HistoryCapacity>` provides the portable,
fixed-capacity command buffer used by the supplied pages. It supports insertion,
deletion, cursor movement, consecutive-duplicate suppression, history traversal
and restoration of the draft that was present before pressing Up. The generic
simulator uses 8 history entries and the Raspberry Pi application uses 16.

Tab completion is intentionally implemented in the Raspberry Pi application,
not in `TerminalView`: it queries the host filesystem and `PATH`, which are OS
services. It completes executable names in command position and files or
directories in argument position. Multiple matches advance only to their
longest common prefix; quoted shell expressions, aliases, functions and
shell-specific option completion remain the shell's responsibility.
