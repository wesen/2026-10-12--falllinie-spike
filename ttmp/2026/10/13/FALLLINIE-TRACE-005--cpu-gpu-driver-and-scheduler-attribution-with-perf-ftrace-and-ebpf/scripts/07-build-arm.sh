#!/usr/bin/env bash
# Existing offline toolchain and approved target libraries; no installation.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
R=$(git -C "$P" rev-parse --show-toplevel)
mkdir -p "$P/evidence/build" "$R/build-device"
REL=${P#"$R/"}/evidence/build
git -C "$R" rev-parse HEAD > "$P/evidence/build/source-revision.txt"
sha256sum "$R"/src/* > "$P/evidence/build/source-hashes.txt"
sha256sum "$R/target-libs/libSDL2-2.0.so.0" > "$P/evidence/build/target-library-hashes.txt"
docker image inspect --format '{{.Id}}' falllinie-spike-toolchain > "$P/evidence/build/image.txt"
docker run --rm --pull=never --network=none --user "$(id -u):$(id -g)" -v "$R:/work" -w /work -e RECEIPTS="$REL" falllinie-spike-toolchain bash -euo pipefail -c '
 aarch64-linux-gnu-g++ --version > "$RECEIPTS/compiler.txt"
 aarch64-linux-gnu-g++ -std=c++17 -O2 -g -fno-omit-frame-pointer -Wall -Wextra -pthread -I/usr/include/SDL2 -I/usr/include/aarch64-linux-gnu src/*.cpp -o build-device/falllinie-trace005 -Ltarget-libs -Wl,--allow-shlib-undefined -Wl,--build-id -l:libSDL2-2.0.so.0
 aarch64-linux-gnu-readelf -h -n build-device/falllinie-trace005 > "$RECEIPTS/elf.txt"
' > "$P/evidence/build/build.log" 2>&1
sha256sum "$R/build-device/falllinie-trace005" > "$P/evidence/build/binary.sha256"
