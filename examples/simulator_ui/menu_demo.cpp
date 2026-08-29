#include "menu_demo.hpp"

namespace epui::demo {
namespace {

bool wifi_enabled = true;
bool dhcp_enabled = true;
bool debug_enabled = false;
bool fps_overlay = true;
bool sleep_enabled = true;
bool invert_enabled = false;
bool soft_theme = true;
bool glide_cursor = true;
int wifi_channel = 6;
int log_level = 2;
int brightness = 72;
int contrast = 205;
int radius = 4;
int battery_warn = 20;
MenuPagePlugin<12>* bound_menu = nullptr;
FpsDebugPlugin* bound_fps = nullptr;

void apply_cursor_style(void*) {
    if (!bound_menu) return;
    bound_menu->set_selection_style(glide_cursor
        ? MenuSelectionStyle::GlideFrame
        : MenuSelectionStyle::Indicator);
}

void apply_fps_overlay(void*) {
    if (bound_fps) bound_fps->set_visible(fps_overlay);
}

void demo_action(void*) {}

void reset_defaults(void*) {
    reset_demo_menu_state();
}

const MenuItem alert_items[] = {
    MenuItem::value("Warn %", battery_warn, 5, 50, 5),
};
const Menu alerts_menu = make_menu("Alerts", alert_items);

const MenuItem battery_items[] = {
    MenuItem::submenu("Alerts", alerts_menu),
    MenuItem::value("Warn %", battery_warn, 5, 50, 5),
};
const Menu battery_menu = make_menu("Battery", battery_items);

const MenuItem power_items[] = {
    MenuItem::submenu("Battery", battery_menu),
    MenuItem::toggle("Sleep", sleep_enabled),
};
const Menu power_menu = make_menu("Power", power_items);

const MenuItem debug_items[] = {
    MenuItem::toggle("FPS Overlay", fps_overlay, apply_fps_overlay),
    MenuItem::toggle("Verbose", debug_enabled),
};
const Menu debug_menu = make_menu("Debug", debug_items);

const MenuItem system_items[] = {
    MenuItem::submenu("Power", power_menu),
    MenuItem::submenu("Debug", debug_menu),
    MenuItem::value("Log Level", log_level, 0, 5),
};
const Menu system_menu = make_menu("System", system_items);

const MenuItem wifi_items[] = {
    MenuItem::toggle("Enabled", wifi_enabled),
    MenuItem::value("Channel", wifi_channel, 1, 13),
};
const Menu wifi_menu = make_menu("WiFi", wifi_items);

const MenuItem network_items[] = {
    MenuItem::submenu("WiFi", wifi_menu),
    MenuItem::toggle("DHCP", dhcp_enabled),
};
const Menu network_menu = make_menu("Network", network_items);

const MenuItem oled_items[] = {
    MenuItem::value("Brightness", brightness, 0, 100, 5),
    MenuItem::value("Contrast", contrast, 0, 255, 5),
    MenuItem::toggle("Invert", invert_enabled),
};
const Menu oled_menu = make_menu("OLED", oled_items);

const MenuItem theme_items[] = {
    MenuItem::toggle("Soft", soft_theme),
    MenuItem::value("Radius", radius, 0, 8),
    MenuItem::toggle("Glide Cursor", glide_cursor, apply_cursor_style),
};
const Menu theme_menu = make_menu("Theme", theme_items);

const MenuItem display_items[] = {
    MenuItem::submenu("OLED", oled_menu),
    MenuItem::submenu("Theme", theme_menu),
};
const Menu display_menu = make_menu("Display", display_items);

// Deliberately longer than one 128x64 page. Labels vary in length so the
// GlideFrame demo exercises Y movement, width interpolation and list scrolling
// at the same time.
const MenuItem long_items[] = {
    MenuItem::action("Short", demo_action),
    MenuItem::action("Status", demo_action),
    MenuItem::action("Telemetry", demo_action),
    MenuItem::action("Network Tools", demo_action),
    MenuItem::action("Display Setup", demo_action),
    MenuItem::action("Sensors", demo_action),
    MenuItem::action("Power Monitor", demo_action),
    MenuItem::action("Diagnostics", demo_action),
    MenuItem::action("Storage", demo_action),
    MenuItem::action("About Device", demo_action),
};
const Menu long_menu = make_menu("Long Menu", long_items);

const MenuItem root_items[] = {
    MenuItem::submenu("System", system_menu),
    MenuItem::submenu("Network", network_menu),
    MenuItem::submenu("Display", display_menu),
    MenuItem::submenu("Long Menu", long_menu),
    MenuItem::action("Reset Defaults", reset_defaults),
};
const Menu root_menu = make_menu("Jelly Menu", root_items);

} // namespace

const Menu& demo_menu_root() {
    return root_menu;
}

MenuStyle demo_menu_style() {
    MenuStyle style{};

    // Demo choice only: 11 px pitch keeps a 9 px frame compact while leaving
    // a clean 2 px visual gap between neighboring rows.
    style.row_height = 11;
    style.visible_rows = 4;

    // Symmetric padding between the frame and label/right-side indicator.
    style.content_inset_left = 8;
    style.content_inset_right = 8;

    // Motion parameters intentionally follow the small U8g2 ui_run pattern:
    // frame Y moves 5 px while far / 1 px near the target, frame width 10/1,
    // and long-menu scrolling 4/1. All advance on a 16 ms logical tick.
    style.glide_fit_content = true;
    style.glide_min_width = 28;
    style.glide_position_fast_step = 5;
    style.glide_position_slow_zone = 4;
    style.glide_width_fast_step = 10;
    style.glide_width_slow_zone = 5;
    style.glide_scroll_fast_step = 4;
    style.glide_scroll_slow_zone = 4;
    style.glide_tick_ms = 16;
    return style;
}

void bind_demo_menu(MenuPagePlugin<12>* menu) {
    bound_menu = menu;
    apply_cursor_style(nullptr);
}

void bind_demo_fps_debug(FpsDebugPlugin* fps) {
    bound_fps = fps;
    apply_fps_overlay(nullptr);
}

void reset_demo_menu_state() {
    wifi_enabled = true;
    dhcp_enabled = true;
    debug_enabled = false;
    fps_overlay = true;
    sleep_enabled = true;
    invert_enabled = false;
    soft_theme = true;
    glide_cursor = true;
    wifi_channel = 6;
    log_level = 2;
    brightness = 72;
    contrast = 205;
    radius = 4;
    battery_warn = 20;
    apply_cursor_style(nullptr);
    apply_fps_overlay(nullptr);
}

} // namespace epui::demo
