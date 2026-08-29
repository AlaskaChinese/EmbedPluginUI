#include "epui/animation_plugin.hpp"
#include "epui/encoder_input_plugin.hpp"
#include "epui/event_bus.hpp"
#include "epui/gpio_input_plugin.hpp"
#include "epui/platform/esp32_idf.hpp"
#include "epui/platform/stm32_hal.hpp"
#include "epui/oled_display_plugin.hpp"
#include "epui/plugin_registry.hpp"
#include "epui/standard_widgets.hpp"
#include "epui/theme_plugin.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

class OrderedPlugin final : public epui::Plugin {
public:
    OrderedPlugin(const char* name, int& counter, const char* const* deps = nullptr, std::size_t dep_count = 0)
        : name_(name), counter_(counter), deps_{deps, dep_count} {}
    const char* name() const override { return name_; }
    epui::PluginKind kind() const override { return epui::PluginKind::Service; }
    epui::PluginDependencies dependencies() const override { return deps_; }
    bool start() override { start_order = ++counter_; return true; }
    void stop() override { stopped = true; }
    int start_order{0};
    bool stopped{false};
private:
    const char* name_;
    int& counter_;
    epui::PluginDependencies deps_;
};

struct CounterEvent { int delta; };
struct EventSink { int value{0}; };
void on_counter(void* user, const CounterEvent& event) {
    static_cast<EventSink*>(user)->value += event.delta;
}

struct PinBank { bool levels[16]{}; };
bool read_pin(void* user, int pin) {
    return static_cast<PinBank*>(user)->levels[pin];
}

struct PlatformMock {
    bool initialized{false};
    bool deinitialized{false};
    bool pins[16]{};
    std::size_t writes{0};
    std::size_t bytes{0};
    std::uint8_t last_prefix{0};
};
bool platform_init(void* user) {
    static_cast<PlatformMock*>(user)->initialized = true;
    return true;
}
void platform_deinit(void* user) { static_cast<PlatformMock*>(user)->deinitialized = true; }
bool platform_write(void* user, std::uint8_t address, const std::uint8_t* data, std::size_t size) {
    auto& mock = *static_cast<PlatformMock*>(user);
    assert(address == 0x3C);
    assert(data != nullptr && size > 0);
    ++mock.writes;
    mock.bytes += size;
    mock.last_prefix = data[0];
    return true;
}
void platform_delay(void*, std::uint32_t) {}
bool platform_gpio(void* user, int pin) { return static_cast<PlatformMock*>(user)->pins[pin]; }

} // namespace

