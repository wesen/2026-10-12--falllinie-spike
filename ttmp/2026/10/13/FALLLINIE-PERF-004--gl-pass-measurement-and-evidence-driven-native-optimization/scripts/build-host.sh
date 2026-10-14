#!/bin/sh
# No downloads, packages or target libraries. All outputs ticket-local.
set -eu
S=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$S/../../../../../.." && pwd)
mkdir -p "$S/.work"
cmake -S "$ROOT" -B "$S/.work/build" -DCMAKE_BUILD_TYPE=Release > "$S/.work/configure.log" 2>&1
cmake --build "$S/.work/build" -j "${JOBS:-4}" > "$S/.work/build.log" 2>&1
ctest --test-dir "$S/.work/build" --output-on-failure > "$S/.work/ctest.log" 2>&1
python3 "$S/test_analyze.py" > "$S/.work/analyzer-tests.log" 2>&1
cat "$S/.work/ctest.log" "$S/.work/analyzer-tests.log"
