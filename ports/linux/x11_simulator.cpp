#include "x11_simulator.hpp"
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <cstring>

namespace epui::x11 {
namespace {

constexpr int Scale = 7;
constexpr int Padding = 24;
constexpr int Footer = 28;
constexpr int WindowWidth = epui::Canvas::Width * Scale + Padding * 2;
constexpr int WindowHeight = epui::Canvas::Height * Scale + Padding * 2 + Footer;

} // namespace

bool X11DisplayPlugin::start() {
    if (display_) return true;
    display_ = XOpenDisplay(nullptr);
    if (!display_) return false;
    const int screen = DefaultScreen(display_);
    window_ = XCreateSimpleWindow(display_, RootWindow(display_, screen), 120, 120,
                                  WindowWidth, WindowHeight, 0,
                                  BlackPixel(display_, screen), BlackPixel(display_, screen));
    XStoreName(display_, window_, title_ ? title_ : "EmbedPluginUI Simulator");
    XSelectInput(display_, window_, ExposureMask | KeyPressMask | StructureNotifyMask);
    XMapWindow(display_, window_);
    wm_delete_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display_, window_, &wm_delete_, 1);
    gc_ = XCreateGC(display_, window_, 0, nullptr);
    close_requested_ = false;
    return gc_ != nullptr;
}

void X11DisplayPlugin::stop() {
    if (!display_) return;
    if (gc_) XFreeGC(display_, gc_);
    if (window_) XDestroyWindow(display_, window_);
    XCloseDisplay(display_);
    display_ = nullptr;
    window_ = 0;
    gc_ = nullptr;
}

bool X11DisplayPlugin::present(const epui::Canvas& canvas) {
    if (!display_ || !window_ || !gc_) return false;
    XSetForeground(display_, gc_, 0x131518);
    XFillRectangle(display_, window_, gc_, 0, 0, WindowWidth, WindowHeight);
    XSetForeground(display_, gc_, 0x060708);
    XFillRectangle(display_, window_, gc_, Padding - 8, Padding - 8,
                   epui::Canvas::Width * Scale + 16, epui::Canvas::Height * Scale + 16);
    XSetForeground(display_, gc_, 0xE5F2FF);
    const auto* framebuffer = canvas.data();
    for (int y = 0; y < epui::Canvas::Height; ++y) {
        int x = 0;
        while (x < epui::Canvas::Width) {
            const std::size_t index = static_cast<std::size_t>(
                x + (y / 8) * epui::Canvas::Width);
            if (((framebuffer[index] >> (y & 7)) & 1u) == 0u) {
                ++x;
                continue;
            }
            const int begin = x;
            do {
                ++x;
                if (x >= epui::Canvas::Width) break;
                const std::size_t next = static_cast<std::size_t>(
                    x + (y / 8) * epui::Canvas::Width);
                if (((framebuffer[next] >> (y & 7)) & 1u) == 0u) break;
            } while (true);
            XFillRectangle(display_, window_, gc_, Padding + begin * Scale,
                           Padding + y * Scale, (x - begin) * Scale, Scale);
        }
    }
    const char* hint = "Left/Right: pages/back/open   Up/Down: menu   Ctrl+Up/Down: terminal output";
    XSetForeground(display_, gc_, 0xA5ADB8);
    XDrawString(display_, window_, gc_, Padding, WindowHeight - 10,
                hint, static_cast<int>(std::strlen(hint)));
    XFlush(display_);
    return true;
}

bool X11InputPlugin::poll(epui::InputEvent& out) {
    Display* display = display_.native_display();
    if (!display) return false;
    while (XPending(display) > 0) {
        XEvent event{};
        XNextEvent(display, &event);
        if (event.type == ClientMessage
            && static_cast<Atom>(event.xclient.data.l[0]) == display_.wm_delete()) {
            display_.request_close();
            continue;
        }
        if (event.type != KeyPress) continue;

        KeySym key = NoSymbol;
        char text[8]{};
        const int count = XLookupString(&event.xkey, text, sizeof(text), &key, nullptr);
        out = epui::InputEvent{};
        const bool control = (event.xkey.state & ControlMask) != 0;
        if (control && key == XK_Up) out.key = epui::Key::ScrollUp;
        else if (control && key == XK_Down) out.key = epui::Key::ScrollDown;
        else if (key == XK_Right) out.key = epui::Key::Right;
        else if (key == XK_Left) out.key = epui::Key::Left;
        else if (key == XK_Up) out.key = epui::Key::Up;
        else if (key == XK_Down) out.key = epui::Key::Down;
        else if (key == XK_Return || key == XK_KP_Enter) out.key = epui::Key::Select;
        else if (key == XK_Escape) out.key = epui::Key::Back;
        else if (key == XK_BackSpace) out.ch = '\b';
        else if (key == XK_Tab) out.ch = '\t';
        else if (count == 1 && text[0] != 0
                 && static_cast<unsigned char>(text[0]) <= 0x7e) out.ch = text[0];
        else continue;
        return true;
    }
    return false;
}

} // namespace epui::x11
