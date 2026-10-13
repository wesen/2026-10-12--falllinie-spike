#!/usr/bin/env bash
# Cross-compilation ONLY. Does not build images, install packages, contact device or run ARM binaries.
set -euo pipefail
cd "$(dirname "$0")/../../../../.."
IMAGE=sha256:8068d5a56ed677c85ffd8e02f9ee4e88c9804d56d070aead0d598c44eb0f83dd
docker run --rm --network none --user "$(id -u):$(id -g)" -v "$PWD:/work" "$IMAGE" bash -euo pipefail -c '
 P=scripts/2026/10/13/falllinie-js-runtime; W="$P/.work"
 cmake -S "$W/quickjs-ng" -B "$W/ng-arm" -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_BUILD_TYPE=Release > "$W/arm-config.log" 2>&1
 cmake --build "$W/ng-arm" --target qjs -j4 > "$W/arm-build.log" 2>&1
 aarch64-linux-gnu-g++ -std=c++17 -O2 -I"$W/quickjs-ng" "$P/host.cpp" "$W/ng-arm/libqjs.a" -lm -ldl -lpthread -o "$W/host-ng-arm"
 mkdir -p "$W/bellard-arm"
 for src in quickjs dtoa libregexp libunicode cutils; do
  aarch64-linux-gnu-gcc -O2 -fwrapv -D_GNU_SOURCE -DCONFIG_VERSION=\"2026-06-04\" -I"$W/quickjs-2026-06-04" -c "$W/quickjs-2026-06-04/$src.c" -o "$W/bellard-arm/$src.o"
 done
 aarch64-linux-gnu-ar rcs "$W/bellard-arm/libquickjs.a" "$W/bellard-arm/"*.o
 aarch64-linux-gnu-g++ -std=c++17 -O2 -I"$W/quickjs-2026-06-04" "$P/host.cpp" "$W/bellard-arm/libquickjs.a" -lm -ldl -lpthread -o "$W/host-bellard-arm"
 aarch64-linux-gnu-g++ --version | head -1
 for exe in "$W/host-bellard-arm" "$W/host-ng-arm"; do aarch64-linux-gnu-readelf -h -l -d "$exe"; aarch64-linux-gnu-size "$exe"; done
' > scripts/2026/10/13/falllinie-js-runtime/.work/cross-link.txt 2>&1
cat scripts/2026/10/13/falllinie-js-runtime/.work/cross-link.txt
