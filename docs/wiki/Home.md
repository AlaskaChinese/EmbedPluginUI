# EmbedPluginUI Wiki

> **A plugin-first embedded UI framework.**  
> 面向小型嵌入式显示设备的插件优先 C++17 UI 框架。

[中文](#中文) · [English](#english)

---

## 中文

### 这是什么？

EmbedPluginUI 是一个面向小型嵌入式显示屏的 C++17 UI 框架。当前重点目标是 128×64 单色 OLED，但显示控制器、传输层、输入、页面、菜单、组件、动画、调试与平台适配都以可替换的插件边界组织。

核心设计原则：

- **万物皆插件**：容易变化的部分都放在插件/适配器边界；
- **MCU 友好**：核心不依赖操作系统、动态库加载或 RTTI 服务发现；
- **确定性资源占用**：128×64 1-bit Canvas 为 1024 字节 framebuffer，核心渲染不要求堆分配；
- **同一套 UI 多平台运行**：Ubuntu/X11、Windows、Raspberry Pi、STM32、ESP32 可共享页面与组件代码；
- **动画优先**：页面、菜单、弹窗使用统一的弹簧/果冻运动模型，同时保留适合 1-bit OLED 的清晰像素风格。

### 从哪里开始？

如果你第一次使用 EmbedPluginUI：

1. 阅读 **[Quick Start](Quick-Start)**，先把桌面模拟器跑起来；
2. 阅读 **[Core API](Core-API)**，理解 `Canvas`、`Ui`、`Page`、`UiOverlay` 和 `Oled128x64`；
3. 需要设置菜单时看 **[Menu & Animation](Menu-and-Animation)**；
4. 需要组合完整应用时看 **[Plugin Runtime](Plugin-Runtime)**；
5. 准备上 STM32/ESP32/其他板卡时看 **[Porting & Hardware](Porting-and-Hardware)**；
6. Raspberry Pi 5 完整应用见 **[Raspberry Pi 5](Raspberry-Pi-5)**。

### 当前主要能力

- 128×64 单色 Canvas 与 UTF-8 小型符号集；
- SSD1306 / SH1106；
- 弹簧式顶层 Page 切换；
- 任意深度静态菜单树；
- `Indicator` / `GlideFrame` / `SlideFrame` / `LiquidGlass` 四种菜单动画；
- 像素级菜单 viewport、安全区与长菜单粘性滚动；
- GPIO 按钮、旋转编码器、键盘与字符输入；
- 固定容量插件注册、依赖排序、生命周期管理；
- Widget / Theme / Animation / EventBus / Sensor / Service 插件；
- Popup 模态弹窗；
- 数学曲线绘制、圆形、进度条等图形；
- `TerminalView`、命令行编辑、历史记录与 Raspberry Pi PTY Shell；
- FPS、帧时间、渲染耗时、内存、显示传输诊断；
- Ubuntu/X11 与 Win32 模拟器；
- Raspberry Pi 5 OLED 控制台应用；
- STM32 HAL / ESP-IDF / callback transport 适配边界。

### 许可证

EmbedPluginUI 是 **source-available（源码可见）双授权项目**，不是 OSI 定义下的 Open Source。个人、学习、研究与非商业用途按照仓库 `LICENSE` 免费授权；商业使用必须在开始商业使用之前获得单独的书面商业授权。

详见 **[Licensing](Licensing)**。

---

## English

### What is EmbedPluginUI?

EmbedPluginUI is a portable C++17 UI framework for small embedded displays. Its current primary target is the common 128×64 monochrome OLED, while display controllers, transports, input devices, pages, menus, widgets, animation, diagnostics, and platform integrations are designed around replaceable plugin boundaries.

Core principles:

- **Everything that varies is a plugin boundary.**
- **MCU friendly:** the portable core has no OS dependency, runtime shared-library loading, or RTTI service discovery.
- **Deterministic resources:** the 128×64 1-bit Canvas uses a 1024-byte framebuffer and core rendering does not require heap allocation.
- **Shared UI across platforms:** the same pages/widgets can run on Ubuntu/X11, Windows, Raspberry Pi, STM32, and ESP32.
- **Animation first:** page, menu, and popup motion uses reusable spring/jelly models designed to remain crisp on 1-bit OLEDs.

### Recommended reading order

1. **[Quick Start](Quick-Start)** — build and run the simulator.
2. **[Core API](Core-API)** — learn `Canvas`, `Ui`, `Page`, `UiOverlay`, and `Oled128x64`.
3. **[Menu & Animation](Menu-and-Animation)** — settings UI, focus, long menus, and cursor styles.
4. **[Plugin Runtime](Plugin-Runtime)** — compose a complete application with deterministic dependencies.
5. **[Porting & Hardware](Porting-and-Hardware)** — bind your MCU/board transport and input functions.
6. **[Raspberry Pi 5](Raspberry-Pi-5)** — use the complete OLED console application.

### Major capabilities

- 128×64 monochrome Canvas and compact UTF-8 symbol support;
- SSD1306 / SH1106 support;
- spring-driven top-level page transitions;
- arbitrary-depth static menu trees;
- `Indicator`, `GlideFrame`, `SlideFrame`, and `LiquidGlass` menu motion;
- pixel-clipped menu viewports and sticky long-menu scrolling;
- GPIO buttons, rotary encoders, keyboards, and character input;
- fixed-capacity plugin registration, dependency ordering, and lifecycle management;
- widgets, themes, animation tracks, typed EventBus, sensors, and services;
- modal popup overlay;
- mathematical plots and graphics primitives;
- `TerminalView`, command editing/history, and Raspberry Pi PTY shell integration;
- FPS, frame-time, render-time, memory, and transfer diagnostics;
- Ubuntu/X11 and native Win32 simulators;
- Raspberry Pi 5 OLED console application;
- STM32 HAL, ESP-IDF, and callback-based transport boundaries.

### Licensing

EmbedPluginUI uses a **source-available dual-license model** and is not OSI Open Source. Personal, educational, research, and other non-commercial use is available under the repository `LICENSE`; commercial use requires a separate written commercial license **before commercial use begins**.

See **[Licensing](Licensing)** for details.
