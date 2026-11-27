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
- **Settings** has language (EN/TR), minimum altitude, beep, and supply (Auto, 12, 15, 20 or 24 V,
  default Auto with the supply sense, else 24 V; 15 V suits USB-PD power banks). Supply sets the TMC2209 starting values and caps slew
  speed at 70 % on 12 V and 87.5 % on 15 V (full speed from 20 V). Language, minimum altitude, beep,
  supply and Own WiFi are kept across power cycles; date and time are not (no clock battery), they come
  from the phone or the LCD.
- **Supply Auto** (with the supply-sense divider on J8, `SUPPLY_SENSE 1`, see `docs/wiring.md`): the
  row shows the applied choice and the measured voltage (`Auto 15 15.1`, `Auto 15 --` without a reading;
  TR `Oto`) and picks 12, 15, 20 or 24 V by itself once a reading inside a window (12: 10.5-14.2, 15: 14.2-16.5,
  20: 18.5-21.5, 24: 22.5-25.5 V) holds for 2 s, only while the mount is not moving (tracking is fine; any move
  holds it back), so a USB-PD trigger giving 15 V from a power bank and 20 V from the wall adapter needs no
  change. Readings between the windows change nothing, and leaving 12 V for 15 V needs 14.5 V (0.3 V hysteresis), so
  a 12 V battery on charge stays on 12 V. Vdd is assumed to be 3.3 V. A click leaves Auto for the applied voltage (nothing
  changes), then steps through the other values and back to Auto; manual values ignore the sense input and show the
  measurement in brackets. Auto is the default with `SUPPLY_SENSE 1` on a blank store; a board upgraded
  from an older version keeps its stored manual voltage until Auto is picked once. To calibrate (the divider is sensitive,
  1 % on a resistor is about 0.4 V): with `SUPPLY_SENSE_CAL 1.0` measure the supply with a meter, ideally at about
  20 V (the wall adapter), read the shown value, set `SUPPLY_SENSE_CAL` = meter / shown (e.g. 1.02; a new value =
  old x meter / shown) and rebuild and flash. R_top goes to the supply + after the fuse; check R20's tolerance
  (`docs/wiring.md`).
- **Settings > Network** shows what the ESP32 bridge reports: mode (Hotspot, Own WiFi, Connecting,
  Off, or No bridge when it has been silent for 5 s), WiFi name, IP address, clients and, in Hotspot
  mode, the signal. Click to change **Own WiFi**: Auto (own access point when no phone hotspot shows
  up within about 20 s), On (always its own access point) or Off (phone hotspot only).

## Robin to ESP32 cable: not USB

The Robin and the ESP32-S3 CAM module are linked by an ordinary USB-C cable, but it carries the board's
serial port and **15 V on VBUS** (the camera module makes its own 5.1 V from it), **not USB** (pins in
[docs/wiring.md](docs/wiring.md)).

- **Never plug it into a computer, phone, charger or power bank.** 15 V on a USB port's VBUS can destroy
  that device, and it puts USB signals on the board's serial pins.
- **Never plug the cable into the ESP32 board's own USB-C port**, and unplug the link cable before
  connecting that port to a computer to flash it: 15 V on the ESP32's USB-C port would destroy the board (and
  the computer behind it).
- **Label both ends "15 V – NOT USB"**, on the Robin side and on the module.
- A safer alternative is a **non-USB connector** for this link, for example a GX12 4-pin (aviation)
  connector or a JST-SM, which nothing else can be plugged into by mistake. Then the label is a second line
  of defence, not the only one.
- The 220 Ω resistors on the data lines are unchanged; they limit the damage on the data lines from a
  mistaken plug, they do not make 15 V on VBUS safe.
- **Power the Robin off** before connecting or disconnecting the cable.

## Motor noise (TMC2209)

RA, DEC, AZ and ALT run stealthChop at idle, tracking and guiding, and spreadCycle only when moving fast. If a motor still hisses,
set these as `#define`s in your local config: `TMC_PWM_FREQ` 3 (58 kHz) or 1 (35 kHz), `RA_HOLD_PERCENT` / `DEC_HOLD_PERCENT`
lower (30) if the hiss is at standstill, `TMC_TOFF` 4 or 5, or `TMC_TPWMTHRS` raised (3000) to stay in stealthChop for faster
moves. More hold means more torque and heat, less hold means quieter but a softer stop.

## License

MIT, see [firmware/LICENSE](firmware/LICENSE). OAE mechanical design is CC BY-NC by
OpenAstroTech; nothing of it is redistributed here.
