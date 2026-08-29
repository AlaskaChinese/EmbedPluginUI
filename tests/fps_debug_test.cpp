#include "epui/fps_debug_plugin.hpp"
#include <cassert>
#include <cstddef>

namespace {

bool any_pixel(const epui::Canvas& canvas) {
    for (std::size_t i = 0; i < epui::Canvas::BufferSize; ++i) {
        if (canvas.data()[i] != 0) return true;
    }
    return false;
}

} // namespace

int main() {
    epui::Ui ui;
    epui::FpsDebugStyle style;
    style.sample_window_ms = 500;
    epui::FpsDebugPlugin fps(ui, "fps-test", style);

    assert(fps.kind() == epui::PluginKind::Debug);
    assert(fps.start());
    assert(fps.attached());
    assert(ui.overlay_count() == 1);

    epui::Canvas canvas;
    for (std::uint32_t t = 0; t <= 500; t += 100) ui.render(canvas, t);
    assert(fps.fps_valid());
    assert(fps.fps_x10() == 100);
    assert(fps.fps() == 10.0f);
    assert(any_pixel(canvas));

    fps.set_visible(false);
    ui.render(canvas, 600);
    assert(!any_pixel(canvas));

    fps.set_auto_sample(false);
    fps.reset();
    fps.mark_frame(0);
    for (std::uint32_t t = 50; t <= 500; t += 50) fps.mark_frame(t);
    assert(fps.fps_valid());
    assert(fps.fps_x10() == 200);

    fps.set_visible(true);
    ui.render(canvas, 550);
    assert(any_pixel(canvas));

    fps.stop();
    assert(!fps.attached());
    assert(ui.overlay_count() == 0);
    return 0;
}
