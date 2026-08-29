#include "app.hpp"

namespace epui::demo {

SimulatorUi::SimulatorUi()
    : menu_(ui_, demo_menu_root(), "demo-menu"),
      fps_debug_(ui_, "demo-fps") {
    ui_.add_page(home_);
    ui_.add_page(sensors_);
    ui_.add_page(about_);
    bind_demo_menu(&menu_);
    bind_demo_fps_debug(&fps_debug_);
    menu_.start();
    fps_debug_.start();
}

SimulatorUi::~SimulatorUi() {
    bind_demo_fps_debug(nullptr);
    bind_demo_menu(nullptr);
    fps_debug_.stop();
    menu_.stop();
}

} // namespace epui::demo
