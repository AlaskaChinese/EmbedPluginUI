#pragma once

#include <cstdint>

namespace epui {

enum class PluginKind : std::uint8_t {
    Display,
    Input,
    Page,
    Widget,
    Platform,
    Service,
};

class Plugin {
public:
    virtual ~Plugin() = default;
    virtual const char* name() const = 0;
    virtual PluginKind kind() const = 0;
    virtual bool start() { return true; }
    virtual void stop() {}
};

} // namespace epui
