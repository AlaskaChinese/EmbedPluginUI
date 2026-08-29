#pragma once

#include <algorithm>
#include "epui/theme_plugin.hpp"
#include "epui/widget_plugin.hpp"
#include "epui/widgets.hpp"

namespace epui {

class TextWidget final : public WidgetPlugin {
public:
    TextWidget(const char* plugin_name, const char* text) : name_(plugin_name), text_(text) {}
    const char* name() const override { return name_; }
    void set_text(const char* text) { text_ = text; }
    void draw(Canvas& canvas, int x, int y, std::uint32_t) override {
        canvas.text(x, y, text_ ? text_ : "");
    }
private:
    const char* name_;
    const char* text_;
};

class ProgressWidget final : public WidgetPlugin {
public:
    ProgressWidget(const char* plugin_name, int width, float value = 0.0f)
        : name_(plugin_name), width_(width), value_(value) {}
    const char* name() const override { return name_; }
    void set_value(float value) { value_ = std::max(0.0f, std::min(1.0f, value)); }
    float value() const { return value_; }
    void draw(Canvas& canvas, int x, int y, std::uint32_t) override {
        canvas.progress_bar(x, y, width_, 7, value_);
    }
private:
    const char* name_;
    int width_;
    float value_;
};

class ThemedProgressWidget final : public WidgetPlugin {
public:
    ThemedProgressWidget(const char* plugin_name, const ThemePlugin& theme, int width, float value = 0.0f)
        : name_(plugin_name), theme_(theme), width_(width), value_(value), dependency_{theme.name()} {}
    const char* name() const override { return name_; }
    PluginDependencies dependencies() const override { return {dependency_, 1}; }
    void set_value(float value) { value_ = std::max(0.0f, std::min(1.0f, value)); }
    void draw(Canvas& canvas, int x, int y, std::uint32_t) override {
        canvas.progress_bar(x, y, width_, theme_.theme().progress_height, value_);
    }
private:
    const char* name_;
    const ThemePlugin& theme_;
    int width_;
    float value_;
    const char* dependency_[1]{};
};

} // namespace epui
