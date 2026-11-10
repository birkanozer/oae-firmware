# OAE 12864 screen mockups: independent UX audit

Sources: `plans/dwin-ui/mockups/screens-12864.html` (+ `.png`), `plans/dwin-ui/2026-11-10-dwin-ui-design.md`.
Screen numbers = mockup captions 1-18. Character counts are for the 6x10 font (21 columns).
Strings proposed below are counted; `"` and `°` count as one column each.

**Width budget the mockups actually give (not 21 everywhere):**
- Status lines and footer: 21 columns.
- Header: 21 columns in total, left text + right text + at least 1 space. With the clock on the right (8 columns), the state word can be at most 12 columns.
- **Menu rows: 19 columns.** The text starts at x=2, the selection bar ends at x=121 and the scrollbar takes x=124-127, so (121-2)/6 = 19.
- Big-digit editors (10x20 font): 12 columns.

Counts: **Must fix 7 · Should fix 20 · Consider 14**

---

## Must fix

### M1. STOP is not one gesture, and long press does the wrong thing mid-slew (screens 3, 5, 7-14, 17)
**Problem.**
- From the status screen during a GoTo, stopping takes click (open menu) + click (STOP), and it only works because STOP is preselected. Nothing on screen 3 says how to stop: the footer, where a hint would go, is a progress bar.
- From anywhere deeper (an editor, Setup, Settings, Home/Park), stopping means holding 0.7 s once per level to get back to the menu, then turning up to STOP if the cursor kept its old position, then clicking. That is 3-6 actions and 2-4 s.
- The panic reflex is to press and hold. Today a hold means Back: in screen 5 it leaves the menu that contains STOP, so the motion carries on.
- The design does not say what screen the UI goes to after CLICK = GO, or what a long press does on the status screen.

**Fix.** One global rule: **while a slew, home, park or LX200-commanded move is running, any click or any long press on any screen = STOP.** The only exception is Jog, whose own motion already stops 0.3 s after the knob does.
- After CLICK = GO, return to the status screen.
- During motion, the footer becomes an inverted bar `click: STOP` (11). TR: `tikla: DURDUR` (13).
- On STOP: one long beep, and the header reads `STOPPED` (TR `DURDU`) for 2 s.
- The same rule must stop moves that were started over LX200 (phone or PC), not only moves started from the screen.

### M2. The menu shifts under the cursor when motion starts or ends (screens 4, 5)
**Problem.** STOP is inserted at row 0 when motion begins and removed when it ends. Every row then moves by one. A click aimed at "STOP" just as the GoTo finishes lands on "Track here". A click aimed at "Jog" lands on "Stop tracking" or "GoTo". Screen 4 (8 items) and screen 5 (9 items) show the problem.

**Fix.**
- Give row 0 one fixed role, the motion slot, whose label follows the state: idle `Track here`, tracking `Stop tracking`, slewing/homing/parking `STOP`. Every other row keeps its index.
- When the row-0 label changes, ignore clicks for 500 ms and move the cursor to row 0 if motion has just started.
- Always open the menu at row 0, so "click, click" has one predictable meaning.

### M3. The screen saver swallows the STOP input and can engage mid-motion (screen 18)
**Problem.**
- The saver starts after 2 min with no input, whatever the mount is doing. A park or home on slow axes, or an LX200 slew, can run under it.
- The first input only wakes the screen, so STOP then takes 3 actions.
- The saver shows no warning (time not set, fault) and gives no hint of how to leave it.

**Fix.**
- Never enter the saver while anything other than sidereal tracking moves.
- If motion starts while the saver is shown (LX200), leave the saver at once.
- Otherwise the first turn or click only wakes the screen, and is not passed on.
- While a warning is active, show it on the saver's bottom line in place of RA/Dec, for example `SET DATE & TIME` (15).

### M4. Jog has no way to switch axis (screen 6)
**Problem.** Turn = move, click = speed, hold = back. That uses every gesture, yet the RA/DEC tabs imply a choice. Centering a star needs alternating axes many times. The only path today is to leave (hold) and re-enter, and even that has no axis-pick step.

