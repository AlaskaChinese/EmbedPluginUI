#pragma once

#include "epui/diagnostics_plugin.hpp"
#include "epui/fps_debug_plugin.hpp"
#include "epui/menu_plugin.hpp"

namespace epui::demo {

const Menu& demo_menu_root();
MenuStyle demo_menu_style();
void reset_demo_menu_state();
void bind_demo_menu(MenuPagePlugin<12>* menu);
void bind_demo_diagnostics(DiagnosticsPlugin* diagnostics);

inline void bind_demo_fps_debug(FpsDebugPlugin* fps) {
    bind_demo_diagnostics(fps);
}

} // namespace epui::demo
