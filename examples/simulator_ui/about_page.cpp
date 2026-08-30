#include "about_page.hpp"
#include "epui/widgets.hpp"

namespace epui::demo {

void AboutPage::draw(Canvas& c, std::uint32_t) {
    draw_header(c, "About", 3, 6);
    c.text(8, 20, "128 x 64 MONO");
    c.text(8, 31, "SSD1306 SH1106");
    c.text(8, 42, "C++17 / PLUGINS");
}

} // namespace epui::demo
