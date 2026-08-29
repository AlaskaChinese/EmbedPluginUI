#include "sensor_page.hpp"
#include "epui/widgets.hpp"

namespace epui::demo {

void SensorPage::draw(Canvas& c, std::uint32_t now_ms) {
    draw_header(c, "Sensors", 2, 3);
    const float wave = 0.5f + 0.5f * static_cast<float>((now_ms / 25) % 100) / 100.0f;

    draw_card(c, 3, 17, 60, 38, "TEMP");
    draw_thermometer(c, 9, 32, wave);
    c.text(25, 35, "42.6 C");

    draw_card(c, 66, 17, 59, 38, "LOAD");
    c.progress_bar(72, 36, 47, 7, wave);
}

} // namespace epui::demo
