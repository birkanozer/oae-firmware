# SDD ledger — plan: plans/dwin-ui/2026-11-10-dwin-ui-plan.md

Spec: plans/dwin-ui/2026-11-10-dwin-ui-design.md. Branch feature/dwin-ui from dev 6bf95ef.
Workspace: .superpowers/sdd/2026-11-10-dwin-ui-plan/

## Pre-flight scan

| Pair / task | Shared file or interface | Finding |
|---|---|---|
| T1–T4 | DwinFrame/ByteSink: T4 renders via text/rect/clear/backlight | consistent |
| T2–T4 | TextId set: T2 defines ids "every label the screens need"; T4 owns the screens | GAP: T2 cannot know every label T4 will draw |
| T3–T4 | checkTarget(ra,dec,lst,lat,minAlt,y,m,d,utcHours) vs MountSnapshot (local date + localSeconds + utcOffsetHours) | consistent: T4 derives UTC (may cross midnight → date shift) |
| T4–T5 | MountSnapshot/UiAction/UiController API; DwinFrame sink = DwinScreen | consistent |
| T5–T6 | pins, TIMER_TONE choice documented by T6 | consistent |
| T1 | width max 48, font ≤ 5 | self-consistent |
| T2 | value units and ranges vs tests | self-consistent |
| T3 | tolerances vs low-precision Sun ±1° | self-consistent |
| T4 | StaleTurnsDropped requires onTurn timestamps | self-consistent |
| T5 | USE_DWIN_UI default 0, robin example 1 | self-consistent |
| T6 | docs only | self-consistent |

Ruling: T4 may add TextIds (both languages, ASCII, ≤ 20 chars) to UiText when a screen needs a label T2 did not define; T2's AllTextIdsHaveBothLanguages test then covers them automatically — cost if wrong: none.
Ruling: T4 computes UTC date/time from local snapshot minus utcOffsetHours, shifting the date across midnight, before calling checkTarget — cost if wrong: Sun check off by a day near midnight (±1° error), caught by a T4 test if added.

## Progress
Task 1: base bf1852a
Task 1: Ruling: TextSanitized per-byte printable-ASCII rule (CC 33 C3 3C → "???3?<") accepted instead of whole-sequence "????" — the requirement is that no tail/GB2312 byte reaches the screen, which per-byte guarantees (CC/C3 always replaced) — cost if wrong: none.
Task 1: minor (deferred): null `s` in text() and null buf in isHandshakeReply not guarded (caller contract).
Task 1: minor (deferred): fonts > 5 not rejected at encoder level (UI layer only uses 1/2/3/5).
Task 1: minor (deferred): missing tests for rect outline mode 0, width exactly 48, width 0.
Task 1: complete (commits bf1852a..7d335ab, review clean)
Task 2: base 7d335ab
Task 2: base 7d335ab
Task 2: fix round 1/5 dispatched (Important: Date begin not normalised → OOB read, month 13 confirmable; minors: Int min>max, dead code, test asserts, UiText spot asserts, TR wording) — FIX_BASE a2970f5
Task 2: fix round 1 landed ef6a078 (401/401); extra: Int sign group ignored when min >= 0
User decision 2026-11-10: screen is the stock Ender 3 LCD (ENH12864Z-1, ST7920 128x64 mono) on the "Ender-3 EXP1" header, not the V2 DWIN — DWIN-specific Task 1 output (DwinFrame) will be dropped and Tasks 4-6 re-planned for U8g2 after mockup feedback/audit.
User decision 2026-11-10: screen settings and app settings must match — one firmware settings table drives both (slice 3 builds the table; slice 2 screen settings menu must be built so it can render from that table).
Task 2: fix round 1/5 (6 addressed; 1 new Important: stuck -0 for Int min>=0; commits a2970f5..ef6a078)
Task 2: fix round 2/5 dispatched — FIX_BASE ef6a078; landed fdceb05 (402/402)
Task 2: fix round 2/5 (2 addressed, 0 open; commits ef6a078..fdceb05)
Task 2: minor (deferred): Int with min<0, max<=0 at 0 lets sign turn show "-0" (value still 0).
Task 2: complete (commits 7d335ab..fdceb05, review clean after 2 fix rounds)
Task 3: base fdceb05 (parallel with mockup-v2 agent, which edits only plans/dwin-ui/mockups/)
Task 3: base fdceb05, head 7f952a5 (411/411)
Task 3: fix round 1/5 dispatched (Critical: NaN inputs fail open to Ok; Important: invalid month → Sun at J2000; minors) — FIX_BASE 7f952a5

