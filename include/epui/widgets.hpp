#pragma once

#include <cstdint>
#include "epui/canvas.hpp"

namespace epui {
void draw_header(Canvas& c, const char* title, int page, int pages);
void draw_metric(Canvas& c, int x, int y, const char* label, const char* value);
void draw_card(Canvas& c, int x, int y, int w, int h, const char* title);
void draw_wifi_icon(Canvas& c, int x, int y, int strength = 3);
void draw_thermometer(Canvas& c, int x, int y, float normalized);
void draw_spinner(Canvas& c, int cx, int cy, std::uint32_t now_ms);
} // namespace epui
