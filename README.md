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
  size per detent; click picks the next axis, and after the last axis comes an **Options** stop; hold
  goes back to the main menu. A click while a move runs stops it and stays on Adjust. The screen
  shows the axis position and what a step is in degrees. Click on the Options stop opens the options:
  **Axis** (starts on the axis you last moved; click picks another), **Step size** for that axis (RA
  and DEC 10 to 50 000 steps, AZ and ALT 100 to 300 000, Focus 1 to 10 000), Sync on target,
  **Set <axis> home** (that axis only), **Set home all** (every axis; AZ and ALT are kept across power
  cycles, Focus until the next boot) and Go home. Hold or Back on the options, and either home
  confirm, return to Adjust on the chosen axis. Use it to line the mount up with the OAE markers, then
  set home (only while idle; it works before date and time are set). Sync on target uses the last
  Point coordinates.
- **Point and Track here stay locked** until both date and time are set since boot. The status
  screen names what is missing; click the warning to go straight to Date, then Time.
- **GoTo refuses** targets below the minimum altitude or within 15° of the Sun. The refused screen
  returns to the editor with the values kept.
- **Settings** has language (EN/TR), minimum altitude, beep, and supply (Auto, 12, 15, 20 or 24 V,
  default Auto with the supply sense, else 24 V; 15 V suits USB-PD power banks). Supply sets the TMC2209 starting values and caps slew
  speed at 70 % on 12 V and 87.5 % on 15 V (full speed from 20 V). Language, minimum altitude, beep,
  supply and Own WiFi are kept across power cycles; date and time are not (no clock battery), they come
  from the phone or the LCD.
- **Supply Auto** picks 12, 15, 20 or 24 V by itself from the supply-sense plug on J8: see
  [Supply Auto: setup and calibration](#supply-auto-setup-and-calibration) below.
- **Settings > Network** shows what the ESP32 bridge reports: mode (Hotspot, Own WiFi, Connecting,
  Off, or No bridge when it has been silent for 5 s), WiFi name, IP address, clients and, in Hotspot
  mode, the signal. Click to change **Own WiFi**: Auto (own access point when no phone hotspot shows
  up within about 20 s), On (always its own access point) or Off (phone hotspot only).

## Supply Auto: setup and calibration

The Robin E3 cannot measure its input voltage on its own (its only dividers feed the 5 V regulator), so
a two-resistor plug on the unused bed-thermistor socket **J8 (TB, PA1)** lets the firmware read it
(`SUPPLY_SENSE 1`, on in the example config). With a USB-PD trigger such as the ZY12PDN, which gives
15 V from a power bank and 20 V from a wall adapter on the same setting, the firmware then follows the
source by itself.

**1. Make the plug** (power off while fitting it; pinout and on-board circuit in `docs/wiring.md`):

| Part | From | To |
|---|---|---|
| R_top 100 kΩ, 1 % | supply + (after the 3 A fuse) | J8 pin 1 |
| R_bot 10 kΩ, 1 % | J8 pin 1 | J8 pin 2 (GND) |

A plug in backwards or a missing, open or shorted divider never reads as a valid voltage.

**2. Turn Auto on:** Settings > Supply, click until the row shows `Auto`. A blank board starts in Auto;
a board upgraded from v0.5.0 or older keeps its manual voltage until Auto is picked once.

**3. Check it:**

- The row shows the applied voltage and the measurement: `Auto 15 15.1` (TR `Oto 15 15.1`), or
  `Auto 15 --` with no valid reading (no plug), which keeps the last voltage.
- Power bank: about 15. Wall adapter: about 20.
- It applies a value once the reading stays 2 s inside that value's window, and only while the mount is
  not moving (tracking is fine; it never switches in the same moment a GoTo, Home or Park starts):

  | Applies | Reading window |
  |---|---|
  | 12 V | 10.5 – 14.2 V |
  | 15 V | 14.2 – 16.5 V |
  | 20 V | 18.5 – 21.5 V |
  | 24 V | 22.5 – 25.5 V |

  Readings between the windows change nothing. Leaving 12 V for 15 V needs 14.5 V (0.3 V hysteresis),
  so a 12 V lead-acid or LiFePO4 battery on charge (13.2 – 14.4 V) stays on 12 V.
- A click leaves Auto for the applied voltage (nothing changes), then steps through the other values
  and back to Auto. Manual values ignore the sense input and show the measurement in brackets,
  e.g. `15 V (15.1)`.

**4. Calibrate once.** The divider is sensitive (1 % on a resistor is about 0.4 V) and the firmware
assumes the board's 3.3 V rail exactly, so an uncalibrated reading can be off by about a volt; 12 and
15 V are the pair most likely to be confused.

1. Run with `SUPPLY_SENSE_CAL 1.0` (the default, `Configuration_adv.hpp`; override it in the local config).
2. Measure the supply with a meter, ideally at about 20 V (the wall adapter), and note the voltage the
   Supply row shows at the same time.
3. Set `SUPPLY_SENSE_CAL` = meter ÷ shown (for example 20.1 ÷ 19.7 = 1.02). To refine a value later:
   new = old × meter ÷ shown.
4. Rebuild and flash, then check the shown value against the meter on both the power bank and the
   adapter.

If the shown value is still far off after calibrating, check the board's R20 pull-up (4.7 kΩ; its
tolerance enters the reading) and the resistor values in the plug.

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
