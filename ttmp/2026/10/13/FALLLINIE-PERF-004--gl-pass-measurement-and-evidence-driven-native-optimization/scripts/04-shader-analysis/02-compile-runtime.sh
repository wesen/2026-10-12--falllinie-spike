#!/usr/bin/env bash
# One host frame obtains exact glShaderSource bytes, not a performance test.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
R=$(git -C "$P" rev-parse --show-toplevel)
M=$P/.work/Arm_Performance_Studio_2026.5/mali_offline_compiler/malioc
mkdir -p "$P/evidence" "$P/inputs/runtime" "$P/evidence/reports"
cmake --build "$R/build/integrated-source" -j4 > "$P/evidence/dump-build.txt" 2>&1
cd "$R"
FALLLINIE_SHADER_DUMP_DIR="$P/inputs/runtime" SDL_AUDIODRIVER=dummy xvfb-run -a "$R/build/integrated-source/falllinie-spike" --freeze-scene --frames 1 --render-size 384x288 --assets "$R/assets/shaders" > "$P/evidence/dump-run.txt" 2>&1
"$M" --info --core Mali-G52 > "$P/evidence/g52-info.txt"
for src in "$P"/inputs/runtime/*; do
 name=$(basename "$src")
 "$M" --core Mali-G52 --opengles "$src" > "$P/evidence/reports/$name.txt"
 "$M" --core Mali-G52 --opengles --format json "$src" > "$P/evidence/reports/$name.json"
done
sha256sum "$P"/inputs/runtime/* > "$P/evidence/shader-hashes.txt"