**Fix.** Click toggles RA/DEC, because that is the frequent action while centering. Move speed out of the click:
- Option A: the Jog entry is a 3-row submenu: `Jog RA`, `Jog DEC`, `Speed  x8`. Hold returns to it.
- Option B: hold changes speed, and leaving Jog = 2 s hold or a `Back` tab.

Option A is simpler with gloves. Either way, footer `click: axis` (11), TR `tikla: eksen` (12).

### M5. Sync has no confirmation and no feedback (no screen for it; menu screens 4, 5, 16)
**Problem.** A mistyped or accidental Sync silently moves the whole pointing model. GoTo has a confirm screen (9) but Sync does not. Nothing tells the user that Sync happened.

**Fix.** Add a `SYNC / CONFIRM` screen in the same layout as screen 9:
- `RA 05:35:17` / `DE -05°23'28"` / `Move dRA+0:12 dDE-1°` (20)
- inverted `CLICK = SYNC` (12), footer `hold: cancel` (12)
- If the offset is more than about 5°, add a line `BIG JUMP - CHECK` (16).
- After the click: double tick and header `SYNCED` (TR `ESITLENDI`, 9) for 2 s.

### M6. GoTo unlocks on time alone, so a stale date defeats the Sun gate (screens 1, 9, 11)
**Problem.** The design sets `isTimeSet()` from `setLocalStartTime` (time only). With no RTC, the date is a default or build date. The Sun moves about 1° per day, so a date more than about 2 weeks off puts the Sun outside the 15° check. The safety gate then passes a target close to the real Sun.

**Fix.**
- Unlock GoTo, Sync and Track only when **both** date and time were set this boot (two flags).
- The status warning names what is missing: `SET DATE & TIME` / `SET DATE` / `SET TIME`. TR: `TARIH/SAAT AYARLA` (17), `TARIHI AYARLA` (13), `SAATI AYARLA` (12).

### M7. Blocking `park()` / `setHome()` could freeze the UI while axes move (screen 13; design doc "Gotchas")
**Problem.** The design notes that `park()` and `setHome()` block until the axes stop. If the main loop blocks during a park slew, the knob is not read and the screen is not redrawn, so STOP is unreachable during the longest unattended move.

**Fix.**
- Confirm on the bench that the UI loop keeps running while a park slew is under way.
- If it does not, turn park into "start the slew, then poll", with only the final stop-and-store step blocking.
- During the move, show the state `PARKING` / `HOMING` (TR `PARK EDIYOR` 11, `EVE GIDIYOR` 11) and the M1 footer `click: STOP`.

---

## Should fix

### S1. First-night date/time entry costs too many actions (screens 1, 11, 12)
**Today, from cold boot:**
- Navigation: click menu, 4 turns to Setup, click.
- Date: click, then 3 groups and 3 clicks.
- Time: 1 turn, click, then 3 groups and 3 clicks.
- Then 2 holds back to the status screen.

That is about 10 clicks, 5 navigation turns and 2 holds, plus value turns. With no prefill and no acceleration, the value turns run to 100+ detents (year 2000→2026, hours up to 12, minutes and seconds up to 30 each). Entering seconds is also pointless: they are stale by the time you click.

**Fix.**
- (a) While the warning shows, a click on the status screen goes straight to the **Date → Time** editors and then back to the status screen. That makes 1 + 3 + 2 = 6 clicks.
- (b) Prefill the date from the last saved date (EEPROM), or else from the firmware build date.
- (c) The time editor has `hh:mm` only. Seconds are set to 00 at the confirm click. Footer `click at :00 sec` (16), TR `:00 da tikla` (12).
- (d) Use encoder acceleration (fast turning = ×10 steps).

### S2. Sync after a GoTo means re-typing the same star (menu; editors 7, 8)
**Problem.** The normal workflow is: GoTo a star, center it with Jog, then Sync on that same star. Today Sync opens empty editors: 7 groups again.

**Fix.**
- Add a menu row `Sync on target` (14), TR `Hedefe esitle` (13). It goes straight to the M5 confirm screen with the last GoTo target.
- Prefill the GoTo and Sync editors with the last target, not zeros.

