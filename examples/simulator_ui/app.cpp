#include "app.hpp"

namespace epui::demo {

SimulatorUi::SimulatorUi() {
    ui_.add_page(home_);
    ui_.add_page(sensors_);
    ui_.add_page(about_);
}

} // namespace epui::demo
