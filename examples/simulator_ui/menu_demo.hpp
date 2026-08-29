#pragma once

#include "epui/fps_debug_plugin.hpp"
#include "epui/menu_plugin.hpp"

namespace epui::demo {

const Menu& demo_menu_root();
MenuStyle demo_menu_style();
void reset_demo_menu_state();
void bind_demo_menu(MenuPagePlugin<12>* menu);
void bind_demo_fps_debug(FpsDebugPlugin* fps);

} // namespace epui::demo
