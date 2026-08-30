# Porting & Hardware / 移植与硬件接入

[中文](#中文) · [English](#english)

---

## 中文

EmbedPluginUI 的移植原则是：**把厂商 SDK 和操作系统头文件留在边缘，核心只看小型抽象接口。**

## 1. Display transport

最基础的边界是 `OledTransport`：

```cpp
class OledTransport {
public:
    virtual ~OledTransport() = default;
    virtual bool write_command(const std::uint8_t* data, std::size_t size) = 0;
    virtual bool write_data(const std::uint8_t* data, std::size_t size) = 0;
    virtual void delay_ms(std::uint32_t ms) = 0;
};
```

你可以：

- 自己派生 `OledTransport`；
- 使用 callback transport 包装已有 HAL/I2C/SPI 代码；
- 使用框架已有 STM32/ESP-IDF/Linux I2C 适配层。

## 2. Callback transport

```cpp
#include <epui/callback_transport.hpp>
#include <epui/oled.hpp>

bool write_bus(void* ctx, bool data_mode,
               const std::uint8_t* data, std::size_t size) {
    // HAL_I2C_Master_Transmit(...)
    // i2c_master_transmit(...)
    // Wire.write(...)
    // write(fd, ...)
    return true;
}

void delay_ms(void*, std::uint32_t ms) {
    // HAL_Delay / vTaskDelay / sleep / busy wait
}

epui::CallbackTransport transport(nullptr, write_bus, delay_ms);
epui::Oled128x64 oled(transport, epui::OledController::SSD1306);
```

核心不知道 `HAL_I2C_HandleTypeDef`、`i2c_master_bus_handle_t` 或 Linux fd 是什么。

## 3. STM32 HAL

框架提供 STM32 HAL adapter 边界，但公共头文件故意不 include 具体 STM32 HAL 版本。推荐在你的工程中写薄 wrapper：

```text
STM32 application
   |
   +-- HAL callbacks / hooks
   |
Stm32HalPlugin / CallbackI2cTransport
   |
Oled128x64
   |
Ui + Canvas
```

这样 F4/F7/H7/G4 等系列只替换 hook，不污染 UI 核心。

## 4. ESP32 / ESP-IDF

同样通过 ESP-IDF hooks 把：

- I2C write；
- GPIO read；
- delay/time；
- 可选 heap diagnostics

接到 portable API。

UI 页面本身不应该 include `driver/i2c.h`、FreeRTOS 或 ESP-IDF 头文件。

## 5. GPIO 输入

推荐把平台读脚函数变成 callback：

```text
GPIO -> read callback -> GpioButtonPlugin -> InputEvent -> Ui
```

按钮插件负责 debounce 和事件排队；板卡代码只负责“这个 pin 现在是高还是低”。

## 6. 旋转编码器

```text
A/B GPIO -> EncoderInputPlugin -> Next / Prev
Button    -> GpioButtonPlugin   -> Select
```

这对三键/旋钮式 OLED UI 很适合。

## 7. 键盘

键盘/evdev/Win32/X11 端应该尽量输出语义键：

```text
Arrow keys -> Up/Down/Left/Right
Enter      -> Select
Esc        -> Back
Ctrl+Up    -> ScrollUp
Ctrl+Down  -> ScrollDown
```

不要让平台虚拟键码进入 Page 代码。

## 8. 时间

所有动画使用应用传入的 `now_ms`，所以平台只需要提供单调递增的毫秒计时。

推荐：

```text
Desktop simulator -> ~60 FPS
MCU               -> 30 FPS 是很好的起点
```

动画内部会限制最大 frame delta，避免某一帧阻塞后弹簧突然“瞬移”。

## 9. 内存预算

基础 Canvas：

```text
128 * 64 / 8 = 1024 bytes
```

核心页面过渡不需要第二 framebuffer。

其他容量主要由模板/固定数组决定：

- Menu navigation depth；
- PluginRegistry capacity；
- EventBus subscriptions；
- Animation tracks；
- Input queue；
- Terminal history/columns；
- Line editor history；
- Popup text buffers。

小 MCU 移植时优先调低这些显式容量，而不是修改渲染核心。

## 10. 推荐移植检查表

```text
[ ] C++17 编译通过
[ ] 1024-byte framebuffer 可接受
[ ] monotonic millis() 可用
[ ] OLED command/data write 已实现
[ ] SSD1306/SH1106 init 成功
[ ] present() 可稳定刷屏
[ ] 输入转换成 InputEvent
[ ] 30/60 FPS 主循环不阻塞
[ ] Diagnostics 能测 render/transfer
[ ] vendor headers 未进入 include/epui
[ ] 所有 public headers 可独立编译
```

## 11. 新显示控制器

如果要加入 SSD1309/SSD1315/SH1107/CH1115 等，建议不要把控制器差异塞进 Page。正确层次是：

```text
Page/Widget
   -> Canvas
   -> Display/OLED controller implementation
   -> Transport
   -> Hardware bus
```

---

## English

EmbedPluginUI's porting rule is simple: **keep vendor SDK and OS headers at the edge; the portable core should see only small framework interfaces.**

### Display transport

Implement `OledTransport`, use a callback transport around existing board functions, or use one of the provided STM32/ESP-IDF/Linux boundaries. The core should not know vendor handle types or file descriptors.

### STM32 and ESP32

Public adapters intentionally avoid including a particular vendor SDK version. Create thin application-side hooks around your exact HAL/IDF APIs and connect them to the framework transport/platform plugin.

### GPIO and encoder input

Convert a platform pin-read function into `GpioButtonPlugin` input. Let the plugin handle debounce and event queuing. Quadrature encoders map naturally to `Next/Prev`, while a push button can emit `Select`.

### Keyboard ports

Translate native key codes to semantic `Key` values before they reach pages. Keep X11/Win32/evdev codes out of application UI logic.

### Timing

Animations only need a monotonic millisecond clock passed as `now_ms`. Desktop simulators target roughly 60 FPS; 30 FPS is a good MCU starting point. Animation integration caps long frame deltas to prevent a delayed frame from causing a visual teleport.

### Memory budget

The base 128×64 framebuffer is 1024 bytes and page transitions do not require a second framebuffer. Most other memory is controlled through explicit fixed capacities: plugin count, menu depth, queues, subscriptions, animation tracks, terminal history, line-editor history, and popup buffers.

### New display controllers

Add controller-specific behavior below Canvas, never inside Page/Widget code:

```text
Page/Widget -> Canvas -> Display/controller -> Transport -> Hardware bus
```
