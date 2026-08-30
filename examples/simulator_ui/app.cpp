#include "app.hpp"

namespace epui::demo {

SimulatorUi::SimulatorUi()
    : menu_(ui_, demo_menu_root(), "demo-menu", demo_menu_style(), false),
      popup_(ui_, "demo-popup"),
      graphics_(popup_),
      diagnostics_(ui_, "demo-diagnostics") {
    ui_.add_page(home_);
    ui_.add_page(sensors_);
    ui_.add_page(about_);
    bind_demo_menu(&menu_);
    bind_demo_diagnostics(&diagnostics_);
    menu_.start();
    ui_.add_page(terminal_);
    ui_.add_page(graphics_);
    diagnostics_.start();
    popup_.start();
}

SimulatorUi::~SimulatorUi() {
    bind_demo_diagnostics(nullptr);
    bind_demo_menu(nullptr);
    popup_.stop();
    diagnostics_.stop();
    menu_.stop();
}

} // namespace epui::demo
