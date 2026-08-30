# Raspberry Pi 5 / 树莓派 5

[中文](#中文) · [English](#english)

---

## 中文

`apps/rpi5_oled_console` 是一个完整产品级示例：用 128×64 I2C OLED + USB 键盘，把 Raspberry Pi 5 变成无 HDMI 的小型状态面板和交互式 Shell。

## 1. 五个页面

```text
1. Overview
   TEMP / CPU / RAM / LOAD

2. Network
   active interface / IPv4 / RX / TX

3. Power
   Pi 5 PMIC / throttling flags

4. System
   USER / HOST / uptime / disk

5. Terminal
   local command editor + real PTY shell output
```

### Pi 5 电源相关

Pi 5 可通过 `vcgencmd` 获取部分 PMIC 信息。实际值依系统/固件可用性决定；桌面模拟器里 Pi-only 数据会显示 `N/A`。

## 2. 先在桌面模拟完整 Pi 应用

这个 target 运行真实 Pi 应用页面、Linux status monitor 和 PTY Shell，只把物理 OLED 与 evdev 键盘换成 X11：

```bash
sudo apt install -y build-essential cmake libx11-dev
cmake -S . -B build -DEPUI_BUILD_RPI_SIM=ON
cmake --build build -j4
./build/epui_rpi_sim
```

`epui_sim` 和 `epui_rpi_sim` 的区别：

```text
epui_sim      -> 框架组件/功能 Gallery
epui_rpi_sim  -> 真正的 Raspberry Pi 5 五页应用，只是输出到 X11
```

## 3. Pi 5 实机编译

```bash
sudo apt install -y cmake g++ i2c-tools
cmake -S . -B build-pi -DEPUI_BUILD_RPI=ON
cmake --build build-pi -j4
```

运行：

```bash
./build-pi/epui_rpi \
  /dev/i2c-1 \
  0x3c \
  ssd1306 \
  /dev/input/event0 \
  /bin/bash
```

SH1106：

```bash
./build-pi/epui_rpi /dev/i2c-1 0x3c sh1106 /dev/input/event0 /bin/bash
```

部署时不建议写死 `event0`，优先使用：

```text
/dev/input/by-id/...-event-kbd
```

## 4. 输入

普通页面：

```text
Left / Right -> 切 Page
```

Terminal 页面：

```text
Enter        -> 聚焦终端
输入字符      -> 编辑命令
Left/Right   -> 移动命令光标
Up/Down      -> 命令历史
Ctrl+Up/Down -> 输出历史滚动
Enter        -> 执行
Ctrl-C       -> 中断前台程序
Esc          -> 退出终端焦点，恢复 Page 导航
```

## 5. PTY Shell

终端页运行真实 PTY，而不是把 shell 输出假装画上去。框架核心只负责：

- `TerminalView`；
- 行编辑；
- history；
- 键语义。

Linux adapter 负责：

- 创建 PTY；
- 读写 shell；
- evdev；
- 系统状态读取。

这保证 `TerminalView` 仍然可移植。

## 6. 性能策略

当前 Pi 应用采用：

- 实机目标约 30 FPS；
- X11 模拟器约 60 FPS；
- 相同 framebuffer 不重复 I2C 刷新；
- cursor blink / 状态变化仍正常触发刷新；
- 状态监控优先采样当前可见稳定页面；
- Page transition 时避免不必要的高频系统采样。

## 7. systemd 部署

仓库提供 `deploy/` 模板。典型流程：

```bash
sudo useradd --system --create-home --shell /bin/bash epui
sudo usermod -aG i2c,input epui

sudo install -m 0755 build-pi/epui_rpi /usr/local/bin/epui_rpi
sudo install -m 0644 apps/rpi5_oled_console/deploy/epui-rpi5.service /etc/systemd/system/
sudo install -m 0644 apps/rpi5_oled_console/deploy/epui-rpi5.default /etc/default/epui-rpi5

sudo editor /etc/default/epui-rpi5
sudo systemctl daemon-reload
sudo systemctl enable --now epui-rpi5.service
```

排查：

```bash
systemctl status epui-rpi5.service
journalctl -u epui-rpi5.service -f
```

服务账户需要 `i2c` 和 `input` 访问权限。

## 8. 硬件建议

- 128×64 I2C OLED，SSD1306 或 SH1106；
- 地址常见 `0x3c`；
- 键盘可用 USB，有条件可换成 GPIO 三键/编码器插件；
- 不建议把 Pi 5 电源键当通用 UI 键；
- 如用 GPIO 输入，单独接 momentary buttons/encoder。

---

## English

`apps/rpi5_oled_console` is a complete product-style example that turns a Raspberry Pi 5, a 128×64 I2C OLED, and a keyboard into a headless system dashboard and interactive shell.

### Pages

1. Overview — temperature, CPU, RAM, load.
2. Network — active interface, local IPv4, RX/TX rate.
3. Power — Pi 5 PMIC/throttling information when available.
4. System — user, host, uptime, disk.
5. Terminal — local line editor plus a real PTY shell.

### Desktop simulation of the real Pi application

```bash
cmake -S . -B build -DEPUI_BUILD_RPI_SIM=ON
cmake --build build -j4
./build/epui_rpi_sim
```

`epui_sim` is the framework component gallery; `epui_rpi_sim` runs the actual Pi application while replacing only the physical OLED/evdev input with X11.

### Hardware build

```bash
cmake -S . -B build-pi -DEPUI_BUILD_RPI=ON
cmake --build build-pi -j4
./build-pi/epui_rpi /dev/i2c-1 0x3c ssd1306 /dev/input/event0 /bin/bash
```

Use `sh1106` for that controller and prefer a stable `/dev/input/by-id/...-event-kbd` path in production.

### Terminal controls

Enter focuses/executes, Left/Right moves the local command cursor, Up/Down selects command history, Ctrl+Up/Down scrolls output, Ctrl-C interrupts the foreground process, and Escape returns to page navigation.

### Runtime strategy

The Pi hardware loop targets roughly 30 FPS, avoids re-sending identical framebuffers over I2C, and limits expensive status sampling to useful periods. The X11 version targets roughly 60 FPS for animation development.

### systemd

Deployment templates live under `apps/rpi5_oled_console/deploy/`. The service account needs permission to access both I2C and input devices.
