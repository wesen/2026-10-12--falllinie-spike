#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
R=$(git -C "$P" rev-parse --show-toplevel)
REL=${P#"$R/"}
mkdir -p "$P/evidence"
docker run --rm --pull=never --network=none --user "$(id -u):$(id -g)" -v "$R:/work" -w /work -e P="$REL" falllinie-spike-toolchain bash -euo pipefail -c '
 cmake -S "$P" -B "$P/.work/build-arm" -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
 cmake --build "$P/.work/build-arm" -j4
 aarch64-linux-gnu-readelf -h -n "$P/.work/build-arm/mali-counters"
' > "$P/evidence/build.txt" 2>&1
sha256sum "$P/.work/build-arm/mali-counters" > "$P/evidence/probe.sha256"
