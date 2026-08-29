#include "epui/plugin_registry.hpp"
#include "epui/input_plugin.hpp"
#include "epui/oled_display_plugin.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {
class DummyPlugin final : public epui::Plugin {
public:
    DummyPlugin(const char* n, bool ok = true) : n_(n), ok_(ok) {}
    const char* name() const override { return n_; }
    epui::PluginKind kind() const override { return epui::PluginKind::Service; }
    bool start() override { ++starts; return ok_; }
    void stop() override { ++stops; }
    int starts{0};
    int stops{0};
private:
    const char* n_;
    bool ok_;
};

class MockTransport final : public epui::OledTransport {
public:
    bool write_command(const std::uint8_t*, std::size_t size) override { command_bytes += size; return true; }
    bool write_data(const std::uint8_t*, std::size_t size) override { data_bytes += size; return true; }
    void delay_ms(std::uint32_t) override {}
    std::size_t command_bytes{0};
    std::size_t data_bytes{0};
};
}

int main() {
    epui::PluginRegistry registry;
    DummyPlugin a("a"), b("b"), duplicate("a");
    assert(registry.add(a));
    assert(registry.add(b));
    assert(!registry.add(duplicate));
    assert(registry.size() == 2);
    assert(registry.find("a") == &a);
    assert(registry.start_all());
    assert(a.starts == 1 && b.starts == 1);
    registry.stop_all();
    assert(a.stops == 1 && b.stops == 1);

    epui::PluginRegistry rollback;
    DummyPlugin good("good"), bad("bad", false);
    assert(rollback.add(good));
    assert(rollback.add(bad));
    assert(!rollback.start_all());
    assert(good.starts == 1 && good.stops == 1);
    assert(bad.starts == 1 && bad.stops == 0);

    epui::QueuedInputPlugin<2> input("buttons");
    assert(input.push({epui::Key::Next, true}));
    assert(input.push({epui::Key::Select, true}));
    assert(!input.push({epui::Key::Back, true}));
    epui::InputEvent event{};
    assert(input.poll(event) && event.key == epui::Key::Next);
    assert(input.poll(event) && event.key == epui::Key::Select);
    assert(!input.poll(event));

    MockTransport transport;
    epui::Oled128x64 oled(transport, epui::OledController::SSD1306);
    epui::OledDisplayPlugin display(oled);
    epui::Canvas canvas;
    assert(display.kind() == epui::PluginKind::Display);
    assert(display.start());
    assert(display.present(canvas));
    assert(transport.command_bytes > 0);
    assert(transport.data_bytes == epui::Canvas::BufferSize);
    display.stop();
    return 0;
}
