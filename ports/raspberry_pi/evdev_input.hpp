#pragma once

#include <string>
#include <utility>
#include "epui/input_plugin.hpp"

namespace epui::rpi {

class EvdevInputPlugin final : public epui::InputPlugin {
public:
    explicit EvdevInputPlugin(std::string device = "/dev/input/event0")
        : device_(std::move(device)) {}
    ~EvdevInputPlugin() override { stop(); }

    const char* name() const override { return "evdev-keyboard"; }
    bool start() override;
    void stop() override;
    bool poll(epui::InputEvent& event) override;
    const std::string& device() const { return device_; }

private:
    char key_to_char(unsigned int code) const;

    std::string device_;
    int fd_{-1};
    bool shift_{false};
    bool control_{false};
    bool caps_lock_{false};
};

} // namespace epui::rpi
