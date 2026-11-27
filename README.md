# OAE Robin E3

> **Unofficial.** A personal project, not affiliated with or supported by OpenAstroTech. The
> firmware in `firmware/` is a fork of
> [OpenAstroTracker-Firmware](https://github.com/OpenAstroTech/OpenAstroTracker-Firmware) v1.14.0
> (MIT, © OpenAstroTech contributors). For the official firmware and support, go there.

Firmware and a phone app that run an **OpenAstroExplorer (OAE)** mount from an **MKS Robin E3
v1.1** board (STM32F103RC, onboard TMC2209). An **ESP32-S3 CAM** on the telescope bridges the board
to the phone over WiFi: it joins the phone's own hotspot, or starts its own access point when there
is none. The app works with no internet: GoTo from an offline catalog, planets computed on the
phone, manual jog, guided startup with microswitch homing, and every motor and gear setting editable and saved on
the board. With internet it can also track planes from free ADS-B data.

Design and roadmap: [docs/architecture.md](docs/architecture.md). Wiring:
[docs/wiring.md](docs/wiring.md).

## Layout

| Folder | What |
|---|---|
| `firmware/` | OAT firmware fork, STM32 port (PlatformIO, Arduino/STM32duino) |
| `bridge/` | ESP32-S3 CAM WiFi bridge: phone hotspot or own access point (PlatformIO, Arduino) |
| `app/` | Phone PWA (English / Turkish) |
| `hardware/` | Printed parts: endstop brackets and cams, camera module with eyepiece rings, focus motor mount |
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
  `click: STOP`. Turning the knob does nothing during motion, except on the Adjust screen.
- **Point:** RA, then Dec, then a confirm screen; turn to pick GO or SYNC, click to run it.
- **Adjust:** turn to move the selected axis (RA, DEC, and AZ, ALT or Focus when fitted) by one step
  size per detent; click picks the next axis; hold opens the options: step size (1 to 10 000
  steps, or a continuous jog at SLOW / CENTER / FIND / MAX for RA and DEC), Sync on target, Set home
  here and Go home. The screen shows the axis position and what a step is in degrees. Use it to
  line the mount up with the OAE markers, then run **Set home here** (only while idle; it works
  before date and time are set). Sync on target uses the last Point coordinates.
- **Point and Track here stay locked** until both date and time are set since boot. The status
  screen names what is missing; click the warning to go straight to Date, then Time.
- **GoTo refuses** targets below the minimum altitude or within 15° of the Sun. The refused screen
  returns to the editor with the values kept.
- **Settings** has language (EN/TR), minimum altitude, beep, and supply (12, 15, 20 or 24 V,
  default 24 V; 15 V suits USB-PD power banks). Supply sets the TMC2209 starting values and caps slew
  speed at 70 % on 12 V and 87.5 % on 15 V (full speed from 20 V). Language, minimum altitude, beep,
  supply and Own WiFi are kept across power cycles; date and time are not (no clock battery), they come
  from the phone or the LCD.
- **Settings > Network** shows what the ESP32 bridge reports: mode (Hotspot, Own WiFi, Connecting,
  Off, or No bridge when it has been silent for 5 s), WiFi name, IP address, clients and, in Hotspot
  mode, the signal. Click to change **Own WiFi**: Auto (own access point when no phone hotspot shows
  up within about 20 s), On (always its own access point) or Off (phone hotspot only).

## Robin to ESP32 cable: not USB

The Robin and the ESP32-S3 CAM are linked by an ordinary USB-C cable, but it carries the board's
serial port and 5 V, **not USB** (pins in [docs/wiring.md](docs/wiring.md)).

- **Never plug it into a computer, phone, charger or power bank.** That ties their 5 V to the
  Robin's 5 V rail and puts USB signals on the board's serial pins.
- **Never plug the cable into the ESP32 board's own USB-C port**, and unplug the link cable before
  connecting that port to a computer to flash it: the cable ties the Robin's 5 V to the ESP32's 5V pin,
  which on most boards is also that port's VBUS.
- **Label both sockets** (for example "OAE LINK, NOT USB"), on the Robin side and on the ESP32 side.
- The 220 Ω resistors on the data lines limit the damage from a mistaken plug; they do not make it
  safe.
- **Power the Robin off** before connecting or disconnecting the cable.

## Motor noise (TMC2209)

RA, DEC, AZ and ALT run stealthChop at idle, tracking and guiding, and spreadCycle only when moving fast. If a motor still hisses,
set these as `#define`s in your local config: `TMC_PWM_FREQ` 3 (58 kHz) or 1 (35 kHz), `RA_HOLD_PERCENT` / `DEC_HOLD_PERCENT`
lower (30) if the hiss is at standstill, `TMC_TOFF` 4 or 5, or `TMC_TPWMTHRS` raised (3000) to stay in stealthChop for faster
moves. More hold means more torque and heat, less hold means quieter but a softer stop.

## License

MIT, see [firmware/LICENSE](firmware/LICENSE). OAE mechanical design is CC BY-NC by
OpenAstroTech; nothing of it is redistributed here.
