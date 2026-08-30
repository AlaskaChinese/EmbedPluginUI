# Quick Start / 快速开始

[中文](#中文) · [English](#english)

---

## 中文

### 1. 环境要求

- CMake >= 3.16
- C++17 编译器
- Ubuntu/Linux 模拟器：X11 开发库
- Windows：Visual Studio / MSVC + CMake

Ubuntu 22.04：

```bash
sudo apt update
sudo apt install -y build-essential cmake libx11-dev
```

### 2. 编译框架与桌面模拟器

```bash
git clone https://github.com/AlaskaChinese/EmbedPluginUI.git
cd EmbedPluginUI
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/epui_sim
```

常用 CMake 开关：

```text
EPUI_BUILD_WINDOWS_SIM   Windows 模拟器
EPUI_BUILD_LINUX_SIM     Ubuntu/Linux X11 模拟器
EPUI_BUILD_RPI           Raspberry Pi 5 实机应用
EPUI_BUILD_RPI_SIM       Raspberry Pi 5 应用的 X11 模拟版本
EPUI_BUILD_TESTS         测试
```

### 3. 在你自己的 CMake 项目中引用

如果把 EmbedPluginUI 作为子目录/子模块放进项目：

```cmake
add_subdirectory(third_party/EmbedPluginUI)

target_link_libraries(my_app PRIVATE epui)
```

然后：

```cpp
#include <epui/canvas.hpp>
#include <epui/page.hpp>
```

### 4. 最小页面

```cpp
#include <epui/canvas.hpp>
#include <epui/page.hpp>

class HelloPage final : public epui::Page {
public:
    void draw(epui::Canvas& canvas, std::uint32_t) override {
        canvas.text(8, 12, "EmbedPluginUI");
        canvas.round_rect(6, 24, 116, 22, 5);
        canvas.text(16, 32, "Hello OLED");
    }
};

int main() {
    epui::Canvas canvas;
    epui::Ui ui;
    HelloPage hello;

    ui.add_page(hello);
    ui.render(canvas, 0);

    // canvas.data() 指向 1024-byte 1-bit framebuffer。
}
```

### 5. 典型主循环

平台层负责：

1. 获取毫秒时间；
2. 轮询输入并转换成 `InputEvent`；
3. 调用 `ui.handle(...)`；
4. `ui.render(...)`；
5. 把 framebuffer 发送到显示器。

```cpp
while (running) {
    const std::uint32_t now = platform_millis();

    epui::InputEvent event{};
    while (input.poll(event)) {
        ui.handle(event, now);
    }

    ui.render(canvas, now);
    display.present(canvas);
}
```

### 6. 直接连接 OLED

如果你的板卡已经有 I2C/SPI 写函数，最简单的方法是实现 `OledTransport`，或使用 callback transport 包装现有函数：

```cpp
#include <epui/callback_transport.hpp>
#include <epui/oled.hpp>

bool write_bus(void* ctx, bool data_mode,
               const std::uint8_t* data, std::size_t size) {
    // 调用 HAL / ESP-IDF / Arduino / Linux I2C
    return true;
}

void delay_ms(void*, std::uint32_t ms) {
    // 平台延时
}

epui::CallbackTransport transport(nullptr, write_bus, delay_ms);
epui::Oled128x64 oled(transport, epui::OledController::SSD1306);

oled.init();
oled.present(canvas.data(), epui::Canvas::BufferSize);
```

SH1106：

```cpp
epui::Oled128x64 oled(transport, epui::OledController::SH1106);
```

### 7. 下一步

- 页面/输入： [Navigation & Input](Navigation-and-Input)
- 菜单： [Menu & Animation](Menu-and-Animation)
- 插件组合： [Plugin Runtime](Plugin-Runtime)
- MCU 移植： [Porting & Hardware](Porting-and-Hardware)

---

## English

### 1. Requirements

- CMake 3.16 or newer
- a C++17 compiler
- X11 development headers for the Linux simulator
- Visual Studio/MSVC + CMake for the native Windows simulator

Ubuntu 22.04:

```bash
sudo apt update
sudo apt install -y build-essential cmake libx11-dev
```

### 2. Build the framework and simulator

```bash
git clone https://github.com/AlaskaChinese/EmbedPluginUI.git
cd EmbedPluginUI
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/epui_sim
```

Important CMake options:

```text
EPUI_BUILD_WINDOWS_SIM   Native Win32 simulator
EPUI_BUILD_LINUX_SIM     Ubuntu/Linux X11 simulator
EPUI_BUILD_RPI           Raspberry Pi 5 hardware application
EPUI_BUILD_RPI_SIM       Raspberry Pi application using X11
EPUI_BUILD_TESTS         Tests
```

### 3. Use EmbedPluginUI from another CMake project

Vendor or add the repository as a submodule, then:

```cmake
add_subdirectory(third_party/EmbedPluginUI)
target_link_libraries(my_app PRIVATE epui)
```

Public headers live under `epui/...`.

### 4. Minimal page

```cpp
#include <epui/canvas.hpp>
#include <epui/page.hpp>

class HelloPage final : public epui::Page {
public:
    void draw(epui::Canvas& canvas, std::uint32_t) override {
        canvas.text(8, 12, "EmbedPluginUI");
        canvas.round_rect(6, 24, 116, 22, 5);
        canvas.text(16, 32, "Hello OLED");
    }
};

int main() {
    epui::Canvas canvas;
    epui::Ui ui;
    HelloPage hello;

    ui.add_page(hello);
    ui.render(canvas, 0);
    // canvas.data() is the 1024-byte 1-bit framebuffer.
}
```

### 5. Typical application loop

Your platform layer should obtain time, poll input, route events, render, and present the framebuffer:

```cpp
while (running) {
    const std::uint32_t now = platform_millis();

    epui::InputEvent event{};
    while (input.poll(event)) {
        ui.handle(event, now);
    }

    ui.render(canvas, now);
    display.present(canvas);
}
```

### 6. Bind an OLED transport

Use `OledTransport` directly or wrap existing board functions with a callback transport:

```cpp
#include <epui/callback_transport.hpp>
#include <epui/oled.hpp>

bool write_bus(void* ctx, bool data_mode,
               const std::uint8_t* data, std::size_t size) {
    // HAL / ESP-IDF / Arduino / Linux I2C
    return true;
}

void delay_ms(void*, std::uint32_t ms) {
    // platform delay
}

epui::CallbackTransport transport(nullptr, write_bus, delay_ms);
epui::Oled128x64 oled(transport, epui::OledController::SSD1306);

oled.init();
oled.present(canvas.data(), epui::Canvas::BufferSize);
```

Use `OledController::SH1106` for SH1106 panels.

### 7. Continue reading

- Pages and input: [Navigation & Input](Navigation-and-Input)
- Menus: [Menu & Animation](Menu-and-Animation)
- Application composition: [Plugin Runtime](Plugin-Runtime)
- MCU/board porting: [Porting & Hardware](Porting-and-Hardware)
