#pragma once

#include <algorithm>
#include <cstddef>
#include "epui/canvas.hpp"

namespace epui {

struct PlotRect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

struct PlotRange {
    float x_min{0.0f};
    float x_max{1.0f};
    float y_min{0.0f};
    float y_max{1.0f};
};

struct PlotPointF {
    float x{0.0f};
    float y{0.0f};
};

namespace detail {

inline int plot_round(float value) {
    return static_cast<int>(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

inline int plot_x(const PlotRect& rect, const PlotRange& range, float value) {
    const float span = range.x_max - range.x_min;
    const float normalized = span == 0.0f ? 0.0f : (value - range.x_min) / span;
    const float clipped = std::max(0.0f, std::min(1.0f, normalized));
    return rect.x + plot_round(clipped * static_cast<float>(rect.width - 1));
}

inline int plot_y(const PlotRect& rect, const PlotRange& range, float value) {
    const float span = range.y_max - range.y_min;
    const float normalized = span == 0.0f ? 0.0f : (value - range.y_min) / span;
    const float clipped = std::max(0.0f, std::min(1.0f, normalized));
    return rect.y + rect.height - 1
        - plot_round(clipped * static_cast<float>(rect.height - 1));
}

} // namespace detail

template <typename Function>
void draw_function_plot(Canvas& canvas, PlotRect rect, PlotRange range,
                        Function function, bool on = true) {
    if (rect.width <= 0 || rect.height <= 0) return;
    int previous_x = rect.x;
    int previous_y = detail::plot_y(rect, range, function(range.x_min));
    if (rect.width == 1) {
        canvas.pixel(previous_x, previous_y, on);
        return;
    }
    for (int column = 1; column < rect.width; ++column) {
        const float progress = static_cast<float>(column)
            / static_cast<float>(rect.width - 1);
        const float x_value = range.x_min + (range.x_max - range.x_min) * progress;
        const int x = rect.x + column;
        const int y = detail::plot_y(rect, range, function(x_value));
        canvas.line(previous_x, previous_y, x, y, on);
        previous_x = x;
        previous_y = y;
    }
}

template <typename Function>
void draw_parametric_plot(Canvas& canvas, PlotRect rect, PlotRange range,
                          float t_min, float t_max, std::size_t samples,
                          Function function, bool on = true) {
    if (rect.width <= 0 || rect.height <= 0 || samples == 0) return;
    PlotPointF point = function(t_min);
    int previous_x = detail::plot_x(rect, range, point.x);
    int previous_y = detail::plot_y(rect, range, point.y);
    if (samples == 1) {
        canvas.pixel(previous_x, previous_y, on);
        return;
    }
    for (std::size_t i = 1; i < samples; ++i) {
        const float progress = static_cast<float>(i)
            / static_cast<float>(samples - 1);
        point = function(t_min + (t_max - t_min) * progress);
        const int x = detail::plot_x(rect, range, point.x);
        const int y = detail::plot_y(rect, range, point.y);
        canvas.line(previous_x, previous_y, x, y, on);
        previous_x = x;
        previous_y = y;
    }
}

template <typename Value>
void draw_series(Canvas& canvas, PlotRect rect, const Value* values,
                 std::size_t count, float minimum, float maximum,
                 bool on = true) {
    if (!values || count == 0 || rect.width <= 0 || rect.height <= 0) return;
    PlotRange range{0.0f, 1.0f, minimum, maximum};
    const std::size_t points = std::min<std::size_t>(
        count, static_cast<std::size_t>(rect.width));
    std::size_t index = 0;
    int previous_x = rect.x;
    int previous_y = detail::plot_y(rect, range, static_cast<float>(values[0]));
    if (points == 1) {
        canvas.pixel(previous_x, previous_y, on);
        return;
    }
    for (std::size_t i = 1; i < points; ++i) {
        index = i * (count - 1) / (points - 1);
        const int x = rect.x + static_cast<int>(
            i * static_cast<std::size_t>(rect.width - 1) / (points - 1));
        const int y = detail::plot_y(
            rect, range, static_cast<float>(values[index]));
        canvas.line(previous_x, previous_y, x, y, on);
        previous_x = x;
        previous_y = y;
    }
}

} // namespace epui
