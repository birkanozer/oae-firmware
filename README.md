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

## Onboard screen

The stock Ender 3 LCD (128×64, knob with click, buzzer) runs the mount with no phone. It plugs
into the board's "Ender-3 EXP1" header; pins in [docs/wiring.md](docs/wiring.md).

- **Turn** moves the cursor, or changes the field in an editor. **Click** selects. **Hold** goes
  back. In an editor, hold goes to the previous field (on the first field it cancels).
- **Any click or hold stops motion.** While the mount slews, jogs, parks or homes, a click or hold
  on any screen issues STOP, which halts every motor including tracking. The footer shows
  `click: STOP`. Turning the knob does nothing during motion, except on the Jog screen.
- **Jog:** turn to move. The slew stops 0.3 s after the knob stops. Click toggles RA and DEC; hold
  returns to the submenu. Speed cycles SLOW / CENTER / FIND / MAX.
- **GoTo, Sync and Track here stay locked** until both date and time are set since boot. The status
  screen names what is missing; click the warning to go straight to Date, then Time.
- **GoTo refuses** targets below the minimum altitude or within 15° of the Sun. The refused screen
  returns to the editor with the values kept.
- **Settings** has language (EN/TR), minimum altitude, beep, and supply (12 V or 24 V, default
  24 V). Supply sets the TMC2209 starting values and caps slew speed at 70 % on 12 V. Screen
  settings reset at boot until settings storage lands (slice 3).

## License

MIT, see [firmware/LICENSE](firmware/LICENSE). OAE mechanical design is CC BY-NC by
OpenAstroTech; nothing of it is redistributed here.
