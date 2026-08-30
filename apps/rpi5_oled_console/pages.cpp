#include "pages.hpp"
#include "epui/widgets.hpp"
#include <algorithm>
#include <cstdio>

namespace epui::rpi::console {
namespace {

void format_value(char* buffer, std::size_t size, float value,
                  const char* suffix, int decimals = 0) {
    if (value < 0) std::snprintf(buffer, size, "N/A");
    else if (decimals > 0) std::snprintf(buffer, size, "%.*f%s", decimals, value, suffix);
    else std::snprintf(buffer, size, "%.0f%s", value, suffix);
}

void draw_key_value(epui::Canvas& canvas, int y, const char* key, const char* value) {
    canvas.text(3, y, key);
    canvas.text(48, y, value);
}

} // namespace

void OverviewPage::draw(epui::Canvas& canvas, std::uint32_t) {
    const auto& status = system_.snapshot();
    epui::draw_header(canvas, "Pi 5", 1, 5);
    char buffer[24];
    format_value(buffer, sizeof(buffer), status.temperature_c, "C", 1);
    draw_key_value(canvas, 17, "TEMP", buffer);
    format_value(buffer, sizeof(buffer), status.cpu_percent, "%");
    draw_key_value(canvas, 27, "CPU", buffer);
    format_value(buffer, sizeof(buffer), status.memory_percent, "%");
    draw_key_value(canvas, 37, "RAM", buffer);
    std::snprintf(buffer, sizeof(buffer), "%.2f", status.load1);
    draw_key_value(canvas, 47, "LOAD", buffer);
}

void NetworkPage::draw(epui::Canvas& canvas, std::uint32_t) {
    const auto& status = system_.snapshot();
    epui::draw_header(canvas, "Network", 2, 5);
    epui::draw_wifi_icon(canvas, 107, 18, 3);
    draw_key_value(canvas, 17, "IF", status.interface.c_str());
    draw_key_value(canvas, 27, "IP", status.ipv4.c_str());
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "%.1f KiB/s", status.rx_kib_s);
    draw_key_value(canvas, 37, "RX", buffer);
    std::snprintf(buffer, sizeof(buffer), "%.1f KiB/s", status.tx_kib_s);
    draw_key_value(canvas, 47, "TX", buffer);
}

void PowerPage::draw(epui::Canvas& canvas, std::uint32_t now_ms) {
    const auto& status = system_.snapshot();
    epui::draw_header(canvas, "Power", 3, 5);
    char buffer[24];
    format_value(buffer, sizeof(buffer), status.supply_voltage_v, "V", 2);
    draw_key_value(canvas, 18, "EXT5V", buffer);
    format_value(buffer, sizeof(buffer), status.core_current_a, "A", 2);
    draw_key_value(canvas, 29, "CORE I", buffer);
    std::snprintf(buffer, sizeof(buffer), "0x%05X", status.throttled);
    draw_key_value(canvas, 40, "FLAGS", buffer);
    canvas.text(3, 51, status.throttled ? "CHECK POWER/THERM" : "POWER OK");
    if (status.throttled) epui::draw_spinner(canvas, 119, 52, now_ms);
}

void SystemPage::draw(epui::Canvas& canvas, std::uint32_t) {
    const auto& status = system_.snapshot();
    epui::draw_header(canvas, "System", 4, 5);
    draw_key_value(canvas, 16, "USER", status.user.c_str());
    draw_key_value(canvas, 25, "HOST", status.hostname.c_str());
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "%lluh",
                  static_cast<unsigned long long>(status.uptime_s / 3600));
    draw_key_value(canvas, 34, "UP", buffer);
    format_value(buffer, sizeof(buffer), status.disk_percent, "%");
    draw_key_value(canvas, 43, "DISK", buffer);
    canvas.progress_bar(3, 53, 122, 6,
                        status.disk_percent < 0 ? 0 : status.disk_percent / 100.0f);
}

bool TerminalPage::captures_key(epui::Key key) const {
    if (key == epui::Key::Select) return true;
    return focused_ && (key == epui::Key::Next || key == epui::Key::Prev
                        || key == epui::Key::Back || key == epui::Key::ScrollUp
                        || key == epui::Key::ScrollDown);
}

void TerminalPage::on_key(epui::Key key) {
    if (!focused_) {
        if (key == epui::Key::Select) {
            if (!shell_.running()) shell_.start();
            focused_ = true;
        }
        return;
    }
    if (key == epui::Key::Back) {
        focused_ = false;
    } else if (key == epui::Key::Select) {
        execute();
    } else if (key == epui::Key::Next) {
        if (cursor_ < length_) ++cursor_;
    } else if (key == epui::Key::Prev) {
        if (cursor_ > 0) --cursor_;
    } else if (key == epui::Key::ScrollUp) {
        shell_.view().scroll(7);
    } else if (key == epui::Key::ScrollDown) {
        shell_.view().scroll(-7);
    }
}

void TerminalPage::on_char(char ch) {
    if (!focused_) return;
    const auto byte = static_cast<unsigned char>(ch);
    if (ch == '\b' || ch == '\x7f') {
        erase_before_cursor();
    } else if (ch == '\t') {
        for (int i = 0; i < 4; ++i) insert(' ');
    } else if (byte >= 0x20 && byte <= 0x7e) {
        insert(ch);
    } else if (ch != 0) {
        shell_.send(ch);
    }
}

void TerminalPage::draw(epui::Canvas& canvas, std::uint32_t now_ms) {
    canvas.text(1, 1, ">");

    std::size_t first = 0;
    if (cursor_ >= VisibleColumns) first = cursor_ - VisibleColumns + 1;
    const std::size_t last = std::min(length_, first + VisibleColumns);
    for (std::size_t i = first; i < last; ++i) {
        canvas.glyph5x7(8 + static_cast<int>((i - first) * 6), 1, command_[i]);
    }

    if (focused_ && ((now_ms / 500u) & 1u) == 0u) {
        const std::size_t visible_cursor = cursor_ - first;
        canvas.invert_rect(8 + static_cast<int>(visible_cursor * 6), 1, 6, 7);
    }

    canvas.line(0, 9, 127, 9);
    shell_.view().draw(canvas, 1, 11, 126, 49, now_ms);
}

void TerminalPage::insert(char ch) {
    if (length_ >= CommandCapacity) return;
    for (std::size_t i = length_; i > cursor_; --i) command_[i] = command_[i - 1];
    command_[cursor_++] = ch;
    command_[++length_] = 0;
}

void TerminalPage::erase_before_cursor() {
    if (cursor_ == 0) return;
    for (std::size_t i = cursor_ - 1; i < length_; ++i) command_[i] = command_[i + 1];
    --cursor_;
    --length_;
}

void TerminalPage::execute() {
    if (!shell_.running() && !shell_.start()) return;
    shell_.view().clear();
    if (length_ != 0) shell_.send(command_, length_);
    shell_.send('\r');
    length_ = 0;
    cursor_ = 0;
    command_[0] = 0;
}

} // namespace epui::rpi::console
