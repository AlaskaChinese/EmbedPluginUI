#include "home_page.hpp"
#include "epui/widgets.hpp"

namespace epui::demo {

void HomePage::draw(Canvas& c, std::uint32_t now_ms) {
    draw_header(c, "EmbedPluginUI", 1, 3);
    c.text(5, 18, "PLUGIN-FIRST UI");
    const float progress = 0.25f + 0.75f * static_cast<float>(now_ms % 3000) / 3000.0f;
    c.progress_bar(5, 31, 118, 8, progress);
    c.text(5, 45, "NEXT: RIGHT / D");
    draw_spinner(c, 115, 49, now_ms);
}

} // namespace epui::demo