## Re-plan (user: display is ENH12864Z-1; "finish all tasks")
Plan continues in plans/dwin-ui/2026-11-10-lcd-ui-plan.md (tasks L1-L6); design 2026-11-10-lcd-ui-design.md; workspace .superpowers/sdd/2026-11-10-lcd-ui-plan/.
Pre-flight: L1 FieldEditor ext ↔ L3 editors (back/TimeHM/letters) consistent; L2 placeholders ↔ L3/L4 fill them; L2 MountSnapshot faultCode/rebooted ↔ L5 adapter sources (TMC test result, RCC_CSR) consistent; L4 AllScreensFit needs all screens from L2-L4 — runs last in L4, consistent; L5 startPark ↔ L2/L4 Parking state consistent.
Ruling: jog speeds labelled SLOW/CENTER/FIND/MAX mapped to OAT slew rates 1-4, overriding the v2 mockup's x1/x8/x64/MAX — OAT rates are fractions of max speed, not sidereal multiples — cost if wrong: label wording only.
Ruling: TargetCheck gets an Invalid value (from Task 3 fix round) that L3 shows as a refused screen — cost if wrong: none.
Ruling: UI proceeds without a separate plan-approval stop — user said "finish all the tasks and tell me when ready for manual inspection" — cost if wrong: user reviews the finished screens instead of the plan.
Task 3: fix round 1/5 (7 addressed; new: finite out-of-range inputs fail open C1-C5; commits 7f952a5..17306dc)
Task 3: fix round 2/5 dispatched — FIX_BASE 17306dc
L1: base 6c719b3, head 12f02b3 (415/415)
Ruling: '°' stays a UTF-8 code point in core strings; L5's U8g2Canvas must use drawUTF8 (the _tf fonts cover U+00B0) and width checks count code points — cost if wrong: garbled degree glyph on the LCD, visible on bench.
L2: base 12f02b3 (dispatched while L1 review runs; model opus — core state machine L3/L4 build on)
L1: minor (deferred): "°" literal relies on UTF-8 source encoding — use "\xC2\xB0".
L1: minor (deferred): setLetters before begin() silently lost; setLetters(0,x) half-set.
L1: minor (deferred): no test that FakeCanvas counts code points ("41°01'N" = 7 cells).
L1: minor (deferred): format buffers must be ≥ 12 bytes for "028°58'E" — callers (L3) must size accordingly.
L1: complete (commits d5951d1..12f02b3, review clean)
Task 3: fix round 2/5 (C1-C5 addressed, adversarial pass clean; commits 17306dc..d5951d1)
Task 3: minor (deferred): Sun-limit boundary float tolerance ~0.01° (a target at 14.99° might read Ok) — negligible.
Task 3: complete (commits fdceb05..d5951d1, review clean after 2 fix rounds)
L2: head 14441cc (438/438); self-review fix included.
Ruling: menu reachable while date/time unset via status click → Date editor → hold → menu — steers the user to set time first — cost if wrong: one extra action.
Ruling: Sync on target row stays in place, locked when no target (design "rows stay in place" over the mockup) — cost if wrong: none.
Ruling: lock after reset comes from dateSet/timeSet, which the L5 adapter reports false at every boot (time is never persisted) — cost if wrong: none.
Carry to L4: TR Jog label "Motorla kaydir". Carry to L5: space out multiple Beep actions from one tick.
L5: base 14441cc (dispatched early, parallel with L2 review; L5 forbidden to edit src/core/ui and unit_tests)
Session restart: L5 implementer died (expired token, no edits left); L2 review stopped (no file). Both resumed.
L2 review (file write refused by permission prompt after restart; findings taken from reviewer reply): Spec ✅; 4 Important (input before tick acts on stale snapshot; latches never cleared → 24.86-day wrap; queue overflow can drop Stop; fault doesn't lock motion), 3 minor.
Ruling: while a driver fault is active, motion rows are locked (Stop, Settings live) — moving on a failed driver is unsafe — cost if wrong: user must reboot/fix before moving.
L2: fix round 1/5 dispatched — FIX_BASE 14441cc
User: will re-enable auto mode (background writes were being refused).
User 2026-11-10: "make it faster" + "set input voltage 12 V / 24 V".
Ruling: L3 and L4 merged into one task L34 (one implementer, one review) to cut a review round — cost if wrong: a larger single review.
Ruling: L6 docs run in parallel now, written from the design; final review catches drift — cost if wrong: small doc fixes later.
Ruling: supply voltage 12/24 V = settings row + SetSupplyVoltage action (L34) and adapter apply (L5 addendum): TMC2209 stealthChop PWM_OFS ≈ 374·R·I/V initial value, 12 V caps slew speed at SUPPLY_12V_SPEED_FACTOR 0.7; default 24 V; persisted in slice 3 — cost if wrong: tuning constants adjustable in config.
L6: done 5ae5aec (docs). Ruling: no separate L6 task review — docs reviewed in the final whole-branch review (user asked for speed) — cost if wrong: doc errors found later.
L6: minor (deferred, final fix wave): wiring.md PB14 = EXP2 pin 1, PB15 = EXP2 pin 6 (Klipper map) missing; encoder positions on the Ender-3 header (pins 3 PB11, 5 PB0) to verify.
L2: fix round 1 landed 20be442 (7 addressed per implementer; 445/445; isolated-worktree gate robin_e3 SUCCESS). Re-review and L34 dispatched in parallel (speed).
L34: base 20be442
L2: fix round 1/5 (7 addressed, re-review PASS; commits 14441cc..20be442)
L2: minor (deferred): input before first tick dropped (tiny boot window); click queued in the same tick a fault arrives acts on the pre-fault screen (fault locks prevent motion); input queue may drop a menu click under alternating floods (Stop-equivalents always survive).
L2: note: real-tree gate pending — shared tree broken by L5's uncommitted UiMountAdapter (String ambiguous); isolated gate passed. Final gate on the full tree before release.
L2: complete (commits 12f02b3..20be442)
L5: done e3bb36d (gate 445/445, robin_e3 SUCCESS, RAM 6904 B, flash 139080 B; TIMER_TONE=TIM4). Added Mount::cancelPark() so STOP during park/home does not resume the slew. Payload units assumed: SetTime s since midnight, lat/lon arc-sec, UTC offset hours — L34 must match. SetSupplyVoltage not mapped yet (kind arrives with L34).
L5: review dispatched (sonnet), range 20be442..e3bb36d
L5: review — 0 Critical, 3 Important (UTC offset sign inverted; STOP lets upstream arrival block restart tracking; applySupplyVoltage unguarded on all TMC2209 boards), 4 Minor.
Ruling: fix 1-3 + minor 4 (jog shows as Slewing -> spurious ARRIVED) now; minor 5 (EXTI 11 vs focus UART RX PC11) -> wiring.md note in final wave; minor 6 (click acts on release) deferred — long press at 700 ms covers a held button, press-edge click would change menu semantics — cost if wrong: STOP delayed until button release.
L5: fix round 1/5 dispatched in a worktree (L34 is writing in the shared tree) — FIX_BASE e3bb36d
L5: fix round 1 landed 585fdac on fix/l5-review (gate pass 445/445, robin_e3 SUCCESS); cherry-pick onto feature/dwin-ui after L34 commits. Re-review dispatched.
L5: re-review round 1 — findings 1,3,4 fixed; 2 fixed but new Important: tracker-off compensation (runToNewPosition, blocking) still runs after UI Stop. Minor: stale flag on zero-length jog (cleared by next motion, harmless); Idle blip for one poll after jog-from-tracking.
L5: fix round 2/5 dispatched (same implementer).
L5: fix round 2 landed 21c001c (stopAll clears _compensateForTrackerOff; jog tail holds Jogging until tracking resumes, max 500 ms). Ruling: no third re-review — 1-line Mount change verified by reading, adapter change covered by final whole-branch review (speed) — cost if wrong: issue caught later in final review.
L5: complete (e3bb36d + fix/l5-review 585fdac, 21c001c; cherry-pick after L34 commits)
L34: done 2081e83 + 53bbb98 (480/480 native, robin_e3 SUCCESS, RAM 14.2 %, flash 58.6 %). Concerns: adapter never clears _synced (repeat "Clears Sync" prompt); 500 ms click-ignore catches jog-axis click after jog; Supply row no confirm (per brief).
L5 fixes cherry-picked onto feature/dwin-ui (clean): 585fdac, 21c001c.
Ruling: L34 task review folded into the final whole-branch review (opus) — user asked for speed — cost if wrong: a larger single review, possibly one more fix round.
Full-tree gate after cherry-pick: PASS (exit 0, robin_e3 SUCCESS).
User 2026-11-10: AZ 28BYJ-48 converted to bipolar, run on 12 V via TMC2209, intermittent duty — set current limits.
Ruling: AZ only (focuser later), Z socket (PC12 UART; E socket PC11 shares EXTI line 11 with knob A), ~100 mA RMS default (0.7 × 5 V / R_half), low hold, AZ_ALWAYS_ON 0; ULN2003 stays selectable; worktree branch feature/az-bipolar, own short review before release — cost if wrong: current tunable in config after measuring R.
Final review (opus): NOT READY. C1 fault lock drops while tracking (GoTo/Jog unlock on failed driver); I1 SetSupplyVoltage routed to beeper (kind >= Beep); I2 GO not re-checked at click (stale alt/Sun check); I3 Sync confirm unarmed; I4 wiring.md EXP1 1/2 swapped, wrong header column, EXP2 positions missing. Minors M1-M9.
Ruling: fix wave = C1, I1, I2, I3 (Sync armed 600 ms like GO — wrong sync defeats Sun/alt guards), I4+(d), (a) clear _synced on date/time/UTC/lon, (b) exempt Screen::Jog from 500 ms ignore, M1 Unpark locked like Track here (date+time), M2 adapter homing latch (HOMING screen, no tracking start on arrival), M6 fault label names the failing axis, M8 architecture.md stale lines, M9 bench step to time canvas.flush(), (e) reworded as "EXTI lines 0 and 11 reserved by the knob". Deferred: (c) Supply confirm, M3 phone-jog ARRIVED, M4 LX200 sync flag (slice 3), M5 EEPROM date prefill (slice 3), M7 Mount guards (inert on other boards; guards would litter upstream code) — cost if wrong: small UX/upstream-diff issues later.
Fix wave dispatched (sonnet) on shared tree, FIX_BASE 74760f8.
AZ: done 0276946 on feature/az-bipolar (480/480, robin_e3 SUCCESS, flash 59.3 %). Actual 91 mA RMS (IRUN 2, vsense 1, R_SENSE 0.11); hold 30 % (IHOLD 0), AZ_ALWAYS_ON 0; added _azDriverFailed guard in enableAzMotor (failed AZ would have run on VREF); SoftwareSerial is timer-sampled, no EXTI clash. Overlaps fix wave on fault code + wiring.md — cherry-pick after fix wave. Short review dispatched.
Final fix wave landed c5438e4 (485/485, robin_e3 SUCCESS, RAM 7008 B, flash 154360 B). Re-review dispatched (sonnet). Release note: drop CHANGELOG Fixed bullets that fix unreleased features; stale wiring.md ULN 'first five EXP1 pins are the screen' line to fix with the AZ merge.
AZ review: 0 Critical; Important: failed AZ driver still moves virtually (fake position stored); _azDriverFailed only honoured with UART_CONNECTION_TEST_TXRX. Minors: IHOLD 0 (harmless, EN released), fault 3 ambiguous (accepted: generic DRIVER FAULT), PWM_OFS uses requested not actual current (deferred, RA/DEC pattern), floating EN before b_setup (deferred, bench check), long #if line, wiring.md nits.
AZ fix round 1 dispatched: rebase onto c5438e4 + Important 1-2 + long line + wiring nits; feature/dwin-ui to fast-forward after.
Final re-review of c5438e4: all items Fixed, stop-anywhere holds; one doc nit (wiring.md:33 stale EXP1 sentence — EXP1 1-5 share MCU pins with the Ender-3 header, so taken) folded into the AZ rebase round.
AZ fix round 1 landed 976e8e9 (rebased 08f4a94); feature/dwin-ui fast-forwarded. Ruling: no AZ re-review — fix verified by reading the diff (enableAzMotor returns bool, all three AZ move paths guarded, flag unconditional, #error when UART test off) — cost if wrong: caught on bench.
Full-tree gate after AZ merge: PASS (485/485, robin_e3 SUCCESS, RAM 7032 B, flash 155728 B). Release v0.2.0 (feature, minor).
