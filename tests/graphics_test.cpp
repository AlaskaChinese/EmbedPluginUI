#include "epui/canvas.hpp"
#include "epui/plot.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>

namespace {

bool pixel_on(const epui::Canvas& canvas, int x, int y) {
    const std::size_t index = static_cast<std::size_t>(
        x + (y / 8) * epui::Canvas::Width);
    return ((canvas.data()[index] >> (y & 7)) & 1u) != 0u;
}

std::size_t lit_pixels(const epui::Canvas& canvas) {
    std::size_t count = 0;
    for (int y = 0; y < epui::Canvas::Height; ++y) {
        for (int x = 0; x < epui::Canvas::Width; ++x) {
            if (pixel_on(canvas, x, y)) ++count;
        }
    }
    return count;
}

} // namespace

int main() {
    epui::Canvas canvas;
    assert(canvas.text_width("ABC") == 17);
    assert(canvas.text_width(u8"°") == 3);
    assert(canvas.text_width(u8"℃") == 9);
    assert(canvas.text_width(u8"25°C") == 21);

    canvas.text(0, 0, u8"°℃±µΩ←↑→↓×÷");
    assert(lit_pixels(canvas) > 40);

    canvas.clear();
    canvas.fill_circle(10, 10, 4);
    assert(pixel_on(canvas, 10, 10));
    assert(pixel_on(canvas, 6, 10));
    assert(!pixel_on(canvas, 5, 10));

    canvas.clear();
    const epui::PlotRect rect{0, 0, 64, 32};
    const epui::PlotRange range{0.0f, 6.2831853f, -1.0f, 1.0f};
    epui::draw_function_plot(canvas, rect, range,
        [](float x) { return std::sin(x); });
    assert(lit_pixels(canvas) >= 64);

    epui::draw_parametric_plot(canvas, rect, {-1.0f, 1.0f, -1.0f, 1.0f},
        0.0f, 6.2831853f, 48,
        [](float t) { return epui::PlotPointF{std::cos(t), std::sin(t)}; });
    const float samples[] = {0.0f, 1.0f, -1.0f, 0.5f};
    epui::draw_series(canvas, {64, 0, 32, 16}, samples, 4, -1.0f, 1.0f);
    assert(lit_pixels(canvas) > 100);
    return 0;
}
