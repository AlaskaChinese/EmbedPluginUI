#pragma once

#include "epui/canvas.hpp"
#include "epui/plugin.hpp"

namespace epui {

class WidgetPlugin : public Plugin {
public:
    PluginKind kind() const final { return PluginKind::Widget; }
    virtual void draw(Canvas& canvas, int x, int y, std::uint32_t now_ms) = 0;
};

} // namespace epui
