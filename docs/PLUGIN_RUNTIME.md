# EmbedPluginUI plugin runtime

EmbedPluginUI treats every variable subsystem as a statically composed plugin. The runtime is designed for embedded targets: fixed-capacity registration, application-owned objects, deterministic lifecycle, no shared-library loading, no RTTI discovery and no heap-owned plugin graph.

## Lifecycle

Every plugin follows:

```text
construct -> registry.add() -> dependency resolution -> start() -> tick(now_ms) -> stop()
```

`PluginRegistry::start_all()` resolves declared plugin dependencies by name, starts dependencies before consumers, detects missing dependencies and cycles, and records the actual startup order. If startup fails, already-started plugins are stopped in reverse startup order. `stop_all()` uses the same reverse order.

`RegistryError`, `error_plugin()` and `error_dependency()` expose deterministic diagnostics without exceptions or dynamic strings.

## Plugin kinds

- `DisplayPlugin`: framebuffer presentation.
- `InputPlugin`: buttons, encoders, keyboards and other input events.
- `SensorPlugin<Snapshot>`: periodically sampled typed data snapshots.
- `ServicePlugin`: non-visual background services.
- `PeriodicServicePlugin`: interval-driven service work.
- `PagePlugin`: page lifecycle plus automatic `Ui` attachment/detachment.
- `WidgetPlugin`: reusable visual components.
- `AnimationPlugin<N>`: fixed-capacity tween tracks and easing.
- `ThemePlugin`: compact monochrome layout/animation theme values.
- `PlatformPlugin`: board/SDK integration boundary.

## Dependency graph

A plugin declares dependencies with a fixed string span:

```cpp
class NetworkPage : public epui::PagePlugin {
public:
    using PagePlugin::PagePlugin;
    epui::PluginDependencies dependencies() const override { return {deps_, 1}; }
private:
    const char* deps_[1]{"network-monitor"};
};
```

Registration order no longer needs to be dependency order:

```cpp
plugins.add(network_page);
plugins.add(network_monitor);
plugins.start_all(); // monitor starts first automatically
```

Missing names return `RegistryError::MissingDependency`; cycles return `RegistryError::DependencyCycle`.

## Typed sensor example

```cpp
struct BatterySnapshot { float voltage; float current; };
class BatteryPlugin : public epui::SensorPlugin<BatterySnapshot> {
public:
    BatteryPlugin() : SensorPlugin(500) {}
    const char* name() const override { return "battery"; }
protected:
    bool sample(BatterySnapshot& out, std::uint32_t) override {
        out.voltage = read_voltage();
        out.current = read_current();
        return true;
    }
};
```

The last valid snapshot remains available if a later sample fails. `valid()`, `last_sample_ok()` and `sample_count()` expose data state.

## Animation plugin

```cpp
epui::AnimationPlugin<4> animation;
animation.animate(0, 0.0f, 1.0f, 220, now, epui::Easing::EaseOutCubic);
plugins.add(animation);
plugins.tick_all(now);
float alpha = animation.value(0);
```

Tracks are fixed-capacity and allocation-free.

## Widget composition and themes

`WidgetPagePlugin<N>` composes registered widgets. Widgets become dependencies of the page automatically.

```cpp
epui::ThemePlugin theme(epui::Theme::compact());
epui::ThemedProgressWidget load("load", theme, 80, 0.4f);
epui::WidgetPagePlugin<4> page(ui, "dashboard");
page.add_widget(load, 8, 30);

plugins.add(page);
plugins.add(load);
plugins.add(theme);
plugins.start_all();
```

The dependency chain is `dashboard -> load -> theme` even when registered in the opposite order.

## Typed EventBus

`EventBusPlugin` provides synchronous typed messages without RTTI, `std::function` or heap allocation.

```cpp
struct BatteryLow { float voltage; };
void on_battery_low(void* user, const BatteryLow& event) { /* ... */ }
epui::EventBusPlugin events;
events.subscribe<BatteryLow, on_battery_low>(this);
events.publish(BatteryLow{4.65f});
```

Use direct typed references for steady-state data dependencies and the EventBus for discrete notifications.

## GPIO buttons and encoders

`GpioButtonPlugin` accepts a platform-independent pin-read callback, debounces buttons and emits `InputEvent` press/release events. `EncoderInputPlugin` decodes full-step quadrature into `Next`/`Prev` or custom keys. Both use fixed-capacity queues.

## STM32 HAL and ESP-IDF adapters

The public adapters deliberately do not include vendor SDK headers. Applications provide a small hook table around their exact HAL/IDF version:

```cpp
epui::platform::Stm32HalPlugin platform(hooks);
epui::Oled128x64 oled(platform.oled_transport());
epui::OledDisplayPlugin display(oled, "oled", "stm32-hal");

plugins.add(display);
plugins.add(platform);
plugins.start_all(); // platform starts first because display declares the dependency
```

`Esp32IdfPlugin` exposes the same model. `CallbackI2cTransport` inserts OLED I2C control bytes and chunks transfers into fixed stack buffers.

## Raspberry Pi 5 reference composition

```text
PluginRegistry
├── OledDisplayPlugin
├── TerminalInputPlugin
├── SystemMonitorPlugin       SensorPlugin<StatusSnapshot>, 1000 ms
├── TerminalFeedPlugin        ServicePlugin
├── OverviewPage              PagePlugin -> system-monitor
├── NetworkPage               PagePlugin -> system-monitor
├── PowerPage                 PagePlugin -> system-monitor
├── SystemPage                PagePlugin -> system-monitor
└── TerminalPage              PagePlugin -> terminal-feed
```

Its main loop only ticks plugins, routes input, renders the UI and presents the framebuffer.

## Embedded rules

- no `dlopen` or runtime shared-library discovery;
- no heap ownership required by the framework;
- no RTTI-based service lookup;
- fixed-capacity registries, tracks, subscriptions and input queues;
- deterministic dependency resolution and rollback;
- vendor SDK headers stay at the application edge;
- public framework headers must be self-contained and compile independently.
