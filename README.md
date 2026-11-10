# OAE Robin E3

> **Unofficial.** A personal project, not affiliated with or supported by OpenAstroTech. The
> firmware in `firmware/` is a fork of
> [OpenAstroTracker-Firmware](https://github.com/OpenAstroTech/OpenAstroTracker-Firmware) v1.14.0
> (MIT, © OpenAstroTech contributors). For the official firmware and support, go there.

Firmware and a phone app that run an **OpenAstroExplorer (OAE)** mount from an **MKS Robin E3
v1.1** board (STM32F103RC, onboard TMC2209). An **ESP32-S3** bridges the board to the phone: over
Bluetooth on Android (Chrome), over the iPhone's own Personal Hotspot on iPhone (Safari). The app
works with no internet: GoTo from an offline catalog, planets computed on the phone, manual jog,
guided startup with microswitch homing, and every motor and gear setting editable and saved on
the board. With internet it can also track planes from free ADS-B data.

Design and roadmap: [docs/architecture.md](docs/architecture.md). Wiring:
[docs/wiring.md](docs/wiring.md).

## Layout

| Folder | What |
|---|---|
| `firmware/` | OAT firmware fork, STM32 port (PlatformIO, Arduino/STM32duino) |
| `bridge/` | ESP32-S3 BLE / hotspot bridge (PlatformIO, Arduino) |
| `app/` | Phone PWA (English / Turkish) |
| `hardware/` | Printed parts: endstop brackets and cams |
| `backup/` | Factory flash image of the board (git-ignored) |

## Setup (Windows)

Everything installs inside this folder.

```powershell
uv venv .venv --python 3.13
uv pip install --python .venv\Scripts\python.exe platformio
```

PlatformIO keeps its toolchains in `.pio-core/` (set in `firmware/platformio.ini`). `.tools/` holds
the portable MinGW GCC used by the native tests.

## Build and flash

```powershell
cd firmware
..\.venv\Scripts\pio.exe run -e robin_e3            # build
..\.venv\Scripts\pio.exe run -e robin_e3 -t upload  # flash over ST-Link (board on 12/24 V)
..\.venv\Scripts\pio.exe test -e native             # host unit tests
```

Full gate (native tests + `robin_e3` build), from the repo root: `bash tools/gate.sh`.

## License

MIT, see [firmware/LICENSE](firmware/LICENSE). OAE mechanical design is CC BY-NC by
OpenAstroTech; nothing of it is redistributed here.
