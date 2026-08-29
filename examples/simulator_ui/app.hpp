#pragma once

#include "about_page.hpp"
#include "epui/canvas.hpp"
#include "epui/page.hpp"
#include "home_page.hpp"
#include "sensor_page.hpp"

namespace epui::demo {

class SimulatorUi {
public:
    SimulatorUi();

    Canvas& canvas() { return canvas_; }
    const Canvas& canvas() const { return canvas_; }
    Ui& ui() { return ui_; }
    const Ui& ui() const { return ui_; }

private:
    Canvas canvas_;
    Ui ui_;
    HomePage home_;
    SensorPage sensors_;
    AboutPage about_;
};

} // namespace epui::demo
