#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
HOST="${HOST:-root@192.168.0.202}"
mkdir -p evidence/qualification
remote=(ssh -n -o IdentitiesOnly=yes -o ConnectTimeout=10 "$HOST")
"${remote[@]}" 'cd /storage/apps/falllinie-spike; ./test-terrain' > evidence/qualification/cpu.log 2>&1
# Deliberately read-only process/thermal context; no compositor policy changes.
"${remote[@]}" 'uname -m; pgrep -af "ds-demo-game/main.py|retroarch|emulationstation" || true; for f in /sys/class/thermal/thermal_zone*/temp; do printf "%s " "$f"; cat "$f"; done' > evidence/qualification/runtime-context.txt
run_case() {
  local name="$1"; shift
  "${remote[@]}" "cd /storage/apps/falllinie-spike; ./falllinie-spike --place $* --capture-dir captures/$name --receipt captures/$name/receipt.json" > "evidence/qualification/$name.log" 2>&1
  mkdir -p "evidence/qualification/$name"
  for file in receipt.json print.bmp scene.bmp depth.bmp; do
    scp -q -o IdentitiesOnly=yes "$HOST:/storage/apps/falllinie-spike/captures/$name/$file" "evidence/qualification/$name/"
  done
  for kind in print scene depth; do
    convert "evidence/qualification/$name/$kind.bmp" "evidence/qualification/$name/$kind.png"
    rm "evidence/qualification/$name/$kind.bmp"
  done
  printf 'Qualified %s\n' "$name"
}
run_case baseline-512 --seconds 12
run_case baseline-1024 --seconds 12 --render-size 1024x768
run_case marker-250 --seconds 10 --cell 2
run_case negative-cell --seconds 6 --cell -1 --seed 1
run_case seven-cells --seconds 12 --cell 12 --cells 7 --seed 12345
run_case resize-stress --frames 721 --resize-check
run_case signal-palette --seconds 6 --cell 2 --palette 1
run_case legacy-depth --seconds 6 --cell 2 --source-sampler-precision
node scripts/summarize-qualification.cjs
