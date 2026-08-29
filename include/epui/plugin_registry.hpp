#pragma once

#include <cstddef>
#include "epui/plugin.hpp"

namespace epui {

class PluginRegistry {
public:
    static constexpr std::size_t MaxPlugins = 16;

    bool add(Plugin& plugin);
    bool remove(Plugin& plugin);
    Plugin* find(const char* name) const;
    Plugin* at(std::size_t index) const;
    std::size_t size() const { return count_; }
    bool start_all();
    void stop_all();
    bool started() const { return started_count_ == count_ && count_ != 0; }

private:
    Plugin* plugins_[MaxPlugins]{};
    std::size_t count_{0};
    std::size_t started_count_{0};
};

} // namespace epui
