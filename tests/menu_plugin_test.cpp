#include "epui/menu_plugin.hpp"
#include <cassert>
#include <cstring>

namespace {

bool toggle_value = false;
int numeric_value = 5;
int actions = 0;

void action(void*) { ++actions; }

bool pixel_on(const epui::Canvas& canvas, int x, int y) {
    const std::size_t index = static_cast<std::size_t>(x + (y / 8) * epui::Canvas::Width);
    return ((canvas.data()[index] >> (y & 7)) & 1u) != 0u;
}

const epui::MenuItem level4_items[] = {
    epui::MenuItem::action("Fire", action),
};
const epui::Menu level4 = epui::make_menu("Level4", level4_items);

const epui::MenuItem level3_items[] = {
    epui::MenuItem::submenu("Level4", level4),
};
const epui::Menu level3 = epui::make_menu("Level3", level3_items);

const epui::MenuItem level2_items[] = {
    epui::MenuItem::submenu("Level3", level3),
};
const epui::Menu level2 = epui::make_menu("Level2", level2_items);

const epui::MenuItem root_items[] = {
    epui::MenuItem::submenu("Nested", level2),
    epui::MenuItem::toggle("Toggle", toggle_value),
    epui::MenuItem::value("Value", numeric_value, 0, 20, 2),
    epui::MenuItem::action("Action", action),
};
const epui::Menu root = epui::make_menu("Root", root_items);

} // namespace

int main() {
    epui::Canvas invert_canvas;
    invert_canvas.pixel(5, 5, true);
    invert_canvas.invert_rect(4, 4, 3, 3);
    assert(!pixel_on(invert_canvas, 5, 5));
    assert(pixel_on(invert_canvas, 4, 4));
    invert_canvas.invert_rect(4, 4, 3, 3);
    assert(pixel_on(invert_canvas, 5, 5));
    assert(!pixel_on(invert_canvas, 4, 4));

    static_assert(static_cast<unsigned>(epui::MenuSelectionStyle::LiquidGlass)
                  == static_cast<unsigned>(epui::MenuSelectionStyle::GlideFrame),
                  "LiquidGlass must remain a source-compatible GlideFrame alias");

    epui::Ui ui;
    epui::MenuPagePlugin<8> menu(ui, root, "test-menu");
    assert(menu.kind() == epui::PluginKind::Menu);
    assert(menu.start());
    assert(menu.attached());
    assert(ui.page_count() == 1);
    assert(menu.focused());
    assert(menu.selection_style() == epui::MenuSelectionStyle::Indicator);

    epui::Canvas indicator_canvas;
    menu.draw(indicator_canvas, 0);
    menu.set_selection_style(epui::MenuSelectionStyle::GlideFrame);
    assert(menu.selection_style() == epui::MenuSelectionStyle::GlideFrame);
    epui::Canvas glide_canvas;
    menu.draw(glide_canvas, 16);
    assert(std::memcmp(indicator_canvas.data(), glide_canvas.data(), epui::Canvas::BufferSize) != 0);
    menu.set_selection_style(epui::MenuSelectionStyle::Indicator);

    menu.on_key(epui::Key::Select);
    assert(menu.depth() == 2);
    menu.on_key(epui::Key::Select);
    assert(menu.depth() == 3);
    menu.on_key(epui::Key::Select);
    assert(menu.depth() == 4);
    menu.on_key(epui::Key::Select);
    assert(actions == 1);

    menu.on_key(epui::Key::Back);
    menu.on_key(epui::Key::Back);
    menu.on_key(epui::Key::Back);
    assert(menu.depth() == 1);

    menu.on_key(epui::Key::Next);
    menu.on_key(epui::Key::Select);
    assert(toggle_value);

    menu.on_key(epui::Key::Next);
    menu.on_key(epui::Key::Select);
    assert(menu.editing());
    menu.on_key(epui::Key::Next);
    assert(numeric_value == 7);
    menu.on_key(epui::Key::Prev);
    assert(numeric_value == 5);
    menu.on_key(epui::Key::Select);
    assert(!menu.editing());

    menu.on_key(epui::Key::Next);
    menu.on_key(epui::Key::Select);
    assert(actions == 2);

    // The original indicator keeps the damped-spring overshoot behavior.
    menu.reset_to_root(true);
    epui::Canvas canvas;
    menu.draw(canvas, 32);
    menu.on_key(epui::Key::Next);
    float maximum = menu.selection_position();
    for (std::uint32_t t = 48; t <= 672; t += 16) {
        menu.draw(canvas, t);
        if (menu.selection_position() > maximum) maximum = menu.selection_position();
    }
    assert(maximum > static_cast<float>(menu.style().row_height) + 0.5f);
    assert(!menu.jelly_active());

    // GlideFrame follows the U8g2-style two-speed approach: 5 px while far,
    // then 1 px near the target, with no overshoot.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::GlideFrame);
    menu.draw(canvas, 688);
    assert(menu.glide_position() == 0);
    menu.on_key(epui::Key::Next);
    assert(menu.glide_target_position() == static_cast<int>(menu.style().row_height));
    menu.draw(canvas, 704);
    assert(menu.glide_position() == 5);
    menu.draw(canvas, 720);
    assert(menu.glide_position() == static_cast<int>(menu.style().row_height));
    menu.draw(canvas, 736);
    assert(menu.glide_position() == static_cast<int>(menu.style().row_height));

    // Moving to an action item also demonstrates independent frame-width
    // interpolation: rows with a right-side indicator use the full frame,
    // while a plain action can shrink-wrap its text plus symmetric padding.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::GlideFrame);
    menu.draw(canvas, 752);
    const int full_width = menu.glide_width();
    menu.on_key(epui::Key::Next);
    menu.on_key(epui::Key::Next);
    menu.on_key(epui::Key::Next);
    menu.draw(canvas, 768);
    assert(menu.glide_target_width() < full_width);
    int previous_width = menu.glide_width();
    for (std::uint32_t t = 784; t <= 1200; t += 16) {
        menu.draw(canvas, t);
        assert(menu.glide_width() <= previous_width);
        assert(menu.glide_width() >= menu.glide_target_width());
        previous_width = menu.glide_width();
    }
    assert(menu.glide_width() == menu.glide_target_width());
    assert(!menu.jelly_active());

    menu.reset_to_root(true);
    menu.on_key(epui::Key::Back);
    assert(!menu.focused());
    assert(!menu.captures_key(epui::Key::Next));
    assert(menu.captures_key(epui::Key::Select));
    menu.on_key(epui::Key::Select);
    assert(menu.focused());

    menu.stop();
    assert(ui.page_count() == 0);
    return 0;
}
