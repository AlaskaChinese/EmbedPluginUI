#pragma once

#include "epui/plugin.hpp"

namespace epui {

class PlatformPlugin : public Plugin {
public:
    PluginKind kind() const final { return PluginKind::Platform; }
};

} // namespace epui
