#include "x11_simulator.hpp"
#include "app.hpp"
#include "epui/plugin_registry.hpp"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <thread>
#include <unistd.h>

using namespace epui;

namespace {

using Clock = std::chrono::steady_clock;

std::uint32_t now_ms() {
    using namespace std::chrono;
    return static_cast<std::uint32_t>(
        duration_cast<milliseconds>(Clock::now().time_since_epoch()).count());
}

std::uint32_t elapsed_us(Clock::time_point begin, Clock::time_point end) {
    using namespace std::chrono;
    const auto value = duration_cast<microseconds>(end - begin).count();
    if (value <= 0) return 0;
    const auto limit = static_cast<long long>(std::numeric_limits<std::uint32_t>::max());
    return static_cast<std::uint32_t>(value > limit ? limit : value);
}

bool process_memory_probe(void*, DebugMemoryStats& out) {
    std::FILE* file = std::fopen("/proc/self/statm", "r");
    if (!file) return false;
    unsigned long total_pages = 0;
    unsigned long resident_pages = 0;
    const int matched = std::fscanf(file, "%lu %lu", &total_pages, &resident_pages);
    std::fclose(file);
    if (matched != 2) return false;
    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0) return false;
    out.used_bytes = static_cast<std::uint64_t>(resident_pages)
        * static_cast<std::uint64_t>(page_size);
    out.total_bytes = 0;
    return true;
}

} // namespace

int main() {
    epui::demo::SimulatorUi app;
    auto& canvas = app.canvas();
    auto& ui = app.ui();
    auto& diagnostics = app.diagnostics();
    diagnostics.set_memory_probe(process_memory_probe);

    epui::x11::X11DisplayPlugin display;
    epui::x11::X11InputPlugin input(display);
    PluginRegistry plugins;
    if (!plugins.add(input) || !plugins.add(display) || !plugins.start_all()) return 1;

    auto next_frame = Clock::now();
    while (!display.close_requested()) {
        const auto now = now_ms();
        plugins.tick_all(now);
        InputEvent event{};
        while (input.poll(event)) ui.handle(event, now);

        const auto render_begin = Clock::now();
        ui.render(canvas, now);
        diagnostics.record_render_time_us(elapsed_us(render_begin, Clock::now()));

        const auto transfer_begin = Clock::now();
        const bool presented = display.present(canvas);
        diagnostics.record_transfer(elapsed_us(transfer_begin, Clock::now()),
                                    Canvas::BufferSize, presented);
        if (!presented) break;
        next_frame += std::chrono::milliseconds(16);
        if (next_frame < Clock::now()) next_frame = Clock::now();
        std::this_thread::sleep_until(next_frame);
    }

    plugins.stop_all();
    return 0;
}
