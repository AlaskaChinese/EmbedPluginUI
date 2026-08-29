#pragma once

#include <cstdint>
#include "epui/plugin.hpp"

namespace epui {

struct Theme {
    std::uint8_t padding{3};
    std::uint8_t gap{3};
    std::uint8_t card_radius{3};
    std::uint8_t line_height{8};
    std::uint8_t progress_height{7};
    std::uint16_t transition_ms{220};
    bool inverted{false};

    static constexpr Theme compact() { return {2, 2, 2, 7, 6, 180, false}; }
    static constexpr Theme soft() { return {4, 4, 4, 9, 8, 240, false}; }
};

class ThemePlugin final : public Plugin {
public:
    explicit ThemePlugin(Theme theme = Theme{}, const char* plugin_name = "theme")
        : name_(plugin_name), theme_(theme) {}

    const char* name() const override { return name_; }
    PluginKind kind() const override { return PluginKind::Theme; }

    const Theme& theme() const { return theme_; }
    void set_theme(const Theme& theme) { theme_ = theme; }

private:
    const char* name_;
    Theme theme_;
};

} // namespace epui
