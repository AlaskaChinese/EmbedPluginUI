# Text symbols and mathematical graphics

## UTF-8 symbol text

`Canvas::text()` and `Canvas::text_width()` decode UTF-8 while retaining the
existing ASCII API:

```cpp
canvas.text(3, 12, u8"42°C");
canvas.text(3, 22, u8"80℃ ±2°");
canvas.text(3, 32, u8"5µA  10Ω  ←↑→↓  × ÷");
```

The built-in extension table contains `°`, `℃`, `±`, `µ`, `Ω`, arrows,
multiplication and division. Unknown or malformed codepoints render as `?`.
This is a deliberately small symbol set rather than a complete Unicode or
Chinese font. Additional glyphs can be added to the static extension table
without changing the text API.

`TerminalView` remains ASCII because changing its fixed history cells to
Unicode would increase storage. UTF-8 support here applies to normal Canvas UI
text and popup content.

## Primitive geometry

`Canvas` provides pixels, lines, rectangles, rounded rectangles, outline
circles and filled circles:

```cpp
canvas.circle(16, 24, 8);
canvas.fill_circle(40, 24, 6);
```

## Function plots

`epui/plot.hpp` maps mathematical coordinates to a pixel rectangle and connects
samples with the existing integer line rasterizer:

```cpp
#include <epui/plot.hpp>
#include <cmath>

epui::draw_function_plot(
    canvas,
    {2, 18, 124, 36},
    {0.0f, 6.2831853f, -1.0f, 1.0f},
    [](float x) { return std::sin(x); }
);
```

`draw_parametric_plot()` accepts a function returning `PlotPointF`, which can
represent circles, spirals and other curves. `draw_series()` renders an
existing fixed array of sensor samples. All helpers are header-only and use no
point buffer or `std::function`.

Function plots sample once per horizontal pixel. This is inexpensive on a
Raspberry Pi. Smaller MCUs can pass a lookup-table function or reduce the plot
width without changing the plotting API.
