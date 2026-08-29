#pragma once

#include "epui/page_plugin.hpp"
#include "rpi_plugins.hpp"

namespace openoledui::rpi {

class OverviewPage final : public epui::PagePlugin {
public:
    OverviewPage(epui::Ui& ui, const SystemMonitorPlugin& system) : PagePlugin(ui, "page-overview"), system_(system) {}
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const SystemMonitorPlugin& system_;
};

class NetworkPage final : public epui::PagePlugin {
public:
    NetworkPage(epui::Ui& ui, const SystemMonitorPlugin& system) : PagePlugin(ui, "page-network"), system_(system) {}
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const SystemMonitorPlugin& system_;
};

class PowerPage final : public epui::PagePlugin {
public:
    PowerPage(epui::Ui& ui, const SystemMonitorPlugin& system) : PagePlugin(ui, "page-power"), system_(system) {}
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const SystemMonitorPlugin& system_;
};

class SystemPage final : public epui::PagePlugin {
public:
    SystemPage(epui::Ui& ui, const SystemMonitorPlugin& system) : PagePlugin(ui, "page-system"), system_(system) {}
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const SystemMonitorPlugin& system_;
};

class TerminalPage final : public epui::PagePlugin {
public:
    TerminalPage(epui::Ui& ui, const TerminalFeedPlugin& terminal) : PagePlugin(ui, "page-terminal"), terminal_(terminal) {}
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const TerminalFeedPlugin& terminal_;
};

} // namespace openoledui::rpi