### S3. Editors cannot step back a group, and cancel is hidden (screens 7, 8, 12, 17)
**Problem.** A mistake in the RA hours, noticed in the Dec editor, means hold (cancel everything) and start again. No footer ever says how to cancel.

**Fix.**
- Hold = previous group. Hold on the first group = cancel.
- Footer `click:next hold:back` (20), TR `tikla:ileri uzun:geri` (21).

### S4. An accidental GoTo can come from the click rhythm (screens 8 → 9)
**Problem.** Confirm comes after a run of identical "next" clicks. One extra click, or a bounce, fires the slew. With no pier or tripod limits, an unexpected slew is a collision risk.

**Fix.**
- Ignore clicks for 600 ms after the confirm screen appears, and show `CLICK = GO` only once it is armed.
- Add Az to the confirm screen so the user can sanity-check the direction: `Alt+38° Az161° Sun112` (21).

### S5. Incompatible actions stay offered while moving or parked (screens 5, 11, 13)
**Problem.**
- Screen 5 offers GoTo, Sync, Setup and Home/Park in the middle of a slew.
- Changing date, time or longitude mid-slew re-bases RA under the moving mount.
- Park and Unpark are both always listed.

**Fix.**
- While slewing, show only `STOP`, `Settings` and `Back`.
- When parked, show `Unpark` and hide Park, Go home and Jog.
- When idle, hide Unpark.
- Before Setup commits a date, time, UTC or longitude change after a Sync, confirm `Clears Sync. Sure?` (18), TR `Esitleme silinir!` (17).

### S6. Locked items look identical to unlocked ones (screens 1, 4, 16)
**Problem.** Before the time is set, Track here, GoTo and Sync look exactly like usable rows. The cursor opens on a locked row 0. The design does not say what a click on a locked row does, so the user hits a dead end.

**Fix.**
- Mark locked rows with a suffix that fits in 19 columns, for example `GoTo RA/Dec  (time)` (19). Alternatively, prefix a `-` and draw the row with dotted underlining.
- A click on a locked row: double beep and a 2 s footer message `Set date+time first` (19), TR `Once tarih/saat gir` (19). Then offer the S1 editors.

### S7. The status screen leaves space unused and misses what an astronomer checks (screens 2, 3, 15)
**Problem.**
- No hour angle, no pier/meridian information, no date and no UTC offset.
- Alt/Az have no spaces and no degree marks (`Alt+38 Az161`).
- During a GoTo, the target is not shown, only a percentage bar.
- Every line has 8-10 free columns.

**Fix.** Proposed layout (all 21 columns or fewer):

| Row | Content | Columns |
|---|---|---|
| Header | `TRACKING    23:41:07` | 20 |
| 1 | `RA 05:35:17 HA-00:37` | 20 |
| 2 | `DE -05°23'28"  pier E` (pier side only if the mount flips; otherwise drop it) | 21 |
| 3 | `Alt +38°  Az 161°` (TR `Yks +38°  Az 161°`) | 17 |
| 4 | `LST 04:58:02 UTC+3` | 18 |
| Footer | hint, warning, or `click: STOP` while moving | ≤ 21 |

During a GoTo, row 4 becomes the target, `> 05:35:17 -05°23'` (18), and the header keeps `GOTO 62%`. Drop the progress bar.

Number formats: Az always 3 digits with leading zeros (`Az 061°`); Alt always signed, 2 digits, with `°`.

### S8. Feedback is missing after the action is done (screens 3, 9, 13)
**Fix.**
- GoTo arrival: 2 beeps, then header `ARRIVED` (TR `HEDEFTE`) for 3 s, then the tracking state.
- Set home here overwrites the reference with no undo, so it needs a confirm: `Set home here?` (14) + `click = yes` (11), TR `Ev burasi olsun?` (16).
- After it is done: `HOME SET` (TR `EV AYARLANDI`, 12).
- Park end: state `PARKED` (TR `PARKTA`), stays until Unpark.
- Track here: header changes to TRACKING plus a tick. This already happens, keep it.

### S9. The refused screen is cryptic and drops the user's entry (screen 10)
**Problem.**
- `min +0°` is unclear.
- There is no Sun variant.
- `click: back` exits the whole flow and loses the typed target.
- It is the only screen where click means back.

