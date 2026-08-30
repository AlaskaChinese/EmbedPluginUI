#include "app.hpp"
#include "epui/input_plugin.hpp"
#include <cassert>
#include <cstdint>
#include <string>

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

    assert(ui.page_count() == 6);
    assert(ui.page_index() == 0);

    // The top-level menu page is intentionally passive until Enter/Select.
    assert(!menu.focused());
    assert(!menu.captures_key(epui::Key::Next));
    assert(menu.captures_key(epui::Key::Select));

    move_page(app, epui::Key::Right, now);
    assert(ui.page_index() == 1);
    move_page(app, epui::Key::Right, now);
    assert(ui.page_index() == 2);
    move_page(app, epui::Key::Right, now);
    assert(ui.page_index() == 3);
    assert(!menu.focused());

    // Enter focuses the menu. Down is then consumed by the menu instead of
    // starting a top-level page transition.
    ui.handle(epui::Key::Select, now);
    assert(menu.focused());
    const std::size_t selected = menu.selected_index();
    ui.handle(epui::Key::Down, now);
    assert(menu.selected_index() != selected);
    assert(!ui.animating());
    assert(ui.page_index() == 3);
    ui.handle(epui::Key::Up, now);
    assert(menu.selected_index() == selected);
    ui.handle(epui::Key::Right, now);
    assert(menu.depth() == 2);
    ui.handle(epui::Key::Left, now);
    assert(menu.depth() == 1);

    // Back at the root releases focus. Next moves to the terminal demo.
    ui.handle(epui::Key::Back, now);
    assert(!menu.focused());
    assert(!menu.captures_key(epui::Key::Next));
    ui.handle(epui::Key::Right, now);
    assert(ui.animating());
    settle_transition(app, now);
    assert(ui.page_index() == 4);

    auto& terminal = app.terminal();
    ui.handle(epui::Key::Select, now);
    assert(terminal.focused());
    ui.handle(epui::InputEvent{epui::Key::Select, true, 'h'}, now);
    ui.handle(epui::InputEvent{epui::Key::Select, true, 'i'}, now);
    ui.handle(epui::Key::Left, now);
    ui.handle(epui::InputEvent{epui::Key::Select, true, '!'}, now);
    assert(std::string(terminal.command()) == "h!i");
    ui.handle(epui::Key::Select, now);
    assert(terminal.command_length() == 0);
    assert(std::string(terminal.view().line(terminal.view().line_count() - 2)) == "$ h!i");
    ui.handle(epui::Key::Up, now);
    assert(std::string(terminal.command()) == "h!i");
    ui.handle(epui::Key::Down, now);
    assert(terminal.command_length() == 0);
    ui.handle(epui::Key::Back, now);
    assert(!terminal.focused());
    ui.handle(epui::Key::Right, now);
    assert(ui.animating());
    settle_transition(app, now);
    assert(ui.page_index() == 5);

    auto& popup = app.popup();
    ui.handle(epui::Key::Select, now);
    assert(popup.visible());
    ui.handle(epui::Key::Right, now);
    assert(popup.selected_index() == 1);
    assert(!ui.animating() && ui.page_index() == 5);
    ui.handle(epui::Key::Left, now);
    assert(popup.selected_index() == 0);
    ui.handle(epui::Key::Select, now);
    for (int i = 0; i < 240 && popup.visible(); ++i) {
        now += 16;
        ui.render(app.canvas(), now);
    }
    assert(!popup.visible());
    assert(app.graphics().last_result() == epui::PopupResult::Accepted);

    move_page(app, epui::Key::Right, now);
    assert(ui.page_index() == 0);

    return 0;
}
