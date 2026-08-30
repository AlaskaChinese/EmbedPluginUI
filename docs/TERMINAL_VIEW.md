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
