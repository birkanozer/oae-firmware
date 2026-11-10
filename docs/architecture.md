# Architecture — OAE on MKS Robin E3 (unofficial)

The vision, architecture and roadmap. Read before non-trivial work. Settled decisions are in
[CLAUDE.md](../CLAUDE.md) under `## Decisions`; wiring in [wiring.md](wiring.md).

## 1. Goal

Drive an **OpenAstroExplorer (OAE)** mount from an **MKS Robin E3 v1.1** board and control it
entirely from a phone over **Bluetooth**, with no internet required:

- manual jog and automatic GoTo to celestial objects, respecting the mount's mechanical limits;
- a guided startup sequence (phone time + GPS → home on microswitches → ready to track);
- every motor / gear / driver parameter editable from the phone and persisted on the board;
- optional plane tracking from free ADS-B data when the phone has internet;
- optional Stellarium Mobile PLUS / desktop control.

Phones and browsers are fixed: **only stock Safari, Chrome and Firefox** on the phone — no extra
browser, no native app. **Android** links over **Bluetooth** (Chrome; Firefox has no Web
Bluetooth). **iPhone** has no Web Bluetooth in any browser (all are WebKit), so it links over
**the iPhone's own Personal Hotspot**: the ESP32 joins it as a WiFi client; the phone keeps its
cellular internet and never joins a device network.

Non-goals (for now): ASCOM/INDI drivers of our own (OAT's LX200 over USB already works with
them), plate solving, autoguiding, a native iOS/Android app.

## 2. System overview

```
 Phone (PWA)                     ESP32-S3-DevKitC-1                 MKS Robin E3 v1.1 (STM32F103RC)
 ─────────────                   ──────────────────                 ───────────────────────────────
 Android: Chrome  ── BLE ──────► BLE GATT "UART" service  ─UART─►   OAT firmware port (LX200 parser,
  (app from GitHub Pages)         ↕ byte pipe              USART2     mount model, limits, homing)
 iPhone: Safari ── hotspot ────► WiFi STA on the iPhone's           ├ TMC2209 X  → RA   (NEMA17 0.9°)
  (app served by the ESP32        hotspot: HTTPS app +              ├ TMC2209 Y  → DEC
   over HTTPS, WSS link)          WSS byte pipe; LX200 TCP          ├ TMC2209 Z  → free (ALT / AZ bipolar)
 Phone does: catalog,             :4030 for Stellarium              ├ TMC2209 E  → free (focus bipolar)
 ephemerides, RA/Dec↔Az/Alt,                                        │
 GPS + time, plane fetch,                                           ├ ULN2003 A (EXP1) → AZ 28BYJ-48
 settings UI (EN/TR)                                                ├ ULN2003 B (EXP1) → focus 28BYJ-48
                                                                    ├ endstops X-/Y-/Z-/Z+ (microswitch)
                                                                    ├ Ender 3 V2 screen (DWIN, USART3
                                                                    │   PB10/PB11 + knob on EXP2)
                                                                    ├ AT24C32 I²C EEPROM (settings)
                                                                    └ USB (CH340, USART1) → LX200 / debug
```

**Authority split.** The STM32 firmware owns motion, limits, homing and persistent settings — it
is safe on its own (a dropped BLE link never leaves a slew running unchecked). The phone owns
astronomy math, catalogs, plane data and UI. The ESP32 is a dumb, transparent bridge so the
firmware protocol stays the single source of truth and USB clients (ASCOM, Stellarium desktop,
N.I.N.A.) keep working unchanged.

## 3. Firmware (`firmware/`)

Fork of OpenAstroTracker-Firmware **v1.14.0** (MIT), imported at commit `6a40302`. Arduino
framework on PlatformIO (`ststm32` platform, STM32duino core). **No RTOS**: OAT is a main loop
plus a stepper timer interrupt; FreeRTOS would cost RAM (48 KB) for no gain.

