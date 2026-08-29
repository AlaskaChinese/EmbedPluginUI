#include "epui/page.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>

namespace {

class MarkerPage final : public epui::Page {
public:
    explicit MarkerPage(int x) : x_(x) {}
    void draw(epui::Canvas& canvas, std::uint32_t) override { canvas.pixel(x_, 8); }
private:
    int x_;
};

void run_until_settled(epui::Ui& ui, epui::Canvas& canvas, std::uint32_t start_ms,
                       float& max_position) {
    std::uint32_t now = start_ms;
    for (int frame = 0; frame < 120 && ui.animating(); ++frame) {
        ui.render(canvas, now);
        if (ui.transition_position() > max_position) max_position = ui.transition_position();
        now += 16;
    }
}

} // namespace

int main() {
    epui::Ui ui;
    epui::Canvas canvas;
    MarkerPage first(4);
    MarkerPage second(12);

    assert(ui.add_page(first));
    assert(ui.add_page(second));
    assert(ui.page_index() == 0);

    ui.handle(epui::Key::Next, 1000);
    assert(ui.animating());
    assert(std::fabs(ui.transition_position()) < 0.001f);

    float max_position = 0.0f;
    run_until_settled(ui, canvas, 1000, max_position);
    assert(max_position > static_cast<float>(epui::Canvas::Width) + 2.0f);
    assert(!ui.animating());
    assert(ui.page_index() == 1);
    assert(std::fabs(ui.transition_position() - static_cast<float>(epui::Canvas::Width)) < 0.001f);

    ui.handle(epui::Key::Prev, 2000);
    assert(ui.animating());
    max_position = 0.0f;
    run_until_settled(ui, canvas, 2000, max_position);
    assert(max_position > static_cast<float>(epui::Canvas::Width) + 2.0f);
    assert(!ui.animating());
    assert(ui.page_index() == 0);

    epui::PageTransitionStyle soft;
    soft.spring_stiffness = 0.24f;
    soft.spring_damping = 0.60f;
    ui.set_transition_style(soft);
    assert(std::fabs(ui.transition_style().spring_stiffness - 0.24f) < 0.001f);

    return 0;
}
