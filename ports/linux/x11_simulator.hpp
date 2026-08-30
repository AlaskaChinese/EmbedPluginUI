#pragma once

#include <X11/Xlib.h>
#include "epui/display_plugin.hpp"
#include "epui/input_plugin.hpp"

namespace epui::x11 {

class X11DisplayPlugin final : public epui::DisplayPlugin {
public:
    explicit X11DisplayPlugin(const char* title = "EmbedPluginUI - 128x64 Simulator")
        : title_(title) {}
    ~X11DisplayPlugin() override { stop(); }

    const char* name() const override { return "x11-display"; }
    int width() const override { return epui::Canvas::Width; }
    int height() const override { return epui::Canvas::Height; }
    bool start() override;
    void stop() override;
    bool present(const epui::Canvas& canvas) override;

    Display* native_display() const { return display_; }
    Atom wm_delete() const { return wm_delete_; }
    bool close_requested() const { return close_requested_; }
    void request_close() { close_requested_ = true; }

private:
    const char* title_;
    Display* display_{};
    Window window_{};
    GC gc_{};
    Atom wm_delete_{};
    bool close_requested_{false};
};

class X11InputPlugin final : public epui::InputPlugin {
public:
    explicit X11InputPlugin(X11DisplayPlugin& display) : display_(display) {}
    const char* name() const override { return "x11-keyboard"; }
    epui::PluginDependencies dependencies() const override { return {dependency_, 1}; }
    bool start() override { return display_.native_display() != nullptr; }
    bool poll(epui::InputEvent& event) override;

private:
    X11DisplayPlugin& display_;
    const char* dependency_[1]{"x11-display"};
};

} // namespace epui::x11
