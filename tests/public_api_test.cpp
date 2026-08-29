#include "epui/animation_plugin.hpp"
#include "epui/callback_transport.hpp"
#include "epui/canvas.hpp"
#include "epui/display_plugin.hpp"
#include "epui/encoder_input_plugin.hpp"
#include "epui/event_bus.hpp"
#include "epui/gpio_input_plugin.hpp"
#include "epui/i2c_transport.hpp"
#include "epui/input_plugin.hpp"
#include "epui/oled.hpp"
#include "epui/oled_display_plugin.hpp"
#include "epui/page.hpp"
#include "epui/page_plugin.hpp"
#include "epui/platform/esp32_idf.hpp"
#include "epui/platform/stm32_hal.hpp"
#include "epui/platform_plugin.hpp"
#include "epui/plugin.hpp"
#include "epui/plugin_registry.hpp"
#include "epui/sensor_plugin.hpp"
#include "epui/service_plugin.hpp"
#include "epui/standard_widgets.hpp"
#include "epui/theme_plugin.hpp"
#include "epui/widget_plugin.hpp"

class PublicPage final : public epui::PagePlugin {
public:
    explicit PublicPage(epui::Ui& ui) : epui::PagePlugin(ui, "public-page") {}
    void draw(epui::Canvas& canvas, std::uint32_t) override { canvas.pixel(0, 0); }
};

int main() {
    epui::Canvas canvas;
    epui::Ui ui;
    PublicPage page(ui);
    page.draw(canvas, 0);
    return canvas.data()[0] == 0 ? 1 : 0;
}
