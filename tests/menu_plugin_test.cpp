#include "epui/menu_plugin.hpp"
#include <cassert>
#include <cmath>
#include <cstring>

namespace {

bool toggle_value = false;
int numeric_value = 5;
int actions = 0;
std::uint8_t choice_index = 0;
const char* const choice_options[] = {"One", "Two", "Three"};

void action(void*) { ++actions; }

bool pixel_on(const epui::Canvas& canvas, int x, int y) {
    const std::size_t index = static_cast<std::size_t>(x + (y / 8) * epui::Canvas::Width);
    return ((canvas.data()[index] >> (y & 7)) & 1u) != 0u;
}

int pixel_count_band(const epui::Canvas& canvas, int y0, int y1) {
    int count = 0;
    y0 = std::max(0, y0);
    y1 = std::min(epui::Canvas::Height - 1, y1);
    for (int y = y0; y <= y1; ++y) {
        for (int x = 0; x < epui::Canvas::Width; ++x) {
            if (pixel_on(canvas, x, y)) ++count;
        }
    }
    return count;
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

const epui::MenuItem choice_items[] = {
    epui::MenuItem::choice("Mode", choice_index, choice_options),
};
const epui::Menu choice_menu = epui::make_menu("Choice", choice_items);

const epui::MenuItem long_items[] = {
    epui::MenuItem::action("One"),
    epui::MenuItem::action("Two"),
    epui::MenuItem::action("Three"),
    epui::MenuItem::action("Four"),
    epui::MenuItem::action("Five"),
    epui::MenuItem::action("Six"),
    epui::MenuItem::action("Seven"),
};
const epui::Menu long_menu = epui::make_menu("Long", long_items);

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
    assert(menu.style().glass_height == 11);

    epui::Canvas indicator_canvas;
    menu.draw(indicator_canvas, 0);

    menu.set_selection_style(epui::MenuSelectionStyle::GlideFrame);
    epui::Canvas glide_canvas;
    menu.draw(glide_canvas, 16);
    assert(std::memcmp(indicator_canvas.data(), glide_canvas.data(), epui::Canvas::BufferSize) != 0);

    menu.set_selection_style(epui::MenuSelectionStyle::SlideFrame);
    epui::Canvas slide_canvas;
    menu.draw(slide_canvas, 32);
    assert(std::memcmp(indicator_canvas.data(), slide_canvas.data(), epui::Canvas::BufferSize) != 0);
    assert(menu.glide_target_width() == static_cast<int>(menu.style().glass_width));

    // At rest LiquidGlass is intentionally the same clean full-width rounded
    // frame as SlideFrame: no sheen/highlight remains once motion is zero.
    menu.set_selection_style(epui::MenuSelectionStyle::LiquidGlass);
    epui::Canvas glass_rest_canvas;
    menu.draw(glass_rest_canvas, 48);
    assert(std::memcmp(slide_canvas.data(), glass_rest_canvas.data(), epui::Canvas::BufferSize) == 0);

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

    // Indicator keeps the original damped-spring overshoot.
    menu.reset_to_root(true);
    epui::Canvas canvas;
    menu.draw(canvas, 64);
    menu.on_key(epui::Key::Next);
    float maximum = menu.selection_position();
    for (std::uint32_t t = 80; t <= 704; t += 16) {
        menu.draw(canvas, t);
        if (menu.selection_position() > maximum) maximum = menu.selection_position();
    }
    assert(maximum > static_cast<float>(menu.style().row_height) + 0.5f);

    // GlideFrame keeps deterministic two-speed Y motion, but its geometric
    // jelly now uses the same velocity->stretch formula as LiquidGlass.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::GlideFrame);
    menu.draw(canvas, 720);
    assert(menu.glide_position() == 0);
    menu.on_key(epui::Key::Next);
    assert(menu.glide_target_position() == static_cast<int>(menu.style().row_height));
    menu.draw(canvas, 736);
    assert(menu.glide_position() == 5);
    assert(std::fabs(menu.frame_motion_velocity()) > 0.5f);
    const float expected_glide_jelly = menu.frame_motion_velocity() * menu.style().glass_stretch_per_velocity;
    assert(std::fabs(menu.frame_jelly_offset() - expected_glide_jelly) < 0.001f);
    menu.draw(canvas, 752);
    assert(menu.glide_position() == static_cast<int>(menu.style().row_height));
    for (std::uint32_t t = 768; t <= 944; t += 16) menu.draw(canvas, t);
    assert(std::fabs(menu.frame_motion_velocity()) < 0.1f);

    // Width changes independently for GlideFrame.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::GlideFrame);
    menu.draw(canvas, 960);
    const int full_width = menu.glide_width();
    menu.on_key(epui::Key::Next);
    menu.on_key(epui::Key::Next);
    menu.on_key(epui::Key::Next);
    menu.draw(canvas, 976);
    assert(menu.glide_target_width() < full_width);
    int previous_width = menu.glide_width();
    for (std::uint32_t t = 992; t <= 1424; t += 16) {
        menu.draw(canvas, t);
        assert(menu.glide_width() <= previous_width);
        assert(menu.glide_width() >= menu.glide_target_width());
        previous_width = menu.glide_width();
    }
    assert(menu.glide_width() == menu.glide_target_width());

    // SlideFrame uses the same velocity-driven jelly geometry but remains full width.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::SlideFrame);
    menu.draw(canvas, 1440);
    menu.on_key(epui::Key::Next);
    menu.draw(canvas, 1456);
    assert(std::fabs(menu.frame_motion_velocity()) > 0.5f);
    assert(menu.glide_target_width() == static_cast<int>(menu.style().glass_width));

    // LiquidGlass uses the same jelly geometry plus a motion-only sheen.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::LiquidGlass);
    epui::Canvas liquid_rest;
    menu.draw(liquid_rest, 1472);
    menu.on_key(epui::Key::Next);
    epui::Canvas liquid_moving;
    menu.draw(liquid_moving, 1488);
    assert(std::fabs(menu.frame_motion_velocity()) > 0.05f);
    assert(std::memcmp(liquid_rest.data(), liquid_moving.data(), epui::Canvas::BufferSize) != 0);
    for (std::uint32_t t = 1504; t <= 2240; t += 16) menu.draw(liquid_moving, t);
    epui::Canvas liquid_settled;
    liquid_settled.clear();
    menu.draw(liquid_settled, 2256);
    assert(!menu.jelly_active());

    // Choice is a reusable enum-style menu item.
    epui::Ui choice_ui;
    epui::MenuPagePlugin<4> choice_page(choice_ui, choice_menu, "choice-menu");
    assert(choice_page.start());
    assert(choice_index == 0);
    assert(choice_page.activate_selected());
    assert(choice_index == 1);
    assert(choice_page.activate_selected());
    assert(choice_index == 2);
    assert(choice_page.activate_selected());
    assert(choice_index == 0);
    choice_page.stop();

    // A fourth row at y=55 on a 64 px display is still drawable. The old
    // y > Height-10 cutoff incorrectly hid it; now the Canvas is allowed to
    // render every pixel that still intersects the physical screen.
    epui::MenuStyle partial_style{};
    partial_style.selection_style = epui::MenuSelectionStyle::SlideFrame;
    partial_style.visible_rows = 4;
    partial_style.row_height = 13;
    epui::Ui partial_ui;
    epui::MenuPagePlugin<4> partial_page(partial_ui, long_menu, "partial-menu", partial_style);
    assert(partial_page.start());
    epui::Canvas partial_canvas;
    partial_page.draw(partial_canvas, 0);
    assert(pixel_count_band(partial_canvas, 55, 63) > 0);
    partial_page.stop();

    // A menu longer than the viewport must scroll gradually instead of jumping.
    epui::MenuStyle long_style{};
    long_style.selection_style = epui::MenuSelectionStyle::GlideFrame;
    long_style.visible_rows = 4;
    long_style.row_height = 10;
    long_style.glide_tick_ms = 16;
    long_style.glide_scroll_fast_step = 4;
    long_style.glide_scroll_slow_zone = 4;
    epui::Ui long_ui;
    epui::MenuPagePlugin<4> long_page(long_ui, long_menu, "long-menu", long_style);
    assert(long_page.start());
    epui::Canvas long_canvas;
    std::uint32_t now = 0;
    long_page.draw(long_canvas, now);

    for (int i = 0; i < 3; ++i) {
        long_page.on_key(epui::Key::Next);
        now += 16;
        long_page.draw(long_canvas, now);
        now += 16;
        long_page.draw(long_canvas, now);
    }
    assert(long_page.selected_index() == 3);
    assert(long_page.glide_scroll_position() == 0);

    long_page.on_key(epui::Key::Next);
    assert(long_page.glide_scroll_target() == 10);
    now += 16;
    long_page.draw(long_canvas, now);
    assert(long_page.glide_scroll_position() == 4);
    now += 16;
    long_page.draw(long_canvas, now);
    assert(long_page.glide_scroll_position() == 8);
    now += 16;
    long_page.draw(long_canvas, now);
    assert(long_page.glide_scroll_position() == 9);
    now += 16;
    long_page.draw(long_canvas, now);
    assert(long_page.glide_scroll_position() == 10);
    long_page.stop();

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
