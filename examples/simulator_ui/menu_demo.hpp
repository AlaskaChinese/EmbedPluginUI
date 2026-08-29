#pragma once

#include "epui/menu_plugin.hpp"

namespace epui::demo {

const Menu& demo_menu_root();
void reset_demo_menu_state();
void bind_demo_menu(MenuPagePlugin<12>* menu);

} // namespace epui::demo
