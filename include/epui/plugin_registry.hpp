#pragma once

#include <cstddef>
#include <cstdint>
#include "epui/plugin.hpp"

namespace epui {

enum class RegistryError : std::uint8_t {
    Ok,
    Full,
    DuplicatePlugin,
    DuplicateName,
    MutatingWhileStarted,
    MissingDependency,
    DependencyCycle,
    StartFailed,
};

class PluginRegistry {
public:
    static constexpr std::size_t MaxPlugins = 16;

    bool add(Plugin& plugin);
    bool remove(Plugin& plugin);
    Plugin* find(const char* name) const;
    Plugin* at(std::size_t index) const;
    std::size_t size() const { return count_; }
    bool start_all();
    void tick_all(std::uint32_t now_ms);
    void stop_all();
    bool started() const { return started_count_ == count_ && count_ != 0; }

    RegistryError last_error() const { return last_error_; }
    const Plugin* error_plugin() const { return error_plugin_; }
    const char* error_dependency() const { return error_dependency_; }
    void clear_error();

private:
    enum class Visit : std::uint8_t { Unvisited, Visiting, Started };

    int find_index(const char* name) const;
    bool start_index(std::size_t index, Visit (&visit)[MaxPlugins]);
    void set_error(RegistryError error, const Plugin* plugin = nullptr, const char* dependency = nullptr);

    Plugin* plugins_[MaxPlugins]{};
    Plugin* started_order_[MaxPlugins]{};
    std::size_t count_{0};
    std::size_t started_count_{0};
    RegistryError last_error_{RegistryError::Ok};
    const Plugin* error_plugin_{nullptr};
    const char* error_dependency_{nullptr};
};

} // namespace epui
