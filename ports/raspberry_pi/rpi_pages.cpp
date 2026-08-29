#include "rpi_pages.hpp"
#include "openoledui/widgets.hpp"
#include <cstdio>

namespace openoledui::rpi {
namespace {
void fmt(char* b, std::size_t n, float v, const char* suffix, int decimals = 0) {
    if (v < 0) std::snprintf(b, n, "N/A");
    else if (decimals > 0) std::snprintf(b, n, "%.*f%s", decimals, v, suffix);
    else std::snprintf(b, n, "%.0f%s", v, suffix);
}
void kv(epui::Canvas& c, int y, const char* key, const char* value) {
    c.text(3, y, key);
    c.text(48, y, value);
}
}

void OverviewPage::draw(epui::Canvas& c, std::uint32_t) {
    const auto& s = system_.snapshot();
    openoledui::draw_header(c, "Pi 5", 1, 5);
    char b[24];
    fmt(b, sizeof(b), s.temperature_c, "C", 1); kv(c, 17, "TEMP", b);
    fmt(b, sizeof(b), s.cpu_percent, "%"); kv(c, 27, "CPU", b);
    fmt(b, sizeof(b), s.memory_percent, "%"); kv(c, 37, "RAM", b);
    std::snprintf(b, sizeof(b), "%.2f", s.load1); kv(c, 47, "LOAD", b);
}

void NetworkPage::draw(epui::Canvas& c, std::uint32_t) {
    const auto& s = system_.snapshot();
    openoledui::draw_header(c, "Network", 2, 5);
    openoledui::draw_wifi_icon(c, 107, 18, 3);
    kv(c, 17, "IF", s.interface.c_str());
    kv(c, 27, "IP", s.ipv4.c_str());
    char b[24];
    std::snprintf(b, sizeof(b), "%.1f KiB/s", s.rx_kib_s); kv(c, 37, "RX", b);
    std::snprintf(b, sizeof(b), "%.1f KiB/s", s.tx_kib_s); kv(c, 47, "TX", b);
}

void PowerPage::draw(epui::Canvas& c, std::uint32_t now) {
    const auto& s = system_.snapshot();
    openoledui::draw_header(c, "Power", 3, 5);
    char b[24];
    fmt(b, sizeof(b), s.supply_voltage_v, "V", 2); kv(c, 18, "EXT5V", b);
    fmt(b, sizeof(b), s.core_current_a, "A", 2); kv(c, 29, "CORE I", b);
    std::snprintf(b, sizeof(b), "0x%05X", s.throttled); kv(c, 40, "FLAGS", b);
    c.text(3, 51, s.throttled ? "CHECK POWER/THERM" : "POWER OK");
    if (s.throttled) openoledui::draw_spinner(c, 119, 52, now);
}

void SystemPage::draw(epui::Canvas& c, std::uint32_t) {
    const auto& s = system_.snapshot();
    openoledui::draw_header(c, "System", 4, 5);
    kv(c, 16, "USER", s.user.c_str());
    kv(c, 25, "HOST", s.hostname.c_str());
    char b[24];
    std::snprintf(b, sizeof(b), "%lluh", static_cast<unsigned long long>(s.uptime_s / 3600)); kv(c, 34, "UP", b);
    fmt(b, sizeof(b), s.disk_percent, "%"); kv(c, 43, "DISK", b);
    c.progress_bar(3, 53, 122, 6, s.disk_percent < 0 ? 0 : s.disk_percent / 100.0f);
}

void TerminalPage::draw(epui::Canvas& c, std::uint32_t) {
    openoledui::draw_header(c, "Terminal", 5, 5);
    int y = 15;
    for (const auto& line : terminal_.feed().lines()) {
        c.text(3, y, line.c_str());
        y += 7;
    }
}

} // namespace openoledui::rpi
