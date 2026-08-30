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
    assert(menu.style().scroll_handoff_kick > 0.0f);
    assert(menu.captures_key(epui::Key::Up));
    assert(menu.captures_key(epui::Key::Down));
    assert(menu.captures_key(epui::Key::Left));
    assert(menu.captures_key(epui::Key::Right));

    menu.on_key(epui::Key::Down);
    assert(menu.selected_index() == 1);
    menu.on_key(epui::Key::Up);
    assert(menu.selected_index() == 0);
    menu.on_key(epui::Key::Right);
    assert(menu.depth() == 2);
    menu.on_key(epui::Key::Left);
    assert(menu.depth() == 1);
    menu.reset_to_root(true);

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

    // GlideFrame keeps the deterministic two-speed path as its virtual target,
    // but the visible frame now follows that path through the same damped
    // spring as Glass. It must visibly overshoot and then settle.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::GlideFrame);
    menu.draw(canvas, 720);
    assert(menu.glide_position() == 0);
    assert(std::fabs(menu.frame_motion_position()) < 0.01f);
    menu.on_key(epui::Key::Next);
    assert(menu.glide_target_position() == static_cast<int>(menu.style().row_height));
    menu.draw(canvas, 736);
    assert(menu.glide_position() == 5);
    assert(menu.frame_motion_target() == 5.0f);
    assert(menu.frame_motion_position() > 0.0f);
    assert(menu.frame_motion_position() < menu.frame_motion_target());
    menu.draw(canvas, 752);
    assert(menu.glide_position() == static_cast<int>(menu.style().row_height));
    assert(menu.frame_motion_target() == static_cast<float>(menu.style().row_height));

    float glide_maximum = menu.frame_motion_position();
    for (std::uint32_t t = 768; t <= 1360; t += 16) {
        menu.draw(canvas, t);
        glide_maximum = std::max(glide_maximum, menu.frame_motion_position());
    }
    assert(glide_maximum > static_cast<float>(menu.style().row_height) + 0.5f);
    assert(std::fabs(menu.frame_motion_position() - static_cast<float>(menu.style().row_height)) < 0.1f);
    assert(std::fabs(menu.frame_motion_velocity()) < 0.1f);

    // Width still changes independently for GlideFrame while the position uses
    // the spring follower.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::GlideFrame);
    menu.draw(canvas, 1376);
    const int full_width = menu.glide_width();
    menu.on_key(epui::Key::Next);
    menu.on_key(epui::Key::Next);
    menu.on_key(epui::Key::Next);
    menu.draw(canvas, 1392);
    assert(menu.glide_target_width() < full_width);
    int previous_width = menu.glide_width();
    for (std::uint32_t t = 1408; t <= 1840; t += 16) {
        menu.draw(canvas, t);
        assert(menu.glide_width() <= previous_width);
        assert(menu.glide_width() >= menu.glide_target_width());
        previous_width = menu.glide_width();
    }
    assert(menu.glide_width() == menu.glide_target_width());

    // SlideFrame uses the same virtual path + same visible spring as GlideFrame,
    // but remains full width. It must independently demonstrate overshoot.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::SlideFrame);
    menu.draw(canvas, 1856);
    menu.on_key(epui::Key::Next);
    float slide_maximum = menu.frame_motion_position();
    for (std::uint32_t t = 1872; t <= 2464; t += 16) {
        menu.draw(canvas, t);
        slide_maximum = std::max(slide_maximum, menu.frame_motion_position());
    }
    assert(slide_maximum > static_cast<float>(menu.style().row_height) + 0.5f);
    assert(std::fabs(menu.frame_motion_position() - static_cast<float>(menu.style().row_height)) < 0.1f);
    assert(std::fabs(menu.frame_motion_velocity()) < 0.1f);
    assert(menu.glide_target_width() == static_cast<int>(menu.style().glass_width));

    // LiquidGlass retains the same spring character plus a motion-only sheen.
    menu.reset_to_root(true);
    menu.set_selection_style(epui::MenuSelectionStyle::LiquidGlass);
    epui::Canvas liquid_rest;
    menu.draw(liquid_rest, 2480);
    menu.on_key(epui::Key::Next);
    epui::Canvas liquid_moving;
    menu.draw(liquid_moving, 2496);
    assert(std::fabs(menu.frame_motion_velocity()) > 0.05f);
    assert(std::memcmp(liquid_rest.data(), liquid_moving.data(), epui::Canvas::BufferSize) != 0);
    for (std::uint32_t t = 2512; t <= 3248; t += 16) menu.draw(liquid_moving, t);
    epui::Canvas liquid_settled;
    liquid_settled.clear();
    menu.draw(liquid_settled, 3264);
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

    // A fourth row at y=55 on a 64 px display is still drawable. The Canvas
    // clips only the pixels beyond the physical edge instead of hiding the row.
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

    // A menu longer than the viewport uses a sticky viewport. Crossing the
    // bottom edge scrolls the content gradually and gives the anchored frame
    // a spring handoff; moving back up within that viewport moves only the
    // frame and must not drag the whole list back down.
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
    assert(long_page.first_visible_index() == 0);
    assert(long_page.glide_scroll_position() == 0);

    // Let the frame settle exactly on the fourth/bottom slot before testing
    // the edge handoff.
    for (int i = 0; i < 48; ++i) {
        now += 16;
        long_page.draw(long_canvas, now);
    }
    const float bottom_slot = 3.0f * static_cast<float>(long_style.row_height);
    assert(std::fabs(long_page.frame_motion_position() - bottom_slot) < 0.1f);

    long_page.on_key(epui::Key::Next);
    assert(long_page.selected_index() == 4);
    assert(long_page.first_visible_index() == 1);
    assert(long_page.glide_scroll_target() == 10);
    assert(long_page.frame_motion_velocity() > 0.5f);

    now += 16;
    long_page.draw(long_canvas, now);
    assert(long_page.glide_scroll_position() == 4);
    assert(long_page.frame_motion_position() > bottom_slot);
    now += 16;
    long_page.draw(long_canvas, now);
    assert(long_page.glide_scroll_position() == 8);
    now += 16;
    long_page.draw(long_canvas, now);
    assert(long_page.glide_scroll_position() == 9);
    now += 16;
    long_page.draw(long_canvas, now);
    assert(long_page.glide_scroll_position() == 10);

    for (int i = 0; i < 56; ++i) {
        now += 16;
        long_page.draw(long_canvas, now);
    }
    assert(std::fabs(long_page.frame_motion_position() - bottom_slot) < 0.1f);
    assert(std::fabs(long_page.frame_motion_velocity()) < 0.1f);

    // Moving 5 -> 4 must keep the current viewport. Scroll remains at one row,
    // while the frame alone travels upward from slot 4 to slot 3.
    long_page.on_key(epui::Key::Prev);
    assert(long_page.selected_index() == 3);
    assert(long_page.first_visible_index() == 1);
    assert(long_page.glide_scroll_target() == 10);
    now += 16;
    long_page.draw(long_canvas, now);
    assert(long_page.glide_scroll_position() == 10);
    now += 16;
    long_page.draw(long_canvas, now);
    assert(long_page.glide_scroll_position() == 10);
    assert(long_page.frame_motion_target() < bottom_slot);

    // The viewport stays sticky while rows 2/1 are still visible. Only when
    // selection reaches item 0 does the window itself need to move back up.
    long_page.on_key(epui::Key::Prev);
    assert(long_page.selected_index() == 2);
    assert(long_page.first_visible_index() == 1);
    assert(long_page.glide_scroll_target() == 10);
    long_page.on_key(epui::Key::Prev);
    assert(long_page.selected_index() == 1);
    assert(long_page.first_visible_index() == 1);
    assert(long_page.glide_scroll_target() == 10);
    long_page.on_key(epui::Key::Prev);
    assert(long_page.selected_index() == 0);
    assert(long_page.first_visible_index() == 0);
    assert(long_page.glide_scroll_target() == 0);
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
