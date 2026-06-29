# FluidMonitor

FluidMonitor is a simple FluidNC DRO monitor and controller that runs on CYD-style ESP32 displays. It uses the [FluidNC-EspNow-Client](https://github.com/figamore/FluidNC-EspNow-Client) library for paired ESP-NOW transport and FluidNC status parsing, and provides status, jogging, and machine action controls.

## Build

```sh
pio run -e esp32-cyd-capacitive
```

## Pairing

Open an ESP-NOW pairing window in FluidNC, then open the **Settings** tab in the
app and tap **Pair**. Once paired, the app stores the machine profile and
reconnects automatically.

## License

FluidMonitor is licensed under the GNU General Public License v3.0 only. See
[`LICENSE`](LICENSE).
