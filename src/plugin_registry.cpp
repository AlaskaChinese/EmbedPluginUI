#include "epui/plugin_registry.hpp"
#include <cstring>

namespace epui {

bool PluginRegistry::add(Plugin& plugin) {
    if (count_ >= MaxPlugins || started_count_ != 0) return false;
    for (std::size_t i = 0; i < count_; ++i) {
        if (plugins_[i] == &plugin) return false;
        const char* a = plugins_[i]->name();
        const char* b = plugin.name();
        if (a && b && std::strcmp(a, b) == 0) return false;
    }
    plugins_[count_++] = &plugin;
    return true;
}

bool PluginRegistry::remove(Plugin& plugin) {
    if (started_count_ != 0) return false;
    for (std::size_t i = 0; i < count_; ++i) {
        if (plugins_[i] != &plugin) continue;
        for (std::size_t j = i + 1; j < count_; ++j) plugins_[j - 1] = plugins_[j];
        plugins_[--count_] = nullptr;
        return true;
    }
    return false;
}

Plugin* PluginRegistry::find(const char* name) const {
    if (!name) return nullptr;
    for (std::size_t i = 0; i < count_; ++i) {
        const char* candidate = plugins_[i]->name();
        if (candidate && std::strcmp(candidate, name) == 0) return plugins_[i];
    }
    return nullptr;
}

Plugin* PluginRegistry::at(std::size_t index) const {
    return index < count_ ? plugins_[index] : nullptr;
}

bool PluginRegistry::start_all() {
    if (started_count_ != 0) return started_count_ == count_;
    for (std::size_t i = 0; i < count_; ++i) {
        if (!plugins_[i]->start()) {
            while (started_count_ > 0) plugins_[--started_count_]->stop();
            return false;
        }
        ++started_count_;
    }
    return true;
}

void PluginRegistry::stop_all() {
    while (started_count_ > 0) plugins_[--started_count_]->stop();
}

} // namespace epui
