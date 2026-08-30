#pragma once

#include <cstddef>
#include "epui/page_plugin.hpp"
#include "epui/terminal_controls.hpp"
#include "epui/terminal_line_editor.hpp"
#include "pty_session.hpp"
#include "rpi_plugins.hpp"

namespace epui::rpi::console {

class OverviewPage final : public epui::PagePlugin {
public:
    OverviewPage(epui::Ui& ui, const SystemMonitorPlugin& system)
        : PagePlugin(ui, "page-overview"), system_(system) {}
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const char* dependency_[1]{"system-monitor"};
    const SystemMonitorPlugin& system_;
};

class NetworkPage final : public epui::PagePlugin {
public:
    NetworkPage(epui::Ui& ui, const SystemMonitorPlugin& system)
        : PagePlugin(ui, "page-network"), system_(system) {}
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const char* dependency_[1]{"system-monitor"};
    const SystemMonitorPlugin& system_;
};

class PowerPage final : public epui::PagePlugin {
public:
    PowerPage(epui::Ui& ui, const SystemMonitorPlugin& system)
        : PagePlugin(ui, "page-power"), system_(system) {}
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const char* dependency_[1]{"system-monitor"};
    const SystemMonitorPlugin& system_;
};

class SystemPage final : public epui::PagePlugin {
public:
    SystemPage(epui::Ui& ui, const SystemMonitorPlugin& system)
        : PagePlugin(ui, "page-system"), system_(system) {}
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    void draw(epui::Canvas&, std::uint32_t) override;
private:
    const char* dependency_[1]{"system-monitor"};
    const SystemMonitorPlugin& system_;
};

class TerminalPage final : public epui::PagePlugin {
public:
    TerminalPage(epui::Ui& ui, PtySessionPlugin& shell,
                 epui::TerminalControls controls = epui::TerminalControls{})
        : PagePlugin(ui, "page-terminal"), shell_(shell), controls_(controls) {
        shell_.view().set_cursor_visible(false);
    }
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    bool captures_key(epui::Key key) const override;
    void on_key(epui::Key key) override;
    void on_char(char ch) override;
    void draw(epui::Canvas&, std::uint32_t) override;
    bool focused() const { return focused_; }
    const char* command() const { return editor_.command(); }
    std::size_t command_length() const { return editor_.length(); }
    std::size_t cursor() const { return editor_.cursor(); }
    std::size_t history_count() const { return editor_.history_count(); }
    const epui::TerminalControls& controls() const { return controls_; }
    void set_controls(const epui::TerminalControls& controls) { controls_ = controls; }
private:
    static constexpr std::size_t CommandCapacity = 128;
    static constexpr std::size_t VisibleColumns = 20;

    void execute();

    const char* dependency_[1]{"shell-session"};
    PtySessionPlugin& shell_;
    epui::TerminalControls controls_{};
    epui::TerminalLineEditor<CommandCapacity, 16> editor_;
    bool focused_{false};
};

} // namespace epui::rpi::console
