#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>
#include "openoledui/canvas.hpp"
#include "openoledui/page.hpp"
#include "demo_pages.hpp"

using namespace openoledui;

namespace {
constexpr int kScale = 7;
constexpr int kPad = 24;
constexpr int kFooter = 28;
constexpr int kWidth = Canvas::Width * kScale + kPad * 2;
constexpr int kHeight = Canvas::Height * kScale + kPad * 2 + kFooter;

struct AppState {
    Canvas canvas;
    Ui ui;
    demo::HomePage home;
    demo::SensorPage sensors;
    demo::AboutPage about;
    AppState() {
        ui.add_page(home);
        ui.add_page(sensors);
        ui.add_page(about);
    }
};

std::uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<std::uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void handle_key(Ui& ui, KeySym key) {
    const auto now = now_ms();
    switch (key) {
        case XK_Right:
        case XK_d:
        case XK_D:
            ui.handle(Key::Next, now);
            break;
        case XK_Left:
        case XK_a:
        case XK_A:
            ui.handle(Key::Prev, now);
            break;
        case XK_Return:
        case XK_space:
            ui.handle(Key::Select, now);
            break;
        case XK_Escape:
            ui.handle(Key::Back, now);
            break;
        default:
            break;
    }
}

void draw_frame(Display* display, Window window, GC gc, AppState& app) {
    app.ui.render(app.canvas, now_ms());

    XSetForeground(display, gc, 0x131518);
    XFillRectangle(display, window, gc, 0, 0, kWidth, kHeight);

    XSetForeground(display, gc, 0x060708);
    XFillRectangle(display, window, gc,
                   kPad - 8, kPad - 8,
                   Canvas::Width * kScale + 16,
                   Canvas::Height * kScale + 16);

    XSetForeground(display, gc, 0xE5F2FF);
    const auto* fb = app.canvas.data();
    for (int y = 0; y < Canvas::Height; ++y) {
        for (int x = 0; x < Canvas::Width; ++x) {
            const std::size_t index = static_cast<std::size_t>(x + (y / 8) * Canvas::Width);
            if (((fb[index] >> (y & 7)) & 1u) != 0u) {
                XFillRectangle(display, window, gc,
                               kPad + x * kScale,
                               kPad + y * kScale,
                               kScale,
                               kScale);
            }
        }
    }

    const char* hint = "Left/Right or A/D: page   Enter: select   Esc: back";
    XSetForeground(display, gc, 0xA5ADB8);
    XDrawString(display, window, gc, kPad, kHeight - 10, hint, static_cast<int>(std::strlen(hint)));
    XFlush(display);
}
} // namespace

int main() {
    Display* display = XOpenDisplay(nullptr);
    if (!display) {
        return 1;
    }

    const int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        120, 120,
        kWidth, kHeight,
        0,
        BlackPixel(display, screen),
        BlackPixel(display, screen));

    XStoreName(display, window, "OpenMonoUI - 128x64 Simulator");
    XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask);
    XMapWindow(display, window);

    Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete, 1);

    GC gc = XCreateGC(display, window, 0, nullptr);
    AppState app;
    bool running = true;

    while (running) {
        while (XPending(display) > 0) {
            XEvent event{};
            XNextEvent(display, &event);
            if (event.type == KeyPress) {
                KeySym key = XLookupKeysym(&event.xkey, 0);
                handle_key(app.ui, key);
            } else if (event.type == ClientMessage && static_cast<Atom>(event.xclient.data.l[0]) == wm_delete) {
                running = false;
            }
        }

        draw_frame(display, window, gc, app);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
