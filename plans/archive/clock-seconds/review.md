# Review: seconds in the clock (fix/clock-seconds, 76891fd)

Read: brief, report, package, UiScreens.cpp, UiController.cpp, FieldEditor.cpp, UiDraw.hpp, UiText.cpp, UiMountAdapter.cpp, Mount.cpp (time/date), LcdUi.cpp, the touched tests.
Not run: gate (report says 557/557 green + robin_e3 build); verdicts below come from reading.

## Verdict 1: spec compliance — MET

| Brief item | Status | Evidence |
|---|---|---|
| 1. Time editor uses FieldKind::Time, seconds editable, reach SetTime | met | UiScreens.cpp:55-57 is the only `Edit::Time` begin; all `openEditor(Edit::Time, ...)` callers (Setup row :408, fast path :98) go through it. `editorDone` (:107-118) pends `_editor.value()` unchanged. FieldEditor `_n` is 3 for Time, `value()` includes seconds. |
| 1b. Sub-labels / layout handle 3 groups | met | `renderEdit` default `sub = kHms`, `n = 3`; the n=2 override was the only thing forcing 2. `renderEditor` groups by alnum runs, centred with `x0 = (128 - 10*cols + 1)/2`, 8 cols = 80 px, same as RA. |
| 2. Setup Time row hh:mm:ss, placeholder `--:--:--` | met | UiScreens.cpp:250-254. Row = label + pad to 19 cols; "Time"/"Saat" + 8 = 12 cols, narrower than the existing Date row (10-char value) and Longitude (8). No overflow on the 121 px highlight box. |
| 3. Saver hh:mm:ss in F10x20, placeholder | met | UiController.cpp:895-899. 8 x 10 = 80 px, centred; `violations == 0` asserted in the saver test (both timeSet and unset). |
| 4. Remove TimeHM only if unused | met | repo-wide grep: no code user left (only archived plans and history). Enum, begin(), format() case, header comment and its test all removed. `format()` default branch still serves Time/RA. |
| 5. Tests | met | see below. |
| Rules | met | `-Werror` build per report, ASCII text, trailer per brief. Wrapped snprintf at renderSaver: unwrapped it is 142 cols (> 140), so the wrap is required and matches clang-format v12 bin-packing. |

### Extras

- Removal of `TextId::FootAt00` and the "click at :00 sec" / "tikla: :00 sn" table row: **needed consequence, not scope creep.** The hint says seconds become :00 at the click, which is now false, and the id would be dead. `kTable` is `static_assert`ed against `TextId::Count`; the enum entry and both-language table row were removed together, so the table stays consistent (EN and TR share one `Entry`). Nothing else references the id.
- Footer on the Time editor is now the default `FootSave` ("click: save" / "tikla: kaydet"). Same footer Date, Lat, Lon already use; fits the 21-col row. Its use is asserted in FastPathDateThenTime.
- No EN/TR text table or text-length test left inconsistent: AllScreensFit walks the editor and Setup list in both languages and still checks `violations`.
- Docs: no `hh:mm` / ":00 sec" hits in docs/, README, app/. CHANGELOG still needs the controller's entry (brief says controller does docs).

## Verdict 2: correctness and quality — APPROVED, Minor findings only

No Critical or Important issues.

What was checked and is fine:

- Seconds path: editor value (0..86399 incl. seconds) -> `pend(SetTime, v)` -> `UiMountAdapter::apply` -> `DayTime(a/3600, a/60%60, a%60)` -> `Mount::setLocalStartTime`. Seconds survive end to end, `getLocalTime()` counts on from them.
- Date rollover at 23:59:59: setting a time does not touch the date; `getLocalDate()` computes day end from `_localStartTime.getTotalSeconds()`, which is now more exact because seconds are kept (before, it was always :00). `localToUtcDate` takes full local seconds, so the UTC-offset path is unchanged and LST gets seconds-accurate input.
- Editor wrap: `FieldEditor::turn` wraps inside the group, so 59 -> 00 does not carry into minutes (stated design, "nothing carries"). No path produces seconds >= 60 or a value > 86399.
- Redraw: saver now changes every second so the HashCanvas differs each second; status screen already did the same, frame gap 250 ms, no new load of note.
- Tests: FastPathDateThenTime (seed :07, +1 -> :08 reaches SetTime with date), TimeSecondsEdited (10:05:59, +1 min, -4 s -> 10:06:55), SetupTimeRowShowsSeconds (value and placeholder), saver test (value, placeholder on the saver itself, no violations). The `u.drew("turn to wake")` addition in the placeholder branch fixes a real hole: before, `--:--` could also match the status header, so the old assertion proved nothing about the saver. Click counts +1 in the 4 flow tests are exactly the one extra group.

### Findings

1. `firmware/src/core/ui/UiController.cpp:895-899` Minor: the saver re-implements `draw::hms()` (UiDraw.hpp:32, same `%02d:%02d:%02d`, already used by renderStatus at :766), which is why the line had to be wrapped. Failure scenario: none today; a later change to the clock format has to be made in two places. Fix: `hms(l, sizeof l, _m.localSeconds);` (3 lines -> 1, no wrap).
2. `firmware/src/core/ui/UiScreens.cpp:98,408` Minor (behaviour change, not a bug): the editor is seeded from `_m.localSeconds` at open, so untouched seconds are the value at open time and are stale by however long the user spends on hh:mm. Before, the confirm click always gave :00. While the time is unset, `localSeconds` is seconds since boot, so a user who clicks straight through gets arbitrary seconds (they were already getting arbitrary hh:mm). Acceptable because the point of the patch is that the user sets the seconds; worth one line in the CHANGELOG/user docs ("set seconds ahead, click on the beat"). Fix, if wanted: nothing in code.
3. `firmware/unit_tests/test_core/ui/test_ui_flows.cpp` (TimeSecondsEdited) Minor: "three groups" is shown only indirectly (the third click reaches the "seconds" label, the fourth confirms). Failure scenario: a regression to `n = 2` would still fail, so the guard is real; an explicit `EXPECT_TRUE(u.drew("3/3"))` on the seconds step would state the brief's intent directly. Optional.

## Summary

Spec: met in full; the FootAt00 removal is a required consequence. Quality: approved, three Minor notes (saver should reuse `hms`, stale-seed behaviour to document, optional "3/3" assertion).