**Fix.**
- Line 3: `Alt -13° (min 0°)` (17).
- Box text: `BELOW HORIZON` (13) / `TOO CLOSE TO SUN` (16), with the line `Sun 9° away (min 15)` (20). TR: `UFKUN ALTINDA` (13) / `GUNESE COK YAKIN` (16).
- Footer `click: edit` (11). It returns to the RA editor with the values kept.

### S10. The longitude sign convention is ambiguous (screens 11, 17)
**Problem.** `Lon +028°58'` could mean east-positive (ISO) or west-positive (LX200). A wrong sign shifts LST and makes the altitude gate wrong.

**Fix.**
- Show hemisphere letters and edit them like the sign group: `Lat  41°01' N` (13), `Lon 028°58' E` (13).
- TR: `K/G` for north/south and `D/B` for east/west, for example `Enlem 41°01' K` (14).

### S11. Truncation risk: menu rows are 19 columns, and the TR strings are unverified
**Problem.** Today's strings fit. Not mocked (counts): `Evi buraya ayarla` 17, `Takibi durdur` 13, `Parktan cik` 11, `Hedefe esitle` 13, `Dil/Lang   Turkce` 17. Any future TR row over 19 columns overruns the bar.

**Fix.** A unit test that fails if any menu string is over 19 columns, a header state word is over 12, or any other line is over 21.

### S12. Two TR wording problems (screens 15, 16)
- **`Elle hareket` ("move by hand") for motorised Jog** clashes with the real hand-aiming flow (`Burayi takip et`). Use `Motorla kaydir` (14).
- **`Yuk` reads as "yük" (load).** Use `Yks` (or `Yuks`).
- Also: the Turkish percent order is `%62`, so `GIT %62`.

### S13. The inverted header bar is the biggest light source on every screen (all screens)
**Problem.** On this blue-negative STN panel, lit pixels are the bright ones. A 128×11 white bar on every screen is the largest bright area, which hurts dark adaptation. It also spends the "inverted = selected or alert" meaning on plain chrome.

**Fix.**
- Header in normal text plus a 1-px dotted rule under it, like the footer separator.
- Reserve inversion for the selection bar, alerts and the STOP footer.

### S14. Warnings are not persistent, there is one warning slot, and fault screens are missing (screens 1, 2, 18)
**Problem.**
- The only warning slot is the status footer, so warnings vanish in every other screen.
- No alert when a tracked object sinks below the minimum altitude, or nears the meridian/pier.
- No screen for a TMC driver UART fault or stall.
- No screen for a reboot mid-session (brownout in the cold): today it silently shows IDLE.

**Fix.**
- A full-screen fault page, entered on any screen: header `FAULT`, then `RA DRIVER FAULT` (15) / `Motors stopped` (14), footer `click: ok` (9). TR: `RA SURUCU HATASI` (16) / `Motorlar durdu` (14).
- While tracking: `BELOW MIN ALT` (13) / `MERIDIAN IN 10 MIN` (18), with 3 beeps once.
- After an unplanned reset: `REBOOTED - SET TIME` (19), TR `YENIDEN BASLADI` (15).
- Show a `!` in the header's last column on every screen while any warning is active.

### S15. Jog speed is shown as `2/4`, which is meaningless, and Jog gives no help at the eyepiece (screen 6)
**Fix.**
- Show the rate as a multiple of sidereal: `JOG RA        x8` (header). Speeds `x1 x8 x64 MAX`.
- Beep the speed number (2 beeps = speed 2) so it works with the eye at the eyepiece.
- Direction hint in the footer: `turn: E-W click:axis` (20).

### S16. Settings rows are misaligned, the language toggle is risky, and settings reset each boot (screen 14)
**Fix.**
- Right-align the values at column 19, for example `Min alt          0°`.
- Use `0°`, not `+0°`.
- Make the language row bilingual so it can always be found again: `Dil/Lang   English` (18).
- While storage is missing, make the boot language a build flag, so TR users do not land in EN every night.

### S17. The screen saver's rationale and exit are unclear (screen 18)
**Problem.** An STN LCD does not burn in. The real value of the saver is a big, readable clock.

