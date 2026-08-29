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

Input plugins --------------------------------> Ui::handle()
(button / encoder / keyboard)
```

## Core rules

- C++17 portable core with no OS dependency.
- 128x64 monochrome framebuffer uses 1024 bytes.
- Rendering uses no dynamic allocation.
- Pages are application-owned and stored as references.
- 220 ms cubic ease-out transitions reuse the same framebuffer.
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
