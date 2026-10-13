#!/usr/bin/env bash
# No installation/device contact. Fetch pinned inputs separately with 00-fetch.py.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd); ROOT=$(git -C "$P" rev-parse --show-toplevel); W="$P/.work"
node "$P/01-extract.cjs"
make -C "$W/quickjs-2026-06-04" -j2 libquickjs.a > "$W/bellard-host-build.log" 2>&1
cmake -S "$W/quickjs-ng" -B "$W/ng-host" -DCMAKE_BUILD_TYPE=Release -DQJS_BUILD_LIBC=OFF > "$W/ng-host-config.log" 2>&1
cmake --build "$W/ng-host" --target qjs -j2 > "$W/ng-host-build.log" 2>&1
mkdir -p "$W/host"
for engine in bellard ng; do
 if [[ $engine == bellard ]]; then INC="$W/quickjs-2026-06-04"; LIB="$INC/libquickjs.a"; else INC="$W/quickjs-ng"; LIB="$W/ng-host/libqjs.a"; fi
 c++ -std=c++17 -O2 -fno-fast-math -I"$INC" "$P/quickjs-probe.cpp" "$LIB" -lm -ldl -lpthread -o "$W/host/$engine-probe"
done
mkdir -p "$W/host/lib"
# Host GCC13 requires host libstdc++; only ARM GCC12 uses bundled Debian libstdc++.
find "$W/debian-amd64" -type f -name '*.so.*' ! -name 'libstdc++*' ! -name 'libgcc_s*' -exec cp -a '{}' "$W/host/lib/" ';'
find "$W/debian-amd64" -type l -name '*.so.*' ! -name 'libstdc++*' ! -name 'libgcc_s*' -exec cp -a '{}' "$W/host/lib/" ';'
L="$W/host/lib"
c++ -std=c++17 -O2 -fno-fast-math -I"$W/debian-amd64/usr/include/node" "$P/v8-probe.cpp" "$L/libnode.so.108" -Wl,-rpath,"$L" -Wl,-rpath-link,"$L" -pthread -ldl -o "$W/host/v8-probe"
OLD="$ROOT/scripts/2026/10/13/falllinie-js-runtime"
c++ -std=c++17 -O2 -fno-fast-math -I"$ROOT/src" "$OLD/native-oracle.cpp" "$ROOT"/src/{game,world,terrain,particles}.cpp -o "$W/host/native-oracle"
c++ -std=c++17 -O2 -fno-fast-math -I"$ROOT/src" "$OLD/native-bench.cpp" "$ROOT/src/terrain.cpp" -o "$W/host/native-bench"
{ c++ --version | head -1; echo 'harness_flags=-std=c++17 -O2 -fno-fast-math'; echo 'Bellard upstream make recipe (dry run only):'; make -Bn -C "$W/quickjs-2026-06-04" libquickjs.a; echo 'NG host actual C flags:'; cat "$W/ng-host/CMakeFiles/qjs.dir/flags.make"; uname -sm; lscpu; echo 'V8 direct and transitive host dependencies:'; LD_LIBRARY_PATH="$L" ldd "$W/host/v8-probe"; } > "$W/host-build-provenance.log"
echo 'HOST BUILD PASS'
