#include "app.hpp"

namespace epui::demo {

SimulatorUi::SimulatorUi()
    : menu_(ui_, demo_menu_root(), "demo-menu") {
    ui_.add_page(home_);
    ui_.add_page(sensors_);
    ui_.add_page(about_);
    menu_.start();
}

SimulatorUi::~SimulatorUi() {
    menu_.stop();
}

} // namespace epui::demo
