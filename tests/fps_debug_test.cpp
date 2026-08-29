#include "epui/diagnostics_plugin.hpp"
#include "epui/fps_debug_plugin.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {

bool any_pixel(const epui::Canvas& canvas) {
    for (std::size_t i = 0; i < epui::Canvas::BufferSize; ++i) {
        if (canvas.data()[i] != 0) return true;
    }
    return false;
}

bool memory_probe(void* user, epui::DebugMemoryStats& out) {
    const auto used = *static_cast<const std::uint32_t*>(user);
    out.used_bytes = used;
    out.total_bytes = 64u * 1024u;
    return true;
}

} // namespace

int main() {
    epui::Ui ui;
    epui::DiagnosticsStyle style;
    style.sample_window_ms = 500;
    epui::DiagnosticsPlugin diagnostics(ui, "diagnostics-test", style);

    // The original public names remain aliases for source compatibility.
    epui::FpsDebugPlugin* compatibility = &diagnostics;
    assert(compatibility->kind() == epui::PluginKind::Debug);

    assert(diagnostics.start());
    assert(diagnostics.attached());
    assert(ui.overlay_count() == 1);

    epui::Canvas canvas;
    for (std::uint32_t t = 0; t <= 500; t += 100) ui.render(canvas, t);
    assert(diagnostics.fps_valid());
    assert(diagnostics.fps_x10() == 100);
    assert(diagnostics.fps() == 10.0f);
    assert(diagnostics.frame_time_valid());
    assert(diagnostics.frame_ms_x10() == 1000);
    assert(diagnostics.frame_time_ms() == 100.0f);
    assert(any_pixel(canvas));

    diagnostics.record_render_time_us(450);
    assert(diagnostics.render_time_valid());
    assert(diagnostics.render_time_us() == 450);

    diagnostics.record_transfer(2000, epui::Canvas::BufferSize, true);
    assert(diagnostics.transfer_valid());
    assert(diagnostics.transfer_time_us() == 2000);
    assert(diagnostics.transfer_bytes() == epui::Canvas::BufferSize);
    assert(diagnostics.transfer_rate_bps() == 512000u);
    assert(diagnostics.transfer_count() == 1);
    assert(diagnostics.transfer_failures() == 0);
    assert(diagnostics.last_transfer_ok());

    diagnostics.record_transfer(1000, 128, false);
    assert(diagnostics.transfer_count() == 2);
    assert(diagnostics.transfer_failures() == 1);
    assert(!diagnostics.last_transfer_ok());

    diagnostics.set_memory_bytes(32u * 1024u, 64u * 1024u);
    assert(diagnostics.memory_valid());
    assert(diagnostics.memory_used_bytes() == 32u * 1024u);
    assert(diagnostics.memory_total_bytes() == 64u * 1024u);

    std::uint32_t probed_used = 24u * 1024u;
    diagnostics.set_memory_probe(memory_probe, &probed_used);
    assert(diagnostics.memory_used_bytes() == 24u * 1024u);
    assert(diagnostics.memory_total_bytes() == 64u * 1024u);

    diagnostics.set_view(epui::DebugMetricView::Timing);
    assert(diagnostics.view() == epui::DebugMetricView::Timing);
    ui.render(canvas, 600);
    assert(any_pixel(canvas));

    diagnostics.set_view(epui::DebugMetricView::Memory);
    ui.render(canvas, 700);
    assert(any_pixel(canvas));

    diagnostics.set_view(epui::DebugMetricView::Transfer);
    ui.render(canvas, 800);
    assert(any_pixel(canvas));

    diagnostics.set_visible(false);
    ui.render(canvas, 900);
    assert(!any_pixel(canvas));

    diagnostics.set_auto_sample(false);
    diagnostics.reset();
    diagnostics.mark_frame(0);
    for (std::uint32_t t = 50; t <= 500; t += 50) diagnostics.mark_frame(t);
    assert(diagnostics.fps_valid());
    assert(diagnostics.fps_x10() == 200);
    assert(diagnostics.frame_ms_x10() == 500);

    diagnostics.set_visible(true);
    diagnostics.set_view(epui::DebugMetricView::Fps);
    ui.render(canvas, 550);
    assert(any_pixel(canvas));

    diagnostics.stop();
    assert(!diagnostics.attached());
    assert(ui.overlay_count() == 0);
    return 0;
}
