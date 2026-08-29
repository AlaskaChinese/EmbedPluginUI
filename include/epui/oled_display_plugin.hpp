#pragma once

#include "epui/display_plugin.hpp"
#include "epui/oled.hpp"

namespace epui {

class OledDisplayPlugin final : public DisplayPlugin {
public:
    explicit OledDisplayPlugin(Oled128x64& oled, const char* plugin_name = "oled-128x64")
        : oled_(oled), name_(plugin_name) {}

    const char* name() const override { return name_; }
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
};

} // namespace epui
