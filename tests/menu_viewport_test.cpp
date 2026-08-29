#include "epui/menu_plugin.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace {

bool pixel_on(const epui::Canvas& canvas, int x, int y) {
    const std::size_t index = static_cast<std::size_t>(x + (y / 8) * epui::Canvas::Width);
    return ((canvas.data()[index] >> (y & 7)) & 1u) != 0u;
}

bool row_has_pixels(const epui::Canvas& canvas, int y) {
    for (int x = 0; x < epui::Canvas::Width; ++x) {
        if (pixel_on(canvas, x, y)) return true;
    }
    return false;
}

const epui::MenuItem items[] = {
    epui::MenuItem::action("Top"),
    epui::MenuItem::action("Second"),
    epui::MenuItem::action("Third"),
    epui::MenuItem::action("Fourth"),
    epui::MenuItem::action("Fifth"),
    epui::MenuItem::action("Sixth"),
    epui::MenuItem::action("Seventh"),
    epui::MenuItem::action("Eighth"),
};
const epui::Menu menu_data = epui::make_menu("Viewport", items);

} // namespace

int main() {
    // Canvas clipping is pixel-exact and applies to all primitives that route
    // through pixel(), including text, rounded frames and invert effects.
    epui::Canvas clip_canvas;
    clip_canvas.set_clip_rect(0, 10, epui::Canvas::Width, 5);
    clip_canvas.fill_rect(0, 0, epui::Canvas::Width, epui::Canvas::Height, true);
    assert(!row_has_pixels(clip_canvas, 9));
    assert(row_has_pixels(clip_canvas, 10));
    assert(row_has_pixels(clip_canvas, 14));
    assert(!row_has_pixels(clip_canvas, 15));
    clip_canvas.reset_clip();
    clip_canvas.pixel(0, 0, true);
    assert(pixel_on(clip_canvas, 0, 0));

    epui::MenuStyle style{};
    style.selection_style = epui::MenuSelectionStyle::SlideFrame;
    style.content_top = 16;
    style.row_height = 13;
    style.visible_rows = 4;
    style.viewport_top = 13;
    style.viewport_bottom = 60;
    style.allow_partial_rows = true;
    style.glass_height = 11;
    style.glide_tick_ms = 16;
    style.glide_position_fast_step = 5;
    style.glide_position_slow_zone = 4;
    style.glide_scroll_fast_step = 4;
    style.glide_scroll_slow_zone = 4;

    epui::Ui ui;
    epui::MenuPagePlugin<4> page(ui, menu_data, "viewport-menu", style);
    assert(page.start());
    assert(page.viewport_scroll_target() == 0);

    epui::Canvas canvas;
    canvas.clear();
    page.draw(canvas, 0);

    // Rows 1..3 fit without moving the list. Selecting row 4 (index 3)
    // raises the exact pixel viewport by four pixels so the resting 11 px
    // frame ends at y=59, just above the y=60 navigation safe zone.
    page.on_key(epui::Key::Next);
    assert(page.selected_index() == 1);
    assert(page.viewport_scroll_target() == 0);
    page.on_key(epui::Key::Next);
    assert(page.selected_index() == 2);
    assert(page.viewport_scroll_target() == 0);
    page.on_key(epui::Key::Next);
    assert(page.selected_index() == 3);
    assert(page.first_visible_index() == 0);
    assert(page.viewport_scroll_target() == 4);
    assert(page.glide_scroll_target() == 4);

    for (std::uint32_t t = 16; t <= 1120; t += 16) {
        canvas.clear();
        page.draw(canvas, t);
    }

    assert(page.glide_scroll_position() == 4);
    assert(std::fabs(page.frame_motion_position() - 35.0f) < 0.15f);

    // With a 4 px scroll the first row starts at y=12. The body viewport starts
    // at y=13, so the row is cut horizontally but remains visibly present.
    assert(pixel_on(canvas, 13, 13));

    // The selected fourth frame is complete and terminates at y=59.
    assert(pixel_on(canvas, 10, 59));

    // The entire bottom navigation band stays untouched by menu drawing.
    for (int y = 60; y < epui::Canvas::Height; ++y) {
        assert(!row_has_pixels(canvas, y));
    }

    // Continue down. Sticky logical rows are preserved, but the exact target
    // carries the extra four-pixel safety offset needed by the 11 px frame.
    page.on_key(epui::Key::Next);
    assert(page.selected_index() == 4);
    assert(page.first_visible_index() == 1);
    assert(page.viewport_scroll_target() == 17);
    page.on_key(epui::Key::Next);
    assert(page.selected_index() == 5);
    assert(page.first_visible_index() == 2);
    assert(page.viewport_scroll_target() == 30);

    // Upward movement is frame-first: items already inside the sticky viewport
    // do not move the list. The cursor climbs while the exact scroll stays 30.
    page.on_key(epui::Key::Prev);
    assert(page.selected_index() == 4);
    assert(page.first_visible_index() == 2);
    assert(page.viewport_scroll_target() == 30);
    page.on_key(epui::Key::Prev);
    assert(page.selected_index() == 3);
    assert(page.first_visible_index() == 2);
    assert(page.viewport_scroll_target() == 30);

    // Only when the selected resting frame would cross the top comfort anchor
    // does the list begin moving downward again, and only by the required amount.
    page.on_key(epui::Key::Prev);
    assert(page.selected_index() == 2);
    assert(page.first_visible_index() == 2);
    assert(page.viewport_scroll_target() == 26);
    page.on_key(epui::Key::Prev);
    assert(page.selected_index() == 1);
    assert(page.first_visible_index() == 1);
    assert(page.viewport_scroll_target() == 13);
    page.on_key(epui::Key::Prev);
    assert(page.selected_index() == 0);
    assert(page.first_visible_index() == 0);
    assert(page.viewport_scroll_target() == 0);

    page.stop();
    return 0;
}
