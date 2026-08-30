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

Input plugins --------------------------------> Ui::handle(InputEvent)
(button / encoder / keyboard)                    |          |
                                              on_key()   on_char()
```

## Core rules

- C++17 portable core with no OS dependency.
- 128x64 monochrome framebuffer uses 1024 bytes.
- Rendering uses no dynamic allocation.
- Text input and semantic navigation share `InputEvent` but remain separate
  page callbacks.
- `TerminalView` owns fixed-capacity terminal parsing, history and rendering;
  PTY/FIFO readers stay in platform adapters.
- Pages are application-owned and stored as references.
- Configurable damped-spring page transitions reuse the same framebuffer.
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
