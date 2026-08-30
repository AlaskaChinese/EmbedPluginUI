#include "epui/animation_plugin.hpp"
#include "epui/callback_transport.hpp"
#include "epui/canvas.hpp"
#include "epui/diagnostics_plugin.hpp"
#include "epui/display_plugin.hpp"
#include "epui/encoder_input_plugin.hpp"
#include "epui/event_bus.hpp"
#include "epui/fps_debug_plugin.hpp"
#include "epui/gpio_input_plugin.hpp"
#include "epui/i2c_transport.hpp"
#include "epui/input_plugin.hpp"
#include "epui/menu_plugin.hpp"
#include "epui/oled.hpp"
#include "epui/oled_display_plugin.hpp"
#include "epui/page.hpp"
#include "epui/page_plugin.hpp"
#include "epui/platform/esp32_idf.hpp"
#include "epui/platform/stm32_hal.hpp"
#include "epui/platform_plugin.hpp"
#include "epui/plot.hpp"
#include "epui/plugin.hpp"
#include "epui/plugin_registry.hpp"
#include "epui/popup_plugin.hpp"
#include "epui/sensor_plugin.hpp"
#include "epui/service_plugin.hpp"
#include "epui/standard_widgets.hpp"
#include "epui/spring.hpp"
#include "epui/theme_plugin.hpp"
#include "epui/terminal_view.hpp"
#include "epui/terminal_controls.hpp"
#include "epui/terminal_line_editor.hpp"
#include "epui/widget_plugin.hpp"
#include <cassert>

class PublicPage final : public epui::PagePlugin {
public:
    explicit PublicPage(epui::Ui& ui) : epui::PagePlugin(ui, "public-page") {}
    void draw(epui::Canvas& canvas, std::uint32_t) override { canvas.pixel(0, 0); }
    void on_key(epui::Key) override { ++keys; }
    void on_char(char value) override { last_char = value; }
    int keys{0};
    char last_char{0};
};

int main() {
    epui::Canvas canvas;
    epui::Ui ui;
    PublicPage page(ui);
    epui::DiagnosticsPlugin diagnostics(ui);
    epui::FpsDebugPlugin* compatibility = &diagnostics;
    page.draw(canvas, 0);
    assert(page.start());

    ui.handle(epui::InputEvent{epui::Key::Select, true, 'x'}, 0);
    assert(page.last_char == 'x' && page.keys == 0);
    ui.handle(epui::InputEvent{epui::Key::Select, false, 'y'}, 0);
    assert(page.last_char == 'x' && page.keys == 0);
    ui.handle(epui::InputEvent{epui::Key::Select, false, 0}, 0);
    assert(page.keys == 0);
    ui.handle(epui::Key::Select, 0);
    assert(page.keys == 1);

    PublicPage second(ui);
    assert(second.start());
    ui.handle(epui::Key::Next, 1);
    assert(ui.animating());
    ui.handle(epui::InputEvent{epui::Key::Select, true, 'z'}, 1);
    assert(page.last_char == 'x' && second.last_char == 0);

    return canvas.data()[0] == 0 || compatibility->kind() != epui::PluginKind::Debug ? 1 : 0;
}
