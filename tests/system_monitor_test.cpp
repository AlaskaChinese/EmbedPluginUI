#include "rpi_plugins.hpp"
#include <cassert>

int main() {
    epui::rpi::SystemMonitorPlugin monitor(1000);
    assert(monitor.section() == epui::rpi::StatusSection::Inactive);

    monitor.tick(0);
    assert(monitor.sample_count() == 0);

    monitor.set_section(epui::rpi::StatusSection::Overview);
    monitor.tick(1);
    assert(monitor.sample_count() == 1);
    monitor.tick(500);
    assert(monitor.sample_count() == 1);

    monitor.set_section(epui::rpi::StatusSection::Network);
    monitor.tick(501);
    assert(monitor.sample_count() == 2);

    monitor.set_section(epui::rpi::StatusSection::Inactive);
    monitor.tick(502);
    assert(monitor.sample_count() == 2);
    return 0;
}
