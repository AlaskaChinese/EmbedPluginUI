#pragma once

#include "about_page.hpp"
#include "epui/canvas.hpp"
#include "epui/fps_debug_plugin.hpp"
#include "epui/menu_plugin.hpp"
#include "epui/page.hpp"
#include "home_page.hpp"
#include "menu_demo.hpp"
#include "sensor_page.hpp"

namespace epui::demo {

class SimulatorUi {
public:
    SimulatorUi();
    ~SimulatorUi();

    Canvas& canvas() { return canvas_; }
    const Canvas& canvas() const { return canvas_; }
    Ui& ui() { return ui_; }
    const Ui& ui() const { return ui_; }
    MenuPagePlugin<12>& menu() { return menu_; }
    FpsDebugPlugin& fps_debug() { return fps_debug_; }

private:
    Canvas canvas_;
    Ui ui_;
    HomePage home_;
    SensorPage sensors_;
    AboutPage about_;
    MenuPagePlugin<12> menu_;
    FpsDebugPlugin fps_debug_;
};

} // namespace epui::demo
