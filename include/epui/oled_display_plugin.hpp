#pragma once

#include "epui/display_plugin.hpp"
#include "epui/oled.hpp"

namespace epui {

class OledDisplayPlugin final : public DisplayPlugin {
public:
    explicit OledDisplayPlugin(Oled128x64& oled, const char* plugin_name = "oled-128x64",
                               const char* platform_dependency = nullptr)
        : oled_(oled), name_(plugin_name), platform_dependency_(platform_dependency) {}

    const char* name() const override { return name_; }
    PluginDependencies dependencies() const override {
        return platform_dependency_ ? PluginDependencies{&platform_dependency_, 1} : PluginDependencies{};
    }
    bool start() override { return oled_.init(); }
    void stop() override { oled_.power(false); }
    int width() const override { return Canvas::Width; }
    int height() const override { return Canvas::Height; }
    bool present(const Canvas& canvas) override {
        return oled_.present(canvas.data(), Canvas::BufferSize);
    }

private:
    Oled128x64& oled_;
    const char* name_;
    const char* platform_dependency_;
};

} // namespace epui