**Fix.**
- Keep the saver, but apply M3.
- When the mount is idle, show `turn to wake` (12) on the last line.
- Optionally offer a "blank" saver mode for imaging nights.

### S18. Footer hints are inconsistent (screens 7-12, 17)
**Problem.** The last group says `click: ok` on screen 12 but `click: next` on screens 7 and 8, even though the Dec editor ends at confirm. Cancel is never shown. Screen 10 uses click for back.

**Fix.** One scheme:
- Middle group: `click:next hold:back`.
- Last group of a GoTo or Sync: `click: check` (12).
- Last group of a Setup editor: `click: save` (11).

### S19. Labels and header conventions are inconsistent (various screens)
- The declination label appears as `DE` (status), `DEC` (jog and editor headers) and `Dec` (menu). Pick `Dec` in body text and `DEC` in headers.
- The page caption says "header 5×7", but the code draws the header in 6×10. Decide which one is meant.
- A step indicator appears on the GoTo editors (`1/2`) but not on the Setup editors (12, 17).
- In menus, `1/8` looks like "page 1 of 8" but is the cursor index. Either drop it (the scrollbar already shows the position) or keep the step format for editors only.
- The editor shows Dec seconds with no `"` mark (screen 8), while the status screen shows it.

### S20. STOP and "Stop tracking" have undefined meanings (screen 5)
**Problem.** Both rows appear together. It is unclear whether STOP also stops tracking, and what state follows a STOP.

**Fix.** Define it and show the result in the header. Recommended: STOP = all motors, including tracking. The header then reads `STOPPED` and row 0 becomes `Track here` (consistent with M2).

---

## Consider

- **C1. Polar alignment helper.** Polaris position from the date, time and longitude: `Polaris HA 07:12` (16) and as a reticle clock position `Polaris at 4:30` (15). It is the first thing a portable user needs.
- **C2. Cold LCD.** STN response slows down a lot near or below 0 °C. Ticking seconds and jog digits will smear. Consider an `hh:mm` header clock while not editing, and redraw changing fields at 1-2 Hz.
- **C3. UTC offset in 15/30-minute steps** (+5:30, +5:45, +9:30, -3:30). Whole hours are wrong for some users.
- **C4. A built-in list of about 30 bright alignment stars** for GoTo and Sync, so nobody has to type coordinates from a printout at night. The design doc already has this in the backlog; it matters most for Sync.
- **C5. A low-precision GoTo editor option** (`hh:mm` and `±dd°mm'`). It cuts 2 clicks per GoTo. Keep the seconds for Sync.
- **C6. A wrap-around menu.** Turning up from row 0 reaches `Back` in 1 detent, instead of the 7-8 detents it takes today.
- **C7. First-ever boot.** If latitude and longitude are still at defaults, warn `SET LOCATION` (12), TR `KONUM AYARLA` (12). Add a 1 s boot splash with the firmware version.
- **C8. A synced marker.** A `*` after RA on the status screen once a Sync has been done this session. The clock drifts only about 1-2 s per day on the board crystal, so a "time since set" display is not needed.
- **C9. Night light.** The backlight is hard-wired and cannot be dimmed. Recommend a red gel or acrylic over the panel in the docs. A backlight PWM mod is out of scope.
- **C10. A remote-control indicator.** Show `PC` (or `BT`) in the header when a move or setting came over LX200, so a slew the user did not start explains itself.
- **C11. Tracking rate** (sidereal, lunar, solar), if the mount supports it. Today the state word is the only tracking indicator.
- **C12. Track here without time.** Sidereal tracking does not need the clock. Allow it before the time is set, with RA shown as `--` and GoTo and Sync still locked. That helps purely visual users.
- **C13. Degree glyph.** `°` (0xB0) needs the U8g2 `_tf` font variants, not `_tr`, or a custom glyph. In the 10×20 editor, the 3×3 ring in a 10-px cell reads as a gap, as if `-05 23'28` were three numbers. Consider a 5×5 ring or a narrower cell.
- **C14. TR terms.** `Ev / Park` is literal. `Baslangic / Park` may read more naturally to Turkish amateurs. Check with users.
