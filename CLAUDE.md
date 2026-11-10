# CLAUDE.md

Guidance for working in this repo. Keep it SHORT: rules, invariants and traps only. Design and
roadmap live in `docs/architecture.md`, history in `CHANGELOG.md` and git.

## What this is

Unofficial OAE mount controller: `firmware/` (OpenAstroTracker-Firmware v1.14.0 fork ported to
the MKS Robin E3 v1.1, STM32F103RC), `bridge/` (ESP32-S3: BLE for Android, iPhone hotspot
HTTPS/WSS), `app/` (PWA, EN/TR). Firmware owns motion, limits, homing and settings; the phone
owns astronomy math and UI; the bridge is a transparent byte pipe.

## Workflow

Feature work follows the global **`feature-workflow`** skill. Keys it reads:

- plans: plans/<slug>/
- plans-after-ship: archive
- ledger: plans/<slug>/progress.md
- gate: bash tools/gate.sh
- version-files: none
- release: github
- notes: `firmware/Version.h` is upstream OAT's version; leave it, our product version is the git tag.
- notes: Never flash without the board on its 12/24 V supply (ST-Link alone back-powers it at ~2 V).

## Commands

- Tools live in this folder: `.venv/` (PlatformIO), `.pio-core/` (toolchains, OpenOCD), `.tools/`
  (portable MinGW for native tests, put on PATH by `tools/gate.sh`). Call `.venv/Scripts/pio.exe`;
  nothing global.
- Firmware build/flash/test: see `README.md`. Flash goes to `0x08000000` over ST-Link (SWD).
- Read-only chip probe: `.pio-core/packages/tool-openocd/bin/openocd.exe -s <pkg>/openocd/scripts
  -f interface/stlink.cfg -c "transport select swd" -f target/stm32f1x.cfg -c "init; reset halt; flash probe 0; resume; shutdown"`.

## Rules and traps

- `firmware/CLAUDE.md` is upstream's guide to the OAT code base: read it before touching
  firmware. `firmware/CHANGELOG.md` is upstream's; ours is the root `CHANGELOG.md`.
- Upstream code style: clang-format v12 (`firmware/.clang-format`). Keep diffs to upstream files
  small and guarded (`#if BOARD == BOARD_STM32_MKS_ROBIN_E3`) so future upstream merges stay easy.
- `backup/robin_e3_factory_full.bin` is the only copy of the factory flash: never overwrite it.
- Board pins and headers: `docs/wiring.md` is the single source; update it with every pin change.
- Phone side: only stock Safari, Chrome and Firefox. No extra browsers, no native app.

## Decisions

- 2026-11-10 Board flashed at `0x08000000`; the ~31 KB factory image (likely the MKS bootloader)
  is overwritten, SD-card updates are lost. Backup kept.
- 2026-11-10 No RTOS on the STM32 (OAT is loop + timer ISR; 48 KB RAM).
- 2026-11-10 Upstream pinned to tag v1.14.0, not `develop` (develop has a HAL refactor in flight
  and reportedly broken envs).
- 2026-11-10 Android links over Web Bluetooth (Chrome). iPhone links over its own Personal
  Hotspot: the ESP32 joins as a station and serves the app over HTTPS + WSS (project CA installed
  once as a profile). No ESP32 access point.
- 2026-11-10 ESP32 ↔ STM32 on USART2 (PA2/PA3); USART1 stays on USB for ASCOM / debug.
- 2026-11-10 AZ and focus 28BYJ-48: both drive methods, per axis — ULN2003 on EXP1 pins or
  bipolar-modded on a free TMC2209 socket.
- 2026-11-10 Homing/limits with mechanical microswitches.
- 2026-11-10 Ender 3 V2 screen on USART3 (PB10/PB11) + knob on EXP2, new graphical UI (not OAT's
  16×2 menus). DWIN driver written from the DWIN T5UIC1 guide only; never copy Marlin or other
  GPL DWIN code into this MIT repo.
- 2026-11-10 Planes from free ADS-B (adsb.lol / airplanes.live), not Flightradar24.
- 2026-11-10 All motor/gear/driver settings editable from the phone and stored on the board,
  stock OAE values as defaults.
- 2026-11-10 One settings table in the firmware (id, EN/TR name, type, range, unit, default, group)
  is the single source: the onboard screen's Settings menu and the phone app's settings page are
  both generated from it (the app reads it over LX200), so the two always match.
- 2026-11-10 Onboard screen is the stock Ender 3 LCD (ENH12864Z-1, ST7920 128×64 mono, knob,
  buzzer) on the board's dedicated "Ender-3 EXP1" header, drawn with U8g2 (BSD-2). Supersedes the
  Ender 3 V2 DWIN plan.
