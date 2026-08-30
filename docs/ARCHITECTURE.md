# EmbedPluginUI architecture

EmbedPluginUI follows one rule: **everything that can vary is a plugin boundary**.

```text
                    Application
                        |
             Page / Widget plugins
                        |
                 Ui + Animation
                        |
                    Canvas
                        |
             Display plugin boundary
              /         |          \
          SSD1306     SH1106     Simulator
                        |
                Transport plugin
             /           |          \
         STM32         ESP32      Linux I2C

Input plugins -------------------------------> Ui::handle(InputEvent)
(button / encoder / keyboard)                         |
                                             top modal overlay
                                              /             \
                                         on_key()        on_char()
                                                \         /
                                         stable current page
```

## Core rules

- C++17 portable core with no OS dependency.
- 128x64 monochrome framebuffer uses 1024 bytes.
- Rendering uses no dynamic allocation.
- Text input and semantic navigation share `InputEvent` but remain separate
  callbacks. The topmost modal overlay gets first refusal; otherwise the event
  reaches the stable current page.
- `TerminalView` owns fixed-capacity terminal parsing, history and rendering;
  PTY/FIFO readers stay in platform adapters.
- Pages are application-owned and stored as references.
- `Spring1D` is shared by configurable page transitions and overlay motion;
  both reuse the same framebuffer.
- `Canvas` decodes a compact UTF-8 symbol subset. Heap-free plot helpers sample
  functions or series directly into a caller-supplied rectangle.
- Hardware details stay outside pages and widgets.
- New display controllers, transports, platforms and input devices enter through adapters/plugins.
- Plugin dependencies are resolved deterministically before startup.

## Naming invariant

- Project: `EmbedPluginUI`
- CMake library: `epui`
- Public and implementation headers: `epui/...`
- Namespace: `epui`
- Platform sub-namespaces: `epui::rpi`, `epui::platform`, etc.
- Build option/macro prefix: `EPUI_*`
- Desktop simulator executable: `epui_sim`
- Raspberry Pi executable: `epui_rpi`

The source tree intentionally uses one naming system end to end. CI rejects reintroduction of retired project names or include paths.
