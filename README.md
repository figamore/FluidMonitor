# FluidMonitor

FluidMonitor is a LovyanGFX + LVGL DRO monitor app for CYD-style ESP32 displays.
It uses the [FluidNC-EspNow-Client](https://github.com/figamore/FluidNC-EspNow-Client) library for
paired ESP-NOW transport and FluidNC status parsing, and provides status, jogging,
and machine action controls.

## Build

```sh
pio run -e esp32-cyd-capacitive
```

## Project Layout

- `src/main.cpp` - app startup, loop timing, and top-level lifecycle.
- `src/Display.*` - LovyanGFX, LVGL display driver, touch input, and backlight.
- `src/FluidLink.*` - FluidNC-EspNow-Client setup, connection callbacks, and status handling.
- `src/ui/` - LVGL views for Status, Jog, Actions, and Settings.

## Pairing

Open a pairing window on the ESP-NOW server, then tap **Settings** in the app and
start pairing. Once paired, the app stores the machine profile in NVS and
reconnects automatically.

## License

FluidMonitor is licensed under the GNU General Public License v3.0 only. See
[`LICENSE`](LICENSE).
