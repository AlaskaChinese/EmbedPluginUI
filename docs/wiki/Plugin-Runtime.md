# Plugin Runtime / 插件运行时

[中文](#中文) · [English](#english)

---

## 中文

EmbedPluginUI 的“插件”不是桌面系统中的 `.so/.dll` 动态插件，而是**编译期静态组合、运行时固定容量注册**的可替换模块边界。

### 生命周期

```text
construct
  -> registry.add()
  -> dependency resolution
  -> start()
  -> tick(now_ms)
  -> stop()
```

`PluginRegistry::start_all()` 会：

- 按名称解析依赖；
- 保证依赖先启动；
- 检测重复插件名；
- 检测缺失依赖；
- 检测依赖环；
- 某插件启动失败时，按已启动顺序逆序回滚。

```cpp
epui::PluginRegistry plugins;
plugins.add(page);
plugins.add(sensor);
plugins.add(display);

if (!plugins.start_all()) {
    // inspect registry.error(), error_plugin(), error_dependency()
}
```

### 插件种类

典型边界：

```text
DisplayPlugin    显示输出
InputPlugin      输入事件
SensorPlugin<T>  周期采样的类型化快照
ServicePlugin    后台服务
PagePlugin       页面 + 自动 Ui 挂载
WidgetPlugin     可复用组件
AnimationPlugin  固定容量 tween
ThemePlugin      主题/布局参数
PlatformPlugin   板级/SDK 边界
Debug plugin     Diagnostics 等
Menu plugin      多级菜单
```

### 依赖声明

```cpp
class NetworkPage : public epui::PagePlugin {
public:
    using PagePlugin::PagePlugin;

    epui::PluginDependencies dependencies() const override {
        return {deps_, 1};
    }

private:
    const char* deps_[1]{"network-monitor"};
};
```

注册顺序不要求等于启动顺序：

```cpp
plugins.add(network_page);
plugins.add(network_monitor);
plugins.start_all(); // network-monitor 先启动
```

### `SensorPlugin<Snapshot>`

适合周期性状态数据：

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

可通过 `valid()`、`last_sample_ok()`、`sample_count()` 观察状态。后一次采样失败时，最后一个有效 snapshot 仍可保留。

### `ServicePlugin` / `PeriodicServicePlugin`

适合：

- 网络状态；
- FIFO/PTTY 读取；
- 状态同步；
- 非视觉周期任务。

页面不要直接依赖 Linux/STM32/ESP32 SDK；让 Service/Platform plugin 把平台细节封装在边界外。

### `AnimationPlugin<N>`

固定容量动画轨道：

```cpp
epui::AnimationPlugin<4> animation;
animation.animate(
    0,
    0.0f,
    1.0f,
    220,
    now,
    epui::Easing::EaseOutCubic
);

plugins.add(animation);
plugins.tick_all(now);
float value = animation.value(0);
```

### Widget 与 Theme

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

依赖可以形成：

```text
dashboard -> load widget -> theme
```

### Typed EventBus

用于离散事件，而不是稳定状态：

```cpp
struct BatteryLow { float voltage; };

void on_battery_low(void* user, const BatteryLow& event) {
    // ...
}

epui::EventBusPlugin events;
events.subscribe<BatteryLow, on_battery_low>(this);
events.publish(BatteryLow{4.65f});
```

原则：

- 稳定状态依赖：直接持有 Sensor/Service 的类型化引用；
- 离散通知：EventBus。

### 推荐的完整应用组合

```text
Application
├── PluginRegistry
├── PlatformPlugin
├── DisplayPlugin
├── InputPlugin(s)
├── Sensor/Service plugins
├── Theme/Animation plugins
├── PagePlugin(s)
├── Popup/Diagnostics overlays
└── Ui + Canvas
```

主循环保持简单：

```cpp
while (running) {
    const auto now = millis();
    plugins.tick_all(now);

    epui::InputEvent event{};
    while (input.poll(event)) ui.handle(event, now);

    ui.render(canvas, now);
    display.present(canvas);
}
```

### 嵌入式约束

框架设计避免：

- `dlopen`；
- 运行时共享库发现；
- RTTI 服务查找；
- 依赖 `std::function`；
- 插件图的堆所有权。

插件、队列、订阅、动画轨道等使用固定容量或应用显式控制的容量。

---

## English

EmbedPluginUI uses the word “plugin” to mean a statically composed, replaceable module boundary. It does **not** require runtime `.so/.dll` loading.

### Lifecycle

```text
construct -> registry.add() -> dependency resolution
          -> start() -> tick(now_ms) -> stop()
```

`PluginRegistry::start_all()` resolves named dependencies, starts dependencies before consumers, detects duplicates/missing dependencies/cycles, and rolls already-started plugins back in reverse order if startup fails.

### Plugin boundaries

Typical kinds include displays, input, typed sensors, services, pages, widgets, animation, themes, platforms, diagnostics, and menus.

### Dependencies

Plugins expose a fixed span of dependency names. Registration order does not need to match startup order; the registry computes a deterministic order.

### Typed sensors

`SensorPlugin<Snapshot>` is appropriate for periodic typed state such as battery, system status, network data, or telemetry. The last valid sample can remain available when a later sample fails.

### Services

Use `ServicePlugin` / `PeriodicServicePlugin` for non-visual work such as platform monitoring, PTY/FIFO reads, network status, or background synchronization. Keep vendor/OS headers outside pages.

### Animation, widgets, and themes

`AnimationPlugin<N>` provides fixed-capacity tween tracks. `WidgetPagePlugin<N>` composes widgets, and theme-aware widgets can declare `ThemePlugin` as a dependency.

### Typed EventBus

Use direct typed references for steady state and `EventBusPlugin` for discrete notifications. The bus is synchronous, typed, and does not require RTTI, `std::function`, or heap allocation.

### Recommended application composition

```text
Application
├── PluginRegistry
├── PlatformPlugin
├── DisplayPlugin
├── InputPlugin(s)
├── Sensor/Service plugins
├── Theme/Animation plugins
├── PagePlugin(s)
├── Popup/Diagnostics overlays
└── Ui + Canvas
```

The main loop should remain a small sequence of plugin ticks, input routing, UI render, and framebuffer presentation.

### Embedded constraints

The runtime intentionally avoids dynamic plugin discovery, shared-library loading, RTTI service lookup, and heap ownership of the plugin graph. Capacities are fixed or explicitly chosen by the application.
