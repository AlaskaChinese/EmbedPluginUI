#pragma once

#include "epui/sensor_plugin.hpp"
#include "epui/service_plugin.hpp"
#include "epui/terminal_view.hpp"
#include "system_monitor.hpp"
#include "terminal_feed.hpp"

namespace epui::rpi {

class SystemMonitorPlugin final : public epui::SensorPlugin<StatusSnapshot> {
public:
    explicit SystemMonitorPlugin(std::uint32_t interval_ms = 1000) : SensorPlugin(interval_ms) {}
    const char* name() const override { return "system-monitor"; }

protected:
    bool sample(StatusSnapshot& out, std::uint32_t) override {
        out = monitor_.sample();
        return true;
    }

private:
    SystemMonitor monitor_;
};

class TerminalFeedPlugin final : public epui::ServicePlugin {
public:
    explicit TerminalFeedPlugin(const char* path = "/tmp/epui-terminal") : feed_(view_, path) {
        view_.set_cursor_visible(false);
    }
    const char* name() const override { return "terminal-feed"; }
    bool start() override { return feed_.open_feed(); }
    void tick(std::uint32_t) override { feed_.poll(); }
    const TerminalFeed& feed() const { return feed_; }
    TerminalFeed& feed() { return feed_; }
    const TerminalFeed::View& view() const { return view_; }
    TerminalFeed::View& view() { return view_; }

private:
    TerminalFeed::View view_;
    TerminalFeed feed_;
};

} // namespace epui::rpi
