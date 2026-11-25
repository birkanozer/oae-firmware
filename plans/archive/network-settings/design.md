# Network page, persistent settings, USB-C serial cable (design)

Date: 2026-11-25. Approved in chat: Network sub-page with Own WiFi Auto/On/Off, no-cellular fallback,
Bluetooth dropped, one ESP32 (the ESP32-S3 CAM does camera and bridge), Robin-ESP32 link over a
standard USB-C cable carrying serial (option A), safety warnings in README, Robin settings kept across
power cycles, a click while moving still stops. Screen-off: skipped.

## T1 Persistent LCD settings

- Every LCD Settings value survives a power cycle: language, minimum altitude, beep, supply voltage,
  Own WiFi mode (T2). Stored through the existing `EEPROMStore` (AT24C32, flash fallback), at new
  addresses after the last used one, behind their own marker/version byte so a blank or old store
  loads the compile-time defaults. Write on change only (the store already writes only changed bytes).
- Loaded at boot before the first UI frame; supply voltage is applied with
  `Mount::applySupplyVoltage` at boot (stored value wins over `SUPPLY_VOLTAGE`; an invalid stored
  value falls back to `SUPPLY_VOLTAGE`).
- Latitude/longitude are already persisted by upstream (`LATITUDE_ADDR`, `LONGITUDE_ADDR`): check they
  are, and that LX200 `:St`/`:Sg` and the LCD Position editor both save them. Date/time are not
  persisted (no RTC): they come from the phone (`:SC`, `:SL`, `:SG`) or the LCD.
- Remove the BACKLOG item "Persist screen settings ... RAM-only until then".

## T2 Network sub-page and bridge status protocol

Settings gets a **Network** row that opens a sub-page (long press back, like the others):

| Line | Kind |
|---|---|
| Mode: Hotspot / Own WiFi / Connecting / Off / No bridge | read-only, what the ESP32 reports |
| Name: <SSID> (up to 20 chars shown) | read-only |
| IP: <a.b.c.d> | read-only |
| Clients: <n>  Signal: <dBm> (signal only in Hotspot mode) | read-only |
| Own WiFi: Auto / On / Off | setting (persisted, T1). Auto = own access point if no hotspot within ~20 s; On = always own access point; Off = hotspot only |

LX200 extension, ESP32 -> Robin (the Robin stays a plain command server; the ESP32 polls about once
a second). Pick a command letter not used upstream (grep `MeadeCommandProcessor.cpp` and
`core/meade/`; `:XN` is the proposal). Every command fits the 31-character LX200 limit:

- `:XNM<m>,<clients>,<rssi>#` status: m 0 Off, 1 Hotspot, 2 Own WiFi, 3 Connecting; clients 0..9;
  rssi negative dBm or 0. Reply `1` (or `0` on a malformed value).
- `:XNI<ip>#` dotted IPv4, reply `1`/`0`.
- `:XNN<ssid>#` SSID truncated by the ESP32 to 20 characters, printable ASCII only (anything else
  rejected or replaced with `?`), reply `1`.
- `:XNG#` returns the Own WiFi choice: `0#` Auto, `1#` On, `2#` Off.
- The page shows **No bridge** when no `:XNM` arrived for 5 s.
- Values are untrusted input (bridge is a trust boundary): bounds-checked, fixed buffers, no
  overflow. Host-test the parser (core/meade has native tests).

The ESP32 side is out of scope (bridge slice); only the Robin side and the documented protocol.

## T3 Docs

- docs/wiring.md: Robin <-> ESP32 cable = a standard USB-C (USB 2.0) cable between two USB-C breakout
  boards that join both contact rows (plug works either way round). VBUS = Robin 5 V, GND, D+ = Robin
  TX PA2 (NeoPixel header) -> ESP32 RX, D- = Robin RX PA3 (J24) <- ESP32 TX, 220 ohm series resistor on
  each data line at the Robin end. ESP32 end: two free GPIOs, never the board's own USB-C port. The
  ESP32 GPIOs are chosen per board in the bridge slice: on Freenove-style ESP32-S3 CAM boards GPIO 17
  and 18 are camera data lines, so the old "GPIO 17/18" note must go. The Robin's 5 V (MP1584, 3 A)
  powers the ESP32-S3 CAM (~0.5 A).
- README.md: a safety section for that cable: it is not USB; never plug it into a computer, phone,
  charger or power bank; label both sockets; the resistors limit damage from a mistaken plug; Robin
  powered off while connecting.
- CLAUDE.md Decisions: new dated lines superseding (1) Android over Web Bluetooth -> both phones over
  WiFi, Bluetooth dropped; (2) "No ESP32 access point" -> phone hotspot first, ESP32 own access point
  as fallback (Auto) or forced (On) from the LCD; (3) one ESP32-S3 CAM on the telescope does camera
  and bridge, linked to the Robin by the USB-C serial cable. Keep the old lines (history) but mark them
  superseded, or replace them, whichever the file's style allows; keep CLAUDE.md short.
- docs/architecture.md: drop BLE paths, add the hotspot/own-AP flow, the `:XN` commands, the cable.
- CHANGELOG [Unreleased], BACKLOG (remove done items, keep the camera spike).

## Out of scope

Screen off; bridge/ESP32 firmware; camera; ALT auto-levelling (later: accelerometer on the polar
axis, phone supplies time and location).
