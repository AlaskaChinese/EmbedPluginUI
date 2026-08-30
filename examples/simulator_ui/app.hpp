#pragma once

#include "about_page.hpp"
#include "epui/canvas.hpp"
#include "epui/diagnostics_plugin.hpp"
#include "epui/fps_debug_plugin.hpp"
#include "epui/menu_plugin.hpp"
#include "epui/page.hpp"
#include "home_page.hpp"
#include "graphics_page.hpp"
#include "menu_demo.hpp"
#include "sensor_page.hpp"
#include "terminal_page.hpp"

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
    TerminalDemoPage& terminal() { return terminal_; }
    GraphicsDemoPage& graphics() { return graphics_; }
    PopupPlugin& popup() { return popup_; }
    DiagnosticsPlugin& diagnostics() { return diagnostics_; }
    const DiagnosticsPlugin& diagnostics() const { return diagnostics_; }

    // Compatibility accessor retained for callers written against the original
    // FPS-only debug plugin.
    FpsDebugPlugin& fps_debug() { return diagnostics_; }

private:
    Canvas canvas_;
    Ui ui_;
    HomePage home_;
    SensorPage sensors_;
    AboutPage about_;
    MenuPagePlugin<12> menu_;
    TerminalDemoPage terminal_;
    PopupPlugin popup_;
    GraphicsDemoPage graphics_;
    DiagnosticsPlugin diagnostics_;
};

} // namespace epui::demo
