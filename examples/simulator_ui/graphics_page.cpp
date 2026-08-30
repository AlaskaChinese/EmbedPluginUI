#include "graphics_page.hpp"
#include "epui/plot.hpp"
#include "epui/widgets.hpp"
#include <cmath>

namespace epui::demo {

void GraphicsDemoPage::draw(Canvas& canvas, std::uint32_t now_ms) {
    draw_header(canvas, "Graphics", 6, 6);
    canvas.text(3, 15, u8"42°C  42℃");
    canvas.text(68, 15, u8"± µ Ω × ÷");

    canvas.circle(9, 43, 6);
    canvas.fill_circle(25, 43, 5);

    const PlotRect plot{38, 33, 88, 23};
    const PlotRange range{0.0f, 6.2831853f, -1.1f, 1.1f};
    canvas.rect(plot.x, plot.y, plot.width, plot.height);
    canvas.line(plot.x, plot.y + plot.height / 2,
                plot.x + plot.width - 1, plot.y + plot.height / 2);
    const float phase = static_cast<float>(now_ms % 4000u) * 0.0015f;
    draw_function_plot(canvas, plot, range,
        [phase](float x) { return std::sin(x + phase); });
    draw_function_plot(canvas, plot, range,
        [phase](float x) { return 0.65f * std::cos(x - phase); });
}

void GraphicsDemoPage::on_key(Key key) {
    if (key != Key::Select) return;
    popup_.show(u8"THERMAL", u8"CPU reached 80℃ ±2°",
                PopupButtons::OkCancel, popup_result, this);
}

void GraphicsDemoPage::popup_result(void* user, PopupResult result) {
    if (user) static_cast<GraphicsDemoPage*>(user)->last_result_ = result;
}

} // namespace epui::demo
