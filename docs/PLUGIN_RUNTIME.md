# EmbedPluginUI plugin runtime

EmbedPluginUI treats every variable subsystem as a statically composed plugin. The runtime is designed for embedded targets: fixed-capacity registration, application-owned objects, deterministic lifecycle, no shared-library loading and no heap-owned plugin graph.

## Lifecycle

Every plugin follows:

```text
construct -> registry.add() -> start() -> tick(now_ms) -> stop()
```

`PluginRegistry::start_all()` starts plugins in registration order. If one plugin fails, already-started plugins are stopped in reverse order. `stop_all()` also shuts down in reverse order.

## Plugin kinds

- `DisplayPlugin`: framebuffer presentation.
- `InputPlugin`: buttons, encoders, keyboards and other input events.
- `SensorPlugin<Snapshot>`: periodically sampled typed data snapshots.
- `ServicePlugin`: non-visual background services.
- `PeriodicServicePlugin`: interval-driven service work.
- `PagePlugin`: page lifecycle plus automatic `Ui` attachment/detachment.
- `WidgetPlugin`: reusable visual components.
- `Platform`: reserved for board/platform integration plugins.

## Typed sensor example

```cpp
struct BatterySnapshot {
    float voltage;
    float current;
};

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

The last valid snapshot remains available if a later sample fails. `valid()`, `last_sample_ok()` and `sample_count()` expose data state without heap allocation or event-bus machinery.

## Page plugin example

```cpp
class BatteryPage : public epui::PagePlugin {
public:
    BatteryPage(epui::Ui& ui, const BatteryPlugin& battery)
        : PagePlugin(ui, "page-battery"), battery_(battery) {}

    void draw(epui::Canvas& canvas, std::uint32_t) override {
        const auto& data = battery_.snapshot();
        // draw data
    }

private:
    const BatteryPlugin& battery_;
};
```

`PagePlugin::start()` automatically attaches the page to `Ui`. `stop()` removes it. This allows registry rollback to restore UI state if later plugin startup fails.

## Composition

```cpp
epui::PluginRegistry plugins;
BatteryPlugin battery;
BatteryPage battery_page(ui, battery);

plugins.add(display);
plugins.add(input);
plugins.add(battery);
plugins.add(battery_page);
plugins.start_all();

while (running) {
    const auto now = millis();
    plugins.tick_all(now);
    // route input -> Ui
    ui.render(canvas, now);
    display.present(canvas);
}

plugins.stop_all();
```

Registration order is the dependency order. Data/service plugins should be registered before pages that consume them.

## Raspberry Pi 5 reference composition

The Pi application is the first complete composition example:

```text
PluginRegistry
├── OledDisplayPlugin
├── TerminalInputPlugin
├── SystemMonitorPlugin       SensorPlugin<StatusSnapshot>, 1000 ms
├── TerminalFeedPlugin        ServicePlugin
├── OverviewPage              PagePlugin
├── NetworkPage               PagePlugin
├── PowerPage                 PagePlugin
├── SystemPage                PagePlugin
└── TerminalPage              PagePlugin
```

Its main loop no longer owns system sampling or page registration. It only ticks plugins, routes input, renders the UI and presents the framebuffer.

## Embedded rules

Plugin implementations should keep these constraints unless a platform-specific port explicitly needs otherwise:

- no dynamic plugin discovery;
- no `dlopen`/shared-object runtime loading;
- no heap ownership required by the framework;
- fixed-capacity registries and queues;
- deterministic startup/shutdown;
- typed direct references for dependencies instead of a global service locator.
