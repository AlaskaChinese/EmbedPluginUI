#include "epui/plugin_registry.hpp"
#include "epui/input_plugin.hpp"
#include "epui/oled_display_plugin.hpp"
#include "epui/page_plugin.hpp"
#include "epui/sensor_plugin.hpp"
#include "epui/service_plugin.hpp"
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
    void tick(std::uint32_t) override { ++ticks; }
    void stop() override { ++stops; }
    int starts{0};
    int ticks{0};
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

class TestPage final : public epui::PagePlugin {
public:
    explicit TestPage(epui::Ui& ui) : PagePlugin(ui, "test-page") {}
    void draw(epui::Canvas& canvas, std::uint32_t) override { canvas.pixel(1, 1); }
};

struct TestSnapshot { int value{0}; };
class TestSensor final : public epui::SensorPlugin<TestSnapshot> {
public:
    TestSensor() : SensorPlugin(100) {}
    const char* name() const override { return "test-sensor"; }
    void refresh() { reset_schedule(); }
protected:
    bool sample(TestSnapshot& out, std::uint32_t) override { out.value = ++samples; return true; }
public:
    int samples{0};
};

class TestService final : public epui::PeriodicServicePlugin {
public:
    TestService() : PeriodicServicePlugin(50) {}
    const char* name() const override { return "test-service"; }
protected:
    void update(std::uint32_t) override { ++updates; }
public:
    int updates{0};
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
    registry.tick_all(10);
    assert(a.starts == 1 && b.starts == 1);
    assert(a.ticks == 1 && b.ticks == 1);
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

    epui::Ui ui;
    TestPage page(ui);
    assert(page.kind() == epui::PluginKind::Page);
    assert(ui.page_count() == 0);
    assert(page.start());
    assert(page.attached());
    assert(ui.page_count() == 1);
    page.stop();
    assert(ui.page_count() == 0);

    TestSensor sensor;
    assert(sensor.kind() == epui::PluginKind::Sensor);
    sensor.tick(1000);
    assert(sensor.valid());
    assert(sensor.snapshot().value == 1);
    sensor.tick(1050);
    assert(sensor.snapshot().value == 1);
    sensor.tick(1100);
    assert(sensor.snapshot().value == 2);
    assert(sensor.sample_count() == 2);
    sensor.refresh();
    sensor.tick(1101);
    assert(sensor.snapshot().value == 3);

    TestService service;
    service.tick(1000);
    service.tick(1020);
    service.tick(1050);
    assert(service.updates == 2);

    epui::PluginRegistry composition;
    TestSensor composed_sensor;
    TestService composed_service;
    TestPage composed_page(ui);
    assert(composition.add(composed_sensor));
    assert(composition.add(composed_service));
    assert(composition.add(composed_page));
    assert(composition.start_all());
    composition.tick_all(2000);
    assert(composed_sensor.valid());
    assert(composed_service.updates == 1);
    assert(ui.page_count() == 1);
    composition.stop_all();
    assert(ui.page_count() == 0);

    return 0;
}
