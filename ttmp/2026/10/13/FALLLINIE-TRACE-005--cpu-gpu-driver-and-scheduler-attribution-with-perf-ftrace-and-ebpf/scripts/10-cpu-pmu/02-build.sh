#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
R=$(git -C "$P" rev-parse --show-toplevel)
mkdir -p "$P/evidence/build" "$R/build-device"
cmake --build "$R/build/integrated-source" -j4 > "$P/evidence/build/host-build.txt" 2>&1
ctest --test-dir "$R/build/integrated-source" --output-on-failure > "$P/evidence/build/cpu-tests.txt" 2>&1
REL=${P#"$R/"}/evidence/build
git -C "$R" rev-parse HEAD > "$P/evidence/build/source-revision.txt"
sha256sum "$R"/src/* > "$P/evidence/build/source-hashes.txt"
docker image inspect --format '{{.Id}}' falllinie-spike-toolchain > "$P/evidence/build/image.txt"
docker run --rm --pull=never --network=none --user "$(id -u):$(id -g)" -v "$R:/work" -w /work -e RECEIPTS="$REL" falllinie-spike-toolchain bash -euo pipefail -c '
 aarch64-linux-gnu-g++ --version > "$RECEIPTS/compiler.txt"
 aarch64-linux-gnu-g++ -std=c++17 -O2 -g -fno-omit-frame-pointer -Wall -Wextra -pthread -I/usr/include/SDL2 -I/usr/include/aarch64-linux-gnu src/*.cpp -o build-device/falllinie-pmu -Ltarget-libs -Wl,--allow-shlib-undefined -Wl,--build-id -l:libSDL2-2.0.so.0
 aarch64-linux-gnu-readelf -h -n build-device/falllinie-pmu > "$RECEIPTS/elf.txt"
' > "$P/evidence/build/arm-build.txt" 2>&1
sha256sum "$R/build-device/falllinie-pmu" > "$P/evidence/build/binary.sha256"
