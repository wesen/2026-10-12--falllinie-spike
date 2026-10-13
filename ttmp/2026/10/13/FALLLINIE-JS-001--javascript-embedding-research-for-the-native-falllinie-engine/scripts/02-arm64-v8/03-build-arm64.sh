#!/usr/bin/env bash
# Cross-build only, no QEMU/device access. Image has GCC12; no installs/network.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd); ROOT=$(git -C "$P" rev-parse --show-toplevel); REL=${P#"$ROOT/"}
IMAGE=sha256:8068d5a56ed677c85ffd8e02f9ee4e88c9804d56d070aead0d598c44eb0f83dd
docker run --rm --network none --user "$(id -u):$(id -g)" -v "$ROOT:/work" -w /work -e "PROBE_REL=$REL" "$IMAGE" bash -euo pipefail -c '
 P="/work/$PROBE_REL"; W="$P/.work"; mkdir -p "$W/arm64/bellard"
 aarch64-linux-gnu-g++ --version | head -1
 echo "harness_flags=-std=c++17 -O2 -fno-fast-math -rpath=\$ORIGIN/lib"
 echo "Bellard C flags=-O2 -fwrapv -D_GNU_SOURCE -DCONFIG_VERSION=2026-06-04"
 for src in quickjs dtoa libregexp libunicode cutils; do
  aarch64-linux-gnu-gcc -O2 -fwrapv -D_GNU_SOURCE -DCONFIG_VERSION=\"2026-06-04\" -I"$W/quickjs-2026-06-04" -c "$W/quickjs-2026-06-04/$src.c" -o "$W/arm64/bellard/$src.o"
 done
 aarch64-linux-gnu-ar rcs "$W/arm64/bellard/libquickjs.a" "$W/arm64/bellard/"*.o
 cmake -S "$W/quickjs-ng" -B "$W/ng-arm64" -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_BUILD_TYPE=Release -DQJS_BUILD_LIBC=OFF > "$W/ng-arm64-config.log" 2>&1
 cmake --build "$W/ng-arm64" --target qjs -j2 > "$W/ng-arm64-build.log" 2>&1
 echo "NG ARM64 actual C flags:"; cat "$W/ng-arm64/CMakeFiles/qjs.dir/flags.make"
 for engine in bellard ng; do
  if [[ $engine == bellard ]]; then INC="$W/quickjs-2026-06-04"; LIB="$W/arm64/bellard/libquickjs.a"; else INC="$W/quickjs-ng"; LIB="$W/ng-arm64/libqjs.a"; fi
  aarch64-linux-gnu-g++ -std=c++17 -O2 -fno-fast-math -I"$INC" "$P/quickjs-probe.cpp" "$LIB" -lm -ldl -pthread -Wl,-rpath,\$ORIGIN/lib -o "$W/arm64/$engine-probe"
 done
 L="$W/debian-arm64/usr/lib/aarch64-linux-gnu"
 aarch64-linux-gnu-g++ -std=c++17 -O2 -fno-fast-math -I"$W/debian-arm64/usr/include/node" "$P/v8-probe.cpp" "$L/libnode.so.108" -Wl,-rpath-link,"$L" -Wl,-rpath-link,"$W/debian-arm64/lib/aarch64-linux-gnu" -Wl,-rpath,\$ORIGIN/lib -pthread -ldl -o "$W/arm64/v8-probe"
 for engine in bellard ng v8; do
  exe="$W/arm64/$engine-probe"; echo "==== $engine ===="
  aarch64-linux-gnu-readelf -h -l -d "$exe"
  aarch64-linux-gnu-readelf --version-info "$exe"
  aarch64-linux-gnu-size "$exe"
 done
' > "$P/.work/arm64-cross-link.log" 2>&1
cat "$P/.work/arm64-cross-link.log"
