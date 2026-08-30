#include "pages.hpp"
#include "evdev_input.hpp"
#include "linux_i2c.hpp"
#include "pty_session.hpp"
#include "rpi_plugins.hpp"
#include "epui/canvas.hpp"
#include "epui/input_plugin.hpp"
#include "epui/oled.hpp"
#include "epui/oled_display_plugin.hpp"
#include "epui/plugin_registry.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

using namespace epui;
using namespace epui::rpi;
using namespace epui::rpi::console;

namespace {

volatile std::sig_atomic_t stop_requested = 0;

void request_stop(int) { stop_requested = 1; }

std::uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<std::uint32_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

} // namespace

int main(int argc, char** argv) {
    const char* i2c_device = argc > 1 ? argv[1] : "/dev/i2c-1";
    const int address = argc > 2 ? std::strtol(argv[2], nullptr, 0) : 0x3c;
    const bool sh1106 = argc > 3 && std::string(argv[3]) == "sh1106";
    const char* input_device = argc > 4 ? argv[4] : "/dev/input/event0";
    const char* configured_shell = argc > 5 ? argv[5] : std::getenv("SHELL");
    const char* shell_path = configured_shell && *configured_shell ? configured_shell : "/bin/sh";

    std::signal(SIGINT, request_stop);
    std::signal(SIGTERM, request_stop);

    LinuxI2cTransport bus(i2c_device, static_cast<std::uint8_t>(address));
    if (!bus.open_bus()) {
        std::perror("Open I2C");
        return 2;
    }

    Canvas canvas;
    Ui ui;
    Oled128x64 oled(bus, sh1106 ? OledController::SH1106 : OledController::SSD1306);
    OledDisplayPlugin display(oled, sh1106 ? "sh1106-display" : "ssd1306-display");
    EvdevInputPlugin input(input_device);
    SystemMonitorPlugin system;
    PtySessionPlugin shell(shell_path);
    OverviewPage overview(ui, system);
    NetworkPage network(ui, system);
    PowerPage power(ui, system);
    SystemPage system_page(ui, system);
    TerminalPage terminal(ui, shell);

    PluginRegistry plugins;
    if (!plugins.add(display) || !plugins.add(input) || !plugins.add(overview)
        || !plugins.add(network) || !plugins.add(power) || !plugins.add(system_page)
        || !plugins.add(terminal) || !plugins.add(system) || !plugins.add(shell)
        || !plugins.start_all()) {
        std::fprintf(stderr, "Plugin startup failed (input=%s)\n", input_device);
        return 3;
    }

    std::fprintf(stderr,
                 "EmbedPluginUI Pi 5 console running: input=%s shell=%s; "
                 "Left/Right=navigate/edit, Ctrl+Up/Down=page output, "
                 "Enter=focus/run, Esc=unfocus\n",
                 input_device, shell_path);

    std::array<std::uint8_t, Canvas::BufferSize> previous_frame{};
    bool frame_presented = false;
    while (!stop_requested) {
        const std::uint32_t now = now_ms();
        plugins.tick_all(now);

        InputEvent event{};
        while (input.poll(event)) ui.handle(event, now);

        ui.render(canvas, now);
        const auto* frame = canvas.data();
        const bool changed = !frame_presented
            || !std::equal(frame, frame + Canvas::BufferSize, previous_frame.begin());
        if (changed) {
            if (!display.present(canvas)) {
                std::fprintf(stderr, "OLED write failed\n");
                break;
            }
            std::copy(frame, frame + Canvas::BufferSize, previous_frame.begin());
            frame_presented = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    plugins.stop_all();
    return 0;
}
