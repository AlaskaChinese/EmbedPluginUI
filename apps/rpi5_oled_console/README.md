# Raspberry Pi 5 OLED console

This application turns a 128x64 I2C OLED and a USB keyboard into a small
headless status dashboard and interactive shell for Raspberry Pi 5.

Pages show temperature, CPU/RAM/load, network interface, IPv4 address,
throughput, uptime, disk usage and Pi 5 power/throttling status. The Terminal
page keeps a compact `>` command editor at the top and renders command output
below it. The shell prompt and terminal echo are hidden so the output area is
reserved for results.

## Run the complete application in the desktop simulator

This target uses the real application pages, Linux status monitor and PTY
shell. Only the physical OLED and evdev keyboard are replaced by an X11 window
and desktop keyboard.

```bash
sudo apt install build-essential cmake libx11-dev
cmake -S . -B build -DEPUI_BUILD_RPI_SIM=ON
cmake --build build -j4
./build/epui_rpi_sim
```

The five pages appear in the same order as the Raspberry Pi build:

1. Overview: temperature, CPU, RAM and load.
2. Network: active interface, local IPv4 address and RX/TX rate.
3. Power: Pi 5 PMIC values and throttling flags.
4. System: user, hostname, uptime and disk usage.
5. Terminal: a real PTY running `$SHELL` (or `/bin/sh`).

Use Left/Right to change pages. On Terminal, press Enter to focus, type a
command such as `uname -a`, and press Enter again to execute it. While focused,
Left/Right moves the local input cursor, Ctrl+Up moves one output line toward
older history, Ctrl+Down moves one line toward the latest output, Ctrl-C interrupts the
foreground command, and Esc returns to page navigation. Close the window to
stop the simulator and its shell.

Up/Down selects from the 16-entry in-memory command history and restores an
unfinished draft when returning to the newest position.

The desktop simulator targets about 60 FPS with deadline-based frame pacing,
and its X11 renderer batches adjacent lit pixels. The status monitor samples
only the currently visible stable page and pauses sampling during page
transitions. On Raspberry Pi, the loop targets 30 FPS and identical frames are
not written to I2C repeatedly; cursor blinking and changing status data still
refresh normally.

The simulator samples the development computer, so network/IP/load values are
real desktop values. Raspberry Pi-only PMIC readings, and sometimes the Pi
thermal path, display `N/A`; they require actual Pi 5 hardware.

## Build and run

```bash
sudo apt install cmake g++ i2c-tools
cmake -S . -B build-pi -DEPUI_BUILD_RPI=ON
cmake --build build-pi -j4
./build-pi/epui_rpi /dev/i2c-1 0x3c ssd1306 /dev/input/event0 /bin/bash
```

Use `sh1106` for an SH1106 panel. Prefer a stable keyboard path from
`/dev/input/by-id/` instead of `event0` when configuring systemd.

The service account needs access to the `i2c` and `input` groups. See the
files under `deploy/` for an installation template.

## Install as a service

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

Check startup and hardware permissions with:

```bash
systemctl status epui-rpi5.service
journalctl -u epui-rpi5.service -f
```
