#include "app.hpp"
#include <cassert>
#include <cstdint>

namespace {

void settle_transition(epui::demo::SimulatorUi& app, std::uint32_t& now) {
    for (int i = 0; i < 160 && app.ui().animating(); ++i) {
        now += 16;
        app.ui().render(app.canvas(), now);
    }
    assert(!app.ui().animating());
}

void move_page(epui::demo::SimulatorUi& app, epui::Key key, std::uint32_t& now) {
    app.ui().handle(key, now);
    assert(app.ui().animating());
    settle_transition(app, now);
}

} // namespace

int main() {
    epui::demo::SimulatorUi app;
    auto& ui = app.ui();
    auto& menu = app.menu();
    std::uint32_t now = 0;

    assert(ui.page_count() == 4);
    assert(ui.page_index() == 0);

    // The top-level menu page is intentionally passive until Enter/Select.
    assert(!menu.focused());
    assert(!menu.captures_key(epui::Key::Next));
    assert(menu.captures_key(epui::Key::Select));

    move_page(app, epui::Key::Next, now);
    assert(ui.page_index() == 1);
    move_page(app, epui::Key::Next, now);
    assert(ui.page_index() == 2);
    move_page(app, epui::Key::Next, now);
    assert(ui.page_index() == 3);
    assert(!menu.focused());

    // Enter focuses the menu. Next is then consumed by the menu instead of
    // starting a top-level page transition.
    ui.handle(epui::Key::Select, now);
    assert(menu.focused());
    const std::size_t selected = menu.selected_index();
    ui.handle(epui::Key::Next, now);
    assert(menu.selected_index() != selected);
    assert(!ui.animating());
    assert(ui.page_index() == 3);

    // Back at the root releases focus. Next returns to top-level page motion.
    ui.handle(epui::Key::Back, now);
    assert(!menu.focused());
    assert(!menu.captures_key(epui::Key::Next));
    ui.handle(epui::Key::Next, now);
    assert(ui.animating());
    settle_transition(app, now);
    assert(ui.page_index() == 0);

    return 0;
}
