#!/usr/bin/env bash
# Requires previously downloaded pins, cc/c++/cmake/make/Node, SDL headers; no installs.
set -euo pipefail
cd "$(dirname "$0")/../../../../.."
P=scripts/2026/10/13/falllinie-js-runtime
W="$P/.work"
node "$P/02-extract-source.cjs"
cmake -S "$W/quickjs-ng" -B "$W/ng-build" -DCMAKE_BUILD_TYPE=Release > "$W/ng-config.log" 2>&1
cmake --build "$W/ng-build" -j4 > "$W/ng-build.log" 2>&1
make -C "$W/quickjs-2026-06-04" -j4 libquickjs.a > "$W/bellard-build.log" 2>&1
for engine in bellard ng; do
 if [[ $engine == bellard ]]; then INC="$W/quickjs-2026-06-04"; LIB="$INC/libquickjs.a"; else INC="$W/quickjs-ng"; LIB="$W/ng-build/libqjs.a"; fi
 c++ -std=c++17 -O2 -I"$INC" "$P/host.cpp" "$LIB" -lm -ldl -lpthread -o "$W/host-$engine"
 "$W/host-$engine" "$W/pure.js" "$P/probe.js" "$W/inline.js" > "$W/$engine-results.txt"
done
c++ -std=c++17 -O2 -Isrc "$P/native-oracle.cpp" src/game.cpp src/world.cpp src/terrain.cpp src/particles.cpp -o "$W/native-oracle"
"$W/native-oracle" > "$W/native-oracle.json"
cc -O2 -I"$W/duktape-2.7.0/src" "$P/duktape-probe.c" "$W/duktape-2.7.0/src/duktape.c" -lm -o "$W/duktape-probe"
"$W/duktape-probe" > "$W/duktape-results.txt"
c++ -std=c++17 -O2 -Isrc "$P/native-bench.cpp" src/terrain.cpp -o "$W/native-bench"
"$W/native-bench" > "$W/native-bench.txt"
python3 "$P/04-compare.py"
size "$W/host-bellard" "$W/host-ng" "$W/duktape-probe" > "$W/size.txt"
cat "$W/bellard-results.txt" "$W/ng-results.txt" "$W/duktape-results.txt" "$W/native-bench.txt" "$W/size.txt"