Port work:
- new board `BOARD_STM32_MKS_ROBIN_E3` + `pins_MKS_ROBIN_E3.hpp` + `[env:robin_e3]`;
- stepper interrupt backend on `HardwareTimer` (modelled on the RP2040 port, PR #298);
- TMC2209 UART on four **separate single-wire** PDN pins (PC7, PD2, PC12, PC11) via half-duplex
  software serial — the riskiest piece, spiked first;
- settings on the onboard **AT24C32** (I²C PB6/PB7) — verify presence by I²C scan on the
  `_0003` board before relying on it; fall back to flash emulation;
- LX200 on USB (USART1, PA9/PA10) **and** on USART2 (PA2 TX / PA3 RX, the NeoPixel and
  BLTouch-servo headers) for the ESP32 bridge;
- flashed at `0x08000000` over ST-Link (the factory image is backed up in `backup/`).

Upgrade work (later slices):
- **Runtime configuration.** Today most motor/gear values are compile-time `#define`s. Move them
  into a versioned settings struct in EEPROM, defaults = stock OAE: per axis — driver slot
  (TMC X/Y/Z/E, ULN2003 A/B, none), motor steps/rev, gear chain (pulley teeth, gearbox ratio,
  worm ratio, belt pitch / wheel circumference, lead-screw pitch), microstepping (slew / track),
  run current, hold %, direction invert, max speed, acceleration, backlash; plus limits, home
  offsets, site, tracking calibration. Exposed through new `:XC…#` get/set commands. Changing a
  driver slot takes effect after reboot.
- **ULN2003 driver type** (AccelStepper `HALF4WIRE`) alongside TMC2209, selectable per axis.
- **Microswitch homing** for RA, DEC (and AZ). OAT's hall-sensor routine finds both edges of a
  sensor zone and homes to the midpoint — a microswitch pressed by a printed cam of known width
  gives the same two edges, so the routine is reused, with debounce tuned for contacts.

## 3a. Onboard screen (Ender 3 V2 DWIN)

Lets the mount be set up and run with no phone, and is the first UI to exist (before the
bridge). Creality Ender 3 V2 display: DWIN T5UIC1 serial LCD, 480×272, rotary knob + click,
beeper. Frames `AA <cmd> <data> CC 33 C3 3C`, 115200 baud, RGB565, drawing commands (clear,
text, rectangle, stock icon library, backlight). The driver is written **from the DWIN T5UIC1
developer guide only** — Marlin's and other community DWIN code is GPL and is not copied.

A **new graphical UI** for the knob, not OAT's 16×2 menus (OAT's `DISPLAY_TYPE` stays NONE; the
screen is its own module calling the mount API):

- **Status** (home screen): RA/Dec, Alt/Az, tracking on/off, LST, time, site, link state.
- **Setup wizard** (first boot and from the menu): date/time, UTC offset, latitude/longitude
  (knob entry; later prefilled when a phone sends them), home the axes.
- **Jog**: pick axis, knob turns move it, click cycles speed; "Stop" always one click away.
- **Point & track**: *Track here* — start sidereal tracking wherever the mount points (manual
  aim, no coordinates needed); *GoTo RA/Dec* — enter coordinates, slew, then track; *Sync here*
  — tell the mount the entered RA/Dec is where it points.
- **Settings** (subset of §3 runtime config once it exists): motor currents, limits, Sun
  avoidance off/on, screen brightness.

The screen and the phone can be connected at once; the firmware stays the single authority, and
the screen redraws from mount state, so either side's command shows up on both.

## 4. Mechanical limits and safety

- Software limits from OAT: RA ±1 h around the 6 h home (`RA_LIMIT_LEFT/RIGHT`), RA tracking
  limit 6.75 h, DEC ±135° from home — all editable and persisted.
- The home microswitches double as **sanity checks**: hitting one outside a homing run stops
  the slew.
- The phone checks every target before sending it: below the horizon (with a settable minimum
  altitude), outside RA/DEC reach, or **within N° of the Sun (default 15°)** → refused, with
  "reachable at HH:MM" when it rises into reach.
- The firmware rejects GoTo until time, site and home are set (no slewing from a random state).

### Endstop placement (four inputs: X- PA12, Y- PA11, Z- PC6, Z+ PB1)

| Input | Use | Placement |
|---|---|---|
| X- | RA home | microswitch on the RA base, pressed by a printed cam on the RA ring at the OAE home marker (counterweight-down / meridian). Cam spans a few degrees so both edges are found. |
| Y- | DEC home | microswitch on the DEC body, cam at DEC home (pointing at the pole). |
| Z- | AZ home / centre | microswitch at the AZ centre position, cam on the moving AZ plate. |
| Z+ | spare | ALT home, or a hard RA end-of-travel limit. |

