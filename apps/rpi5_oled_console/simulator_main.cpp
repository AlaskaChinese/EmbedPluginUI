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

std::uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<std::uint32_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
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

    while (!display.close_requested()) {
        const std::uint32_t now = now_ms();
        plugins.tick_all(now);

        InputEvent event{};
        while (input.poll(event)) ui.handle(event, now);

        ui.render(canvas, now);
        if (!display.present(canvas)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    plugins.stop_all();
    return 0;
}
