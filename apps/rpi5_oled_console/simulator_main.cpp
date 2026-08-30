#include "pages.hpp"
#include "pty_session.hpp"
#include "rpi_plugins.hpp"
#include "x11_simulator.hpp"
#include "epui/canvas.hpp"
#include "epui/input_plugin.hpp"
#include "epui/plugin_registry.hpp"
#include <chrono>
#include <cstdlib>
#include <thread>

using namespace epui;
using namespace epui::rpi;
using namespace epui::rpi::console;

namespace {

using Clock = std::chrono::steady_clock;

std::uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<std::uint32_t>(
        duration_cast<milliseconds>(Clock::now().time_since_epoch()).count());
}

StatusSection active_section(const Ui& ui) {
    if (ui.animating()) return StatusSection::Inactive;
    switch (ui.page_index()) {
    case 0: return StatusSection::Overview;
    case 1: return StatusSection::Network;
    case 2: return StatusSection::Power;
    case 3: return StatusSection::System;
    default: return StatusSection::Inactive;
    }
}

} // namespace

int main() {
    const char* configured_shell = std::getenv("SHELL");
    const char* shell_path = configured_shell && *configured_shell ? configured_shell : "/bin/sh";

    Canvas canvas;
    Ui ui;
    epui::x11::X11DisplayPlugin display("EmbedPluginUI - Raspberry Pi 5 App Simulator");
    epui::x11::X11InputPlugin input(display);
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
        || !plugins.start_all()) return 1;

    auto next_frame = Clock::now();
    while (!display.close_requested()) {
        const std::uint32_t now = now_ms();

        InputEvent event{};
        while (input.poll(event)) ui.handle(event, now);
        system.set_section(active_section(ui));
        plugins.tick_all(now);

        ui.render(canvas, now);
        if (!display.present(canvas)) break;
        next_frame += std::chrono::milliseconds(16);
        if (next_frame < Clock::now()) next_frame = Clock::now();
        std::this_thread::sleep_until(next_frame);
    }

    plugins.stop_all();
    return 0;
}
