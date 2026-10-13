#!/usr/bin/env bash
# CPU/static checks + sanitized adapters. Third-party libraries NOT instrumented.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd); ROOT=$(git -C "$P" rev-parse --show-toplevel); W="$P/.work"
for script in "$P"/*.sh; do bash -n "$script"; done
shellcheck "$P"/*.sh
node --check "$P/01-extract.cjs"; node --check "$P/workloads.js"
python3 -m py_compile "$P"/*.py
python3 "$P/test_compare.py"
cmake -S "$ROOT" -B "$W/native-tests" -DCMAKE_BUILD_TYPE=Release > "$W/native-config.log" 2>&1
cmake --build "$W/native-tests" -j2 > "$W/native-build.log" 2>&1
ctest --test-dir "$W/native-tests" --output-on-failure > "$W/native-tests.log" 2>&1
mkdir -p "$W/sanitized"
flags=(-std=c++17 -O1 -g -fno-fast-math -fno-omit-frame-pointer "-fsanitize=address,undefined")
for engine in bellard ng; do
 if [[ $engine == bellard ]]; then INC="$W/quickjs-2026-06-04"; LIB="$INC/libquickjs.a"; else INC="$W/quickjs-ng"; LIB="$W/ng-host/libqjs.a"; fi
 c++ "${flags[@]}" -I"$INC" "$P/quickjs-probe.cpp" "$LIB" -lm -ldl -pthread -o "$W/sanitized/$engine-probe"
done
L="$W/host/lib"
# Debian V8 has no exported C++ RTTI; UBSan vptr requires unavailable typeinfo.
c++ "${flags[@]}" -fno-sanitize=vptr -I"$W/debian-amd64/usr/include/node" "$P/v8-probe.cpp" "$L/libnode.so.108" -Wl,-rpath,"$L" -Wl,-rpath-link,"$L" -pthread -ldl -o "$W/sanitized/v8-probe"
for engine in bellard ng v8; do
 ASAN_OPTIONS=detect_leaks=1:quarantine_size_mb=16:thread_local_quarantine_size_kb=256 UBSAN_OPTIONS=halt_on_error=1 LD_LIBRARY_PATH="$L" timeout 120 "$W/sanitized/$engine-probe" "$W/pure.js" "$W/probe.js" 20 > "$W/sanitized/$engine.log" 2>&1
 grep '^PROBE PASS$' "$W/sanitized/$engine.log"
done
set +e
bash "$P/run-target.sh" > "$W/runner-host-refusal.log" 2>&1; rc=$?
set -e
[[ $rc == 2 ]] && grep -q 'target must be aarch64' "$W/runner-host-refusal.log"
echo 'VALIDATION PASS: static, comparator, native CPU, sanitized adapters, host runner refusal'
