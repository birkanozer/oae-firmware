# Ledger: adjust-per-axis

Branch `feature/adjust-per-axis` off `dev` @ 081ee9c. Design approved in chat 2026-11-28.

## Tasks

- [x] Task 1 UI fixes and per-axis step sizes (`task1-ui.md`) — sonnet/high — 2ac02ae; fix round 1 for final review findings
- [x] Task 2 DEC/ALT bench test build (`task2-bench.md`) — sonnet/high — d26429a (diff checked by controller)

## Rulings

- 2026-11-28 Exit from the Adjust options to the main menu via a new last row "Main menu" (user choice).
- 2026-11-28 Per-axis step size lives in RAM only (no settings block change).
- 2026-11-28 Skip fixes are bench experiments; backlog items stay open until the user confirms.
- 2026-11-28 One Opus whole-branch review instead of per-task Sonnet reviews (two small tasks).
- 2026-11-28 Turkish row text "Ana menu" (LCD font and test_ui_text are ASCII only).
- 2026-11-28 Options keep the automatic Back row after Main menu; Back = one level up = Adjust.
- 2026-11-28 Dead UI jog plumbing (JogStart/JogStop/SetSlewRate kinds, adapter jog state) removed in this branch.
