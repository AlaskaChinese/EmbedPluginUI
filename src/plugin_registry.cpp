#include "epui/plugin_registry.hpp"
#include <cstring>

namespace epui {

void PluginRegistry::clear_error() {
    last_error_ = RegistryError::Ok;
    error_plugin_ = nullptr;
    error_dependency_ = nullptr;
}

void PluginRegistry::set_error(RegistryError error, const Plugin* plugin, const char* dependency) {
    last_error_ = error;
    error_plugin_ = plugin;
    error_dependency_ = dependency;
}

int PluginRegistry::find_index(const char* name) const {
    if (!name) return -1;
    for (std::size_t i = 0; i < count_; ++i) {
        const char* candidate = plugins_[i]->name();
        if (candidate && std::strcmp(candidate, name) == 0) return static_cast<int>(i);
    }
    return -1;
}

bool PluginRegistry::add(Plugin& plugin) {
    clear_error();
    if (started_count_ != 0) {
        set_error(RegistryError::MutatingWhileStarted, &plugin);
        return false;
    }
    if (count_ >= MaxPlugins) {
        set_error(RegistryError::Full, &plugin);
        return false;
    }
    for (std::size_t i = 0; i < count_; ++i) {
        if (plugins_[i] == &plugin) {
            set_error(RegistryError::DuplicatePlugin, &plugin);
            return false;
        }
        const char* a = plugins_[i]->name();
        const char* b = plugin.name();
        if (a && b && std::strcmp(a, b) == 0) {
            set_error(RegistryError::DuplicateName, &plugin);
            return false;
        }
    }
    plugins_[count_++] = &plugin;
    return true;
}

bool PluginRegistry::remove(Plugin& plugin) {
    clear_error();
    if (started_count_ != 0) {
        set_error(RegistryError::MutatingWhileStarted, &plugin);
        return false;
    }
    for (std::size_t i = 0; i < count_; ++i) {
        if (plugins_[i] != &plugin) continue;
        for (std::size_t j = i + 1; j < count_; ++j) plugins_[j - 1] = plugins_[j];
        plugins_[--count_] = nullptr;
        return true;
    }
    return false;
}

Plugin* PluginRegistry::find(const char* name) const {
    const int index = find_index(name);
    return index >= 0 ? plugins_[static_cast<std::size_t>(index)] : nullptr;
}

Plugin* PluginRegistry::at(std::size_t index) const {
    return index < count_ ? plugins_[index] : nullptr;
}

bool PluginRegistry::start_index(std::size_t index, Visit (&visit)[MaxPlugins]) {
    if (visit[index] == Visit::Started) return true;
    if (visit[index] == Visit::Visiting) {
        set_error(RegistryError::DependencyCycle, plugins_[index]);
        return false;
    }

    visit[index] = Visit::Visiting;
    Plugin& plugin = *plugins_[index];
    const PluginDependencies deps = plugin.dependencies();
    for (std::size_t i = 0; i < deps.count; ++i) {
        const char* dependency = deps.names ? deps.names[i] : nullptr;
        const int dependency_index = find_index(dependency);
        if (dependency_index < 0) {
            set_error(RegistryError::MissingDependency, &plugin, dependency);
            return false;
        }
        if (!start_index(static_cast<std::size_t>(dependency_index), visit)) return false;
    }

    if (!plugin.start()) {
        set_error(RegistryError::StartFailed, &plugin);
        return false;
    }

    started_order_[started_count_++] = &plugin;
    visit[index] = Visit::Started;
    return true;
}

bool PluginRegistry::start_all() {
    clear_error();
    if (started_count_ != 0) return started_count_ == count_;

    Visit visit[MaxPlugins]{};
    for (std::size_t i = 0; i < count_; ++i) {
        if (!start_index(i, visit)) {
            while (started_count_ > 0) {
                Plugin* plugin = started_order_[--started_count_];
                started_order_[started_count_] = nullptr;
                plugin->stop();
            }
            return false;
        }
    }
    return true;
}

void PluginRegistry::tick_all(std::uint32_t now_ms) {
    for (std::size_t i = 0; i < started_count_; ++i) started_order_[i]->tick(now_ms);
}

void PluginRegistry::stop_all() {
    while (started_count_ > 0) {
        Plugin* plugin = started_order_[--started_count_];
        started_order_[started_count_] = nullptr;
        plugin->stop();
    }
}

} // namespace epui
