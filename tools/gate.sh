#!/usr/bin/env bash
# Local gate: native unit tests, then the STM32 build once [env:robin_e3] exists.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$root/.tools/mingw64/bin:$PATH"
pio="$root/.venv/Scripts/pio.exe"
cd "$root/firmware"
"$pio" test -e native
if grep -q '^\[env:robin_e3\]' platformio.ini; then
  "$pio" run -e robin_e3
fi
