# Raspberry Pi 5 port

Five pages: Overview, Network, Power, System and Terminal. The terminal page consumes lines written to `/tmp/epui-terminal`.

```bash
sudo apt install cmake g++ i2c-tools
cmake -S . -B build-pi -DEPUI_BUILD_RPI=ON
cmake --build build-pi -j4
i2cdetect -y 1
./build-pi/epui_rpi /dev/i2c-1 0x3c ssd1306
```

Use `sh1106` as the last argument for SH1106 panels. Left/Right or `a`/`d` changes pages; `q` exits.

Send terminal output with:

```bash
ping -c 4 1.1.1.1 > /tmp/epui-terminal
```

Raspberry Pi 5 has a dedicated power button with shutdown/wake semantics, so EmbedPluginUI leaves it to the OS. Product enclosures should connect separate momentary GPIO buttons and translate them to `epui::Key::Prev`, `epui::Key::Next` and `epui::Key::Select`.

The Power page reads Pi 5 PMIC ADC values through `vcgencmd`: `EXT5V_V` is shown as the 5 V input voltage and `VDD_CORE_A` as CPU-core rail current. Core rail current is labelled separately from total USB-C input current.
