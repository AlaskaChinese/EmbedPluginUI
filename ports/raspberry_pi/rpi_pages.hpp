#pragma once

#include "epui/page_plugin.hpp"
#include "rpi_plugins.hpp"

namespace openoledui::rpi {

class OverviewPage final : public epui::PagePlugin {
public:
    OverviewPage(epui::Ui& ui, const SystemMonitorPlugin& system) : PagePlugin(ui, "page-overview"), system_(system) {}
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const char* dependency_[1]{"system-monitor"};
    const SystemMonitorPlugin& system_;
};

class NetworkPage final : public epui::PagePlugin {
public:
    NetworkPage(epui::Ui& ui, const SystemMonitorPlugin& system) : PagePlugin(ui, "page-network"), system_(system) {}
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const char* dependency_[1]{"system-monitor"};
    const SystemMonitorPlugin& system_;
};

class PowerPage final : public epui::PagePlugin {
public:
    PowerPage(epui::Ui& ui, const SystemMonitorPlugin& system) : PagePlugin(ui, "page-power"), system_(system) {}
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const char* dependency_[1]{"system-monitor"};
    const SystemMonitorPlugin& system_;
};

class SystemPage final : public epui::PagePlugin {
public:
    SystemPage(epui::Ui& ui, const SystemMonitorPlugin& system) : PagePlugin(ui, "page-system"), system_(system) {}
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const char* dependency_[1]{"system-monitor"};
    const SystemMonitorPlugin& system_;
};

class TerminalPage final : public epui::PagePlugin {
public:
    TerminalPage(epui::Ui& ui, const TerminalFeedPlugin& terminal) : PagePlugin(ui, "page-terminal"), terminal_(terminal) {}
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const char* dependency_[1]{"terminal-feed"};
    const TerminalFeedPlugin& terminal_;
};

} // namespace openoledui::rpi
