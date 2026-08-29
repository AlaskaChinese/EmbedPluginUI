#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>
#include "epui/display_plugin.hpp"
#include "epui/input_plugin.hpp"
#include "epui/plugin_registry.hpp"
#include "epui/page.hpp"
#include "demo_pages.hpp"

using namespace epui;

namespace {
constexpr int kScale = 7;
constexpr int kPad = 24;
constexpr int kFooter = 28;
constexpr int kWidth = Canvas::Width * kScale + kPad * 2;
constexpr int kHeight = Canvas::Height * kScale + kPad * 2 + kFooter;

std::uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<std::uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

class X11DisplayPlugin final : public DisplayPlugin {
public:
    const char* name() const override { return "x11-display"; }
    int width() const override { return Canvas::Width; }
    int height() const override { return Canvas::Height; }

    bool start() override {
        display_ = XOpenDisplay(nullptr);
        if (!display_) return false;
        const int screen = DefaultScreen(display_);
        window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen), 120, 120,
                                      kWidth, kHeight, 0,
                                      BlackPixel(display_, screen), BlackPixel(display_, screen));
        XStoreName(display_, window_, "EmbedPluginUI - 128x64 Simulator");
        XSelectInput(display_, window_, ExposureMask | KeyPressMask | StructureNotifyMask);
        XMapWindow(display_, window_);
        wm_delete_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(display_, window_, &wm_delete_, 1);
        gc_ = XCreateGC(display_, window_, 0, nullptr);
        return gc_ != nullptr;
    }

    void stop() override {
        if (!display_) return;
        if (gc_) XFreeGC(display_, gc_);
        if (window_) XDestroyWindow(display_, window_);
        XCloseDisplay(display_);
        display_ = nullptr;
        window_ = 0;
        gc_ = nullptr;
    }

    bool present(const Canvas& canvas) override {
        if (!display_ || !window_ || !gc_) return false;
        XSetForeground(display_, gc_, 0x131518);
        XFillRectangle(display_, window_, gc_, 0, 0, kWidth, kHeight);
        XSetForeground(display_, gc_, 0x060708);
        XFillRectangle(display_, window_, gc_, kPad - 8, kPad - 8,
                       Canvas::Width * kScale + 16, Canvas::Height * kScale + 16);
        XSetForeground(display_, gc_, 0xE5F2FF);
        const auto* fb = canvas.data();
        for (int y = 0; y < Canvas::Height; ++y) {
            for (int x = 0; x < Canvas::Width; ++x) {
                const std::size_t index = static_cast<std::size_t>(x + (y / 8) * Canvas::Width);
                if (((fb[index] >> (y & 7)) & 1u) != 0u) {
                    XFillRectangle(display_, window_, gc_, kPad + x * kScale, kPad + y * kScale,
                                   kScale, kScale);
                }
            }
        }
        const char* hint = "Left/Right or A/D: page   Enter: select   Esc: back";
        XSetForeground(display_, gc_, 0xA5ADB8);
        XDrawString(display_, window_, gc_, kPad, kHeight - 10, hint, static_cast<int>(std::strlen(hint)));
        XFlush(display_);
        return true;
    }

    Display* native_display() const { return display_; }
    Atom wm_delete() const { return wm_delete_; }
    bool close_requested() const { return close_requested_; }
    void request_close() { close_requested_ = true; }

private:
    Display* display_{};
    Window window_{};
    GC gc_{};
    Atom wm_delete_{};
    bool close_requested_{false};
};

class X11InputPlugin final : public InputPlugin {
public:
    explicit X11InputPlugin(X11DisplayPlugin& display) : display_(display) {}
    const char* name() const override { return "x11-keyboard"; }
    PluginDependencies dependencies() const override { return {dependency_, 1}; }
    bool start() override { return display_.native_display() != nullptr; }
    bool poll(InputEvent& out) override {
        Display* native = display_.native_display();
        if (!native) return false;
        while (XPending(native) > 0) {
            XEvent event{};
            XNextEvent(native, &event);
            if (event.type == ClientMessage && static_cast<Atom>(event.xclient.data.l[0]) == display_.wm_delete()) {
                display_.request_close();
                continue;
            }
            if (event.type != KeyPress) continue;
            const KeySym key = XLookupKeysym(&event.xkey, 0);
            if (key == XK_Right || key == XK_d || key == XK_D) out.key = Key::Next;
            else if (key == XK_Left || key == XK_a || key == XK_A) out.key = Key::Prev;
            else if (key == XK_Return || key == XK_space) out.key = Key::Select;
            else if (key == XK_Escape) out.key = Key::Back;
            else continue;
            out.pressed = true;
            return true;
        }
        return false;
    }
private:
    X11DisplayPlugin& display_;
    const char* dependency_[1]{"x11-display"};
};
} // namespace

int main() {
    Canvas canvas;
    Ui ui;
    openoledui::demo::HomePage home;
    openoledui::demo::SensorPage sensors;
    openoledui::demo::AboutPage about;
    ui.add_page(home);
    ui.add_page(sensors);
    ui.add_page(about);

    X11DisplayPlugin display;
    X11InputPlugin input(display);
    PluginRegistry plugins;
    if (!plugins.add(input) || !plugins.add(display) || !plugins.start_all()) return 1;

    while (!display.close_requested()) {
        const auto now = now_ms();
        plugins.tick_all(now);
        InputEvent event{};
        while (input.poll(event)) {
            if (event.pressed) ui.handle(event.key, now);
        }
        ui.render(canvas, now);
        if (!display.present(canvas)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    plugins.stop_all();
    return 0;
}
