#pragma once

#include <cstddef>
#include <cstdint>

namespace epui {

enum class PluginKind : std::uint8_t {
    Display,
    Input,
    Sensor,
    Page,
    Widget,
    Animation,
    Theme,
    Platform,
    Service,
};

struct PluginDependencies {
    const char* const* names{nullptr};
    std::size_t count{0};
};

template <std::size_t N>
constexpr PluginDependencies plugin_dependencies(const char* const (&names)[N]) {
    return {names, N};
}

class Plugin {
public:
    virtual ~Plugin() = default;
    virtual const char* name() const = 0;
    virtual PluginKind kind() const = 0;
    virtual PluginDependencies dependencies() const { return {}; }
    virtual bool start() { return true; }
    virtual void tick(std::uint32_t) {}
    virtual void stop() {}
};

} // namespace epui
