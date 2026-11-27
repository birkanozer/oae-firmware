# Magnetic camera module, eyepiece rings, focus motor mount (design)

Date: 2026-11-26. User: cap design is better; add a magnetic aligner: each eyepiece keeps a part with
3 magnet points, the camera module (USB-C port, stepper driver, ESP32, integrated camera) snaps onto
it; the focus motor is a different model. Decisions: stepper driver in the camera module, the
28BYJ-48 plugs into it with its own socket; magnets + balls and V-grooves (kinematic mount).

Telescope: 76/700 Newtonian, 1.25" focuser, eyepieces 25 / 12.5 / 8 mm, almost always in the 3x Barlow.
Optics as in `hardware/camera-cap/` (afocal, OV5640 lens at the eye relief, gap formula
`gap = eye_relief - lens_recess - lens_stop_offset`, defaults eye relief 18 / 9 / 5, recess 3,
stop offset 1.5, housing OD 33).

## A. Eyepiece ring (one per eyepiece, stays on it) — replaces the camera-cap "cap"

- Sleeve over the eyepiece top, three radial M3 thumbscrews at 120 deg (captive nuts): grip and
  centre once, then left on the eyepiece.
- Top face = kinematic seat: **three radial V-grooves at 120 deg** (90 deg included angle) for the
  module's balls, plus **three magnet pockets** (default 6 x 3 mm N52 discs) next to the grooves, and
  **one key notch** so the module always sits the same way round (USB-C and motor socket on a known
  side).
- **The ring's height carries the per-eyepiece eye relief**: the seat plane is placed so the module's
  lens lands at the right gap for that eyepiece. The module is identical for all eyepieces.
- Light-tight: the ring's inner tube meets a matching skirt on the module (labyrinth step) so no stray
  light gets in between ring and module.

## B. Camera module (one) — replaces the camera-cap "head"

- Closed two-part enclosure (base + lid, M2 or snap), housing:
  - ESP32-S3 CAM board (parameter size, default 27 x 50 mm; standoffs or rails);
  - OV5640 module lens-down on the optical axis, protruding through a nose into the ring;
  - ULN2003 driver board for the focus motor (default the common 35 x 32 mm board with 4 x M3 holes
    at 30 x 27 mm; parameters) — or room for a bare ULN2003 on a small perfboard;
  - USB-C breakout panel-mounted in the wall (serial + 5 V cable from the Robin, see docs/wiring.md);
  - **JST-XH 5-pin socket** panel-mounted (or the ULN board's own socket reachable through a wall
    cut-out) so the 28BYJ-48's own plug goes straight in;
  - optional antenna exit hole.
- Underside: three **steel balls** (default 5 mm bearing balls, pressed/glued into hemispherical
  sockets) matching the ring's grooves, three magnet pockets matching the ring's, the key tab.
- Vent slots away from the optical path (the ESP32 and ULN warm the enclosure; keep warm air off
  the eyepiece).
- Total mass low (target < 80 g with magnets); magnet pull must hold it upside down (4 x N52 6x3 mm
  is ~ 1 kgf+; 3 is plenty) — note in README.

## C. Focus motor mount (separate model, `hardware/focus-mount/`)

- 28BYJ-48 (stock unipolar, 5 V) on a bracket that clamps the focuser body, driving the focus knob
  through a GT2 6 mm belt: printed GT2 pulley ring that clamps onto the focus knob (parameter knob
  diameter, default 30 mm, and knob width default 12 mm) and a printed GT2 pulley on the 28BYJ-48's
  5 mm double-flat shaft (default 20 teeth). Belt tension by a slotted motor plate.
- Parameters for the focuser body (default a cheap rack-and-pinion: body block 40 x 40 mm, knob shaft
  height from the tube), all to be measured; closed-loop belt length computed and printed in the
  console (`echo`) so the user can buy the right belt.
- Clears the eyepiece ring and camera module (they are above the drawtube; the motor sits beside
  the knob on the side away from the tube).

## Firmware and docs consequences (docs only now; code in the bridge slice)

- Focus moves from the Robin's ULN2003 (group B, v0.4.0) to the ESP32 in the camera module: the
  ESP32 drives the ULN2003 with 4 GPIOs (pick free pins on the chosen ESP32-S3 CAM board; camera, SD
  and the bridge UART take many). The Robin's focus code stays as a fallback but the example config
  will switch focus off when the bridge slice lands; LX200 focus commands (`:F...`) and the LCD
  Focus axis will be served by the ESP32 (e.g. the Robin queues LCD focus moves and the ESP32 picks
  them up in its poll). Record this in docs/wiring.md, docs/architecture.md, CLAUDE.md Decisions and
  BACKLOG.
- Power: 28BYJ-48 ~ 240 mA at 5 V from the Robin's 5 V over the USB-C cable's VBUS, plus the ESP32
  ~ 0.5 A: ~ 0.75 A, fine for the cable and the MP1584 (3 A).

## Deliverables

- `hardware/camera-cap/`: redesigned into ring + module (`part = "ring" | "module_base" |
  "module_lid" | "assembly"`, `eyepiece = 25 | 12.5 | 8`, `section`), README updated (measuring,
  printing, balls/magnets to buy, polarity: all ring magnets one pole up, all module magnets the
  opposite, centring a ring once with the live view).
- `hardware/focus-mount/focus_mount.scad` + README.
- Previews: hardware/camera-cap/preview.png (assembly, module on the 12.5 mm ring), section.png,
  exploded.png; hardware/focus-mount/preview.png.
- STLs not committed.
