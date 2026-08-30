#include "epui/input_plugin.hpp"
#include "epui/popup_plugin.hpp"
#include <cassert>
#include <cmath>

namespace {

class TestPage final : public epui::Page {
public:
    void draw(epui::Canvas& canvas, std::uint32_t) override { canvas.pixel(1, 1); }
    void on_key(epui::Key) override { ++keys; }
    void on_char(char) override { ++characters; }
    int keys{0};
    int characters{0};
};

struct CallbackState {
    bool called{false};
    epui::PopupResult result{epui::PopupResult::Cancelled};
};

void record_result(void* user, epui::PopupResult result) {
    auto* state = static_cast<CallbackState*>(user);
    state->called = true;
    state->result = result;
}

void settle(epui::Ui& ui, epui::Canvas& canvas, epui::PopupPlugin& popup,
            std::uint32_t& now, float* maximum = nullptr, int* maximum_stretch = nullptr) {
    for (int frame = 0; frame < 240 && popup.animating(); ++frame) {
        ui.render(canvas, now);
        if (maximum && popup.position() > *maximum) *maximum = popup.position();
        if (maximum_stretch && popup.stretch_pixels() > *maximum_stretch) {
            *maximum_stretch = popup.stretch_pixels();
        }
        now += 16;
    }
    assert(!popup.animating());
}

} // namespace

int main() {
    epui::Ui ui;
    epui::Canvas canvas;
    TestPage first;
    TestPage second;
    epui::PopupPlugin popup(ui);
    CallbackState callback;

    assert(ui.add_page(first));
    assert(ui.add_page(second));
    assert(popup.start());
    assert(popup.kind() == epui::PluginKind::Overlay);

    popup.show("Confirm", u8"CPU reached 80℃", epui::PopupButtons::OkCancel,
               record_result, &callback);
    assert(popup.visible() && popup.captures_input());
    const float hidden_position = popup.position();

    ui.handle(epui::Key::Next, 0);
    assert(popup.selected_index() == 1);
    assert(!ui.animating() && ui.page_index() == 0);
    ui.handle(epui::InputEvent{epui::Key::Select, true, 'x'}, 0);
    assert(first.characters == 0);

    std::uint32_t now = 0;
    float maximum = hidden_position;
    int maximum_stretch = 0;
    settle(ui, canvas, popup, now, &maximum, &maximum_stretch);
    assert(popup.state() == epui::PopupState::Visible);
    assert(maximum > static_cast<float>(popup.style().resting_y));
    assert(maximum_stretch > 0);

    ui.handle(epui::Key::Prev, now);
    assert(popup.selected_index() == 0);
    ui.handle(epui::Key::Select, now);
    assert(popup.state() == epui::PopupState::Closing);
    assert(!callback.called);
    ui.handle(epui::Key::Next, now);
    assert(!ui.animating());
    int closing_stretch = 0;
    settle(ui, canvas, popup, now, nullptr, &closing_stretch);
    assert(closing_stretch > 0);
    assert(!popup.visible());
    assert(callback.called && callback.result == epui::PopupResult::Accepted);

    ui.handle(epui::Key::Next, now);
    assert(ui.animating());

    epui::Spring1D spring;
    epui::SpringStyle style;
    spring.reset(-20.0f);
    spring.set_target(10.0f);
    for (int i = 0; i < 8; ++i) spring.step(16, style);
    spring.set_target(-20.0f);
    for (int i = 0; i < 240 && !spring.settled(style); ++i) spring.step(16, style);
    assert(spring.settled(style));
    assert(std::fabs(spring.position() + 20.0f) < 0.001f);

    popup.stop();
    return 0;
}
