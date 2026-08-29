#pragma once

#include "epui/canvas.hpp"
#include "epui/plugin.hpp"

namespace epui {

class DisplayPlugin : public Plugin {
public:
    PluginKind kind() const final { return PluginKind::Display; }
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual bool present(const Canvas& canvas) = 0;
};

} // namespace epui
