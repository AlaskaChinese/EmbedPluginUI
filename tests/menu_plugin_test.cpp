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

bool region_equal(const epui::Canvas& lhs, const epui::Canvas& rhs,
                  int x, int y, int width, int height) {
    for (int yy = y; yy < y + height; ++yy) {
        for (int xx = x; xx < x + width; ++xx) {
            if (pixel_on(lhs, xx, yy) != pixel_on(rhs, xx, yy)) return false;
        }
    }
    return true;
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
    menu.set_selection_style(epui::MenuSelectionStyle::LiquidGlass);
    assert(menu.selection_style() == epui::MenuSelectionStyle::LiquidGlass);
    assert(menu.style().glass_text_safe_padding_x == 1);
    epui::Canvas glass_canvas;
    menu.draw(glass_canvas, 16);
    assert(std::memcmp(indicator_canvas.data(), glass_canvas.data(), epui::Canvas::BufferSize) != 0);
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

    menu.reset_to_root(true);
    epui::Canvas canvas;
    menu.draw(canvas, 32);
    menu.on_key(epui::Key::Next);
    float maximum = menu.selection_position();
    for (std::uint32_t t = 48; t <= 672; t += 16) {
        canvas.clear();
        menu.draw(canvas, t);
        if (menu.selection_position() > maximum) maximum = menu.selection_position();
    }
    assert(maximum > static_cast<float>(menu.style().row_height) + 0.5f);
    assert(!menu.jelly_active());

    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::LiquidGlass);
    canvas.clear();
    menu.draw(canvas, 688);
    menu.on_key(epui::Key::Next);
    assert(menu.jelly_active());
    for (std::uint32_t t = 704; t <= 1328; t += 16) {
        canvas.clear();
        menu.draw(canvas, t);
    }
    assert(!menu.jelly_active());

    // During a fast glass transition, the frame and inverse sheen may cross the
    // row geometry, but the 5x7 glyph area must remain byte-for-byte readable.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::LiquidGlass);
    epui::Canvas protected_canvas;
    protected_canvas.clear();
    menu.draw(protected_canvas, 1344);
    menu.on_key(epui::Key::Next);
    for (std::uint32_t t = 1360; t <= 1392; t += 16) {
        protected_canvas.clear();
        menu.draw(protected_canvas, t);
    }
    assert(menu.jelly_active());

    epui::Canvas expected_text;
    const int text_x = static_cast<int>(menu.style().text_x);
    const int first_y = static_cast<int>(menu.style().content_top);
    const int second_y = first_y + static_cast<int>(menu.style().row_height);
    expected_text.text(text_x, first_y, "Nested");
    expected_text.text(text_x, second_y, "Toggle");
    assert(region_equal(protected_canvas, expected_text,
                        text_x, first_y, expected_text.text_width("Nested"), 7));
    assert(region_equal(protected_canvas, expected_text,
                        text_x, second_y, expected_text.text_width("Toggle"), 7));

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