Exact bracket geometry is a hardware task once the switches are in hand
([wiring.md](wiring.md)).

## 5. Startup sequence

1. Power on → firmware boots, motors disabled, state `UNREADY`.
2. Phone connects over BLE → sends UTC time, date, UTC offset, latitude, longitude, altitude
   (from phone GPS + clock).
3. User confirms the mount is roughly level and polar-aligned (OAE ALT/AZ adjust) → **Home**:
   RA then DEC seek their microswitches (bounded search, abort on timeout).
4. Optional AZ home; optional polar-alignment helper.
5. State `READY` → tracking on, GoTo enabled.

## 6. ESP32 bridge (`bridge/`)

ESP32-S3-DevKitC-1 (BLE 5, no Bluetooth Classic). Arduino on PlatformIO (FreeRTOS underneath).
Powered from the board's 5 V (EXP1 pin 10); UART to the STM32 at 3.3 V. Two front-ends feed one
byte pipe to the firmware (one client at a time owns it; others are refused):

- **BLE** (Android): GATT service with RX (write) and TX (notify) characteristics,
  Nordic-UART-style UUIDs.
- **WiFi station on the iPhone's Personal Hotspot** (SSID/password set from the app over USB or
  BLE once, stored in NVS). Serves the PWA from LittleFS over **HTTPS** and the byte pipe over
  **WSS** on the same origin. HTTPS is required for GPS and the service worker; it uses a
  project CA generated once, installed on the iPhone as a configuration profile, and a
  device certificate for the hotspot address (`172.20.10.x`) / `oae.local`. Same Wi-Fi link
  also exposes LX200 TCP :4030 for Stellarium Mobile / desktop.
- Never runs an access point that would take over the phone's WiFi.

Open risks (spiked in slice 4): TLS server RAM on the S3 (has 8 MB PSRAM on N8R8 — check the
module variant), whether the iPhone hotspot stays up with no cellular signal in the field, and
CORS on the ADS-B APIs from the ESP32 origin.

## 7. Phone app (`app/`)

One PWA codebase, two HTTPS origins (HTTPS is needed for GPS and service workers), cached for
offline use. No framework build step unless it earns its place.

- **Transport** behind one interface (`send(bytes)`, `onData`):
  - Android / Chrome: Web Bluetooth; app installed from GitHub Pages.
  - iPhone / Safari: WSS to the ESP32; app installed from the ESP32's HTTPS origin.
- **Setup:** GPS + time from the phone → firmware; homing; status.
- **Manual:** jog pad with speed steps per axis, stop button always visible.
- **GoTo:** offline catalog (bright stars, Messier, bright NGC/IC, named objects) + Sun, Moon
  and planets computed on the phone (`astronomy-engine`); search, "visible now" filter,
  reachability marked per object.
- **Planes:** with internet, adsb.lol / airplanes.live within a radius → list, select → poll
  every few seconds, dead-reckon between fixes, stream target updates to the mount.
- **Settings:** every firmware setting (§3) with stock defaults and reset; site; Sun avoidance;
  language **English / Turkish**.

## 8. Roadmap (slices)

Each slice is its own feature branch, plan, and release.

| # | Slice | Testable when |
|---|---|---|
| 0 | Scaffold: repo, docs, OAT import, toolchain, native test gate | now |
| 1 | `stm32-port`: board env, pins, timer ISR, TMC UART, EEPROM, LX200 on USB + USART2 | board on a 12/24 V PSU |
| 2 | `dwin-ui`: Ender 3 V2 screen driver + status, setup wizard, jog, point & track | screen wired (adapter cable) |
| 3 | `runtime-config`: settings struct + `:XC` commands, ULN2003 driver type, screen settings page | slice 1 |
| 4 | `homing`: microswitch homing, startup sequence (screen + LX200), end-stop sanity stops | switches mounted |
| 5 | `esp32-bridge`: BLE ↔ UART, hotspot STA + HTTPS/WSS | ESP32 wired |
| 6 | `pwa`: connect (BLE + WSS), setup, jog, catalog GoTo, settings, EN/TR, offline | slice 5 |
| 7 | `planes`: ADS-B tracking | slice 6 |
| 8 | `stellarium`: LX200 TCP :4030 on the hotspot link | slice 5 |
