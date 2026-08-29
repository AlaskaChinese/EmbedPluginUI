# FPS debug plugin

`FpsDebugPlugin` is a heap-free `UiOverlay` plugin that measures and displays the current UI frame rate.

```cpp
epui::Ui ui;
epui::FpsDebugPlugin fps(ui);
fps.start();

// ui.render(canvas, now_ms) automatically counts one frame.
```

The default overlay is drawn at the bottom-right of the 128x64 framebuffer and samples over a 500 ms window. It renders a compact value such as `FPS 60.1`.

## Runtime control

```cpp
fps.set_visible(false);  // keep sampling, hide text
fps.set_visible(true);
```

The Ubuntu simulator exposes this under `Jelly Menu -> System -> Debug -> FPS Overlay`.

## Render FPS vs presented FPS

By default the plugin counts `Ui::render()` calls because it is registered as a `UiOverlay`. This is the most useful measure for the simulator and for embedded applications that render once before each display flush.

If a platform wants to measure successful physical display presents instead, disable automatic sampling and call `mark_frame()` after the display transfer succeeds:

```cpp
fps.set_auto_sample(false);

ui.render(canvas, now_ms);
if (display.present(canvas)) {
    fps.mark_frame(now_ms);
}
```

The overlay will then show the most recently sampled presented-frame rate on the next render.

## Styling

```cpp
epui::FpsDebugStyle style;
style.x = -1;                 // negative = right aligned
style.y = 54;
style.background = true;
style.padding = 1;
style.sample_window_ms = 500;
style.label = "FPS";

epui::FpsDebugPlugin fps(ui, "fps-debug", style);
```

The plugin uses fixed integer counters and fixed-size text buffers; it does not allocate memory dynamically.