int main() {
    int counter = 0;
    const char* page_deps[] = {"sensor"};
    OrderedPlugin page("page", counter, page_deps, 1);
    OrderedPlugin sensor("sensor", counter);
    epui::PluginRegistry graph;
    assert(graph.add(page));
    assert(graph.add(sensor));
    assert(graph.start_all());
    assert(sensor.start_order == 1);
    assert(page.start_order == 2);
    graph.stop_all();
    assert(page.stopped && sensor.stopped);

    int missing_counter = 0;
    const char* missing_deps[] = {"not-registered"};
    OrderedPlugin missing("missing", missing_counter, missing_deps, 1);
    epui::PluginRegistry missing_registry;
    assert(missing_registry.add(missing));
    assert(!missing_registry.start_all());
    assert(missing_registry.last_error() == epui::RegistryError::MissingDependency);
    assert(missing_registry.error_plugin() == &missing);
    assert(std::strcmp(missing_registry.error_dependency(), "not-registered") == 0);

    int cycle_counter = 0;
    const char* a_deps[] = {"b"};
    const char* b_deps[] = {"a"};
    OrderedPlugin cycle_a("a", cycle_counter, a_deps, 1);
    OrderedPlugin cycle_b("b", cycle_counter, b_deps, 1);
    epui::PluginRegistry cycle_registry;
    assert(cycle_registry.add(cycle_a));
    assert(cycle_registry.add(cycle_b));
    assert(!cycle_registry.start_all());
    assert(cycle_registry.last_error() == epui::RegistryError::DependencyCycle);

    epui::AnimationPlugin<2> animation;
    assert(animation.animate(0, 0.0f, 10.0f, 100, 1000, epui::Easing::Linear));
    animation.tick(1050);
    assert(std::fabs(animation.value(0) - 5.0f) < 0.01f);
    assert(animation.active(0));
    animation.tick(1100);
    assert(std::fabs(animation.value(0) - 10.0f) < 0.01f);
    assert(!animation.active(0));

    epui::EventBusPlugin bus;
    EventSink sink;
    assert((bus.subscribe<CounterEvent, on_counter>(&sink)));
    assert(bus.publish(CounterEvent{3}) == 1);
    assert(sink.value == 3);
    bus.unsubscribe_owner(&sink);
    assert(bus.publish(CounterEvent{2}) == 0);

    epui::Ui ui;
    epui::ThemePlugin theme(epui::Theme::compact());
    epui::ThemedProgressWidget progress("progress", theme, 50, 0.5f);
    epui::WidgetPagePlugin<1> dashboard(ui, "dashboard");
    assert(dashboard.add_widget(progress, 4, 20));
    epui::PluginRegistry widgets;
    assert(widgets.add(dashboard));
    assert(widgets.add(progress));
    assert(widgets.add(theme));
    assert(widgets.start_all());
    assert(ui.page_count() == 1);
    epui::Canvas canvas;
    ui.render(canvas, 0);
    widgets.stop_all();
    assert(ui.page_count() == 0);

    PinBank pins;
    pins.levels[1] = true;
    epui::GpioButtonPlugin<2> buttons("gpio-buttons", &pins, read_pin, 25);
    assert(buttons.add_button(1, epui::Key::Select, true));
    assert(buttons.start());
    buttons.tick(0);
    pins.levels[1] = false;
    buttons.tick(10);
    buttons.tick(20);
    epui::InputEvent event{};
    assert(!buttons.poll(event));
    buttons.tick(40);
    assert(buttons.poll(event));
    assert(event.key == epui::Key::Select && event.pressed);
    pins.levels[1] = true;
    buttons.tick(50);
    buttons.tick(80);
    assert(buttons.poll(event));
    assert(!event.pressed);

    PinBank encoder_pins;
    epui::EncoderInputPlugin<> encoder("encoder", &encoder_pins, read_pin, 2, 3);
    assert(encoder.start());
    encoder_pins.levels[2] = true;  encoder_pins.levels[3] = false; encoder.tick(1);
    encoder_pins.levels[2] = true;  encoder_pins.levels[3] = true;  encoder.tick(2);
    encoder_pins.levels[2] = false; encoder_pins.levels[3] = true;  encoder.tick(3);
    encoder_pins.levels[2] = false; encoder_pins.levels[3] = false; encoder.tick(4);
    assert(encoder.poll(event));
    assert(event.key == epui::Key::Next && event.pressed);

    PlatformMock stm32_mock;
    epui::platform::Stm32HalHooks stm32_hooks{
        &stm32_mock, platform_init, platform_deinit, platform_write, platform_delay, platform_gpio
    };
    epui::platform::Stm32HalPlugin stm32(stm32_hooks);
    assert(stm32.kind() == epui::PluginKind::Platform);
    assert(stm32.start() && stm32_mock.initialized);
    std::uint8_t payload[40]{};
    assert(stm32.oled_transport().write_data(payload, sizeof(payload)));
    assert(stm32_mock.writes == 2);
    assert(stm32_mock.last_prefix == 0x40);
    stm32.stop();
    assert(stm32_mock.deinitialized);

    PlatformMock composed_mock;
    epui::platform::Stm32HalHooks composed_hooks{
        &composed_mock, platform_init, platform_deinit, platform_write, platform_delay, platform_gpio
    };
    epui::platform::Stm32HalPlugin composed_platform(composed_hooks);
    epui::Oled128x64 composed_oled(composed_platform.oled_transport());
    epui::OledDisplayPlugin composed_display(composed_oled, "oled", "stm32-hal");
    epui::PluginRegistry platform_graph;
    assert(platform_graph.add(composed_display));
    assert(platform_graph.add(composed_platform));
    assert(platform_graph.start_all());
    assert(composed_mock.initialized);
    platform_graph.stop_all();
    assert(composed_mock.deinitialized);

    PlatformMock esp_mock;
    epui::platform::Esp32IdfHooks esp_hooks{
        &esp_mock, platform_init, platform_deinit, platform_write, platform_delay, platform_gpio
    };
    epui::platform::Esp32IdfPlugin esp32(esp_hooks);
    assert(esp32.start() && esp_mock.initialized);
    assert(esp32.oled_transport().write_command(payload, 1));
    assert(esp_mock.last_prefix == 0x00);
    esp32.stop();
    assert(esp_mock.deinitialized);

    return 0;
}
