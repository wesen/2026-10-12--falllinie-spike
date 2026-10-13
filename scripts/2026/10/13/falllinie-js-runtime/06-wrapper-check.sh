#!/usr/bin/env bash
# Expected compatibility diagnostics; do not silently replace wrapper's bundled engine.
set -euo pipefail
cd "$(dirname "$0")/../../../../.."
P=scripts/2026/10/13/falllinie-js-runtime; W=$P/.work
mkdir -p "$W/wrapper-current"
# Explicit include-path adaptation ONLY in ignored copy; original remains unchanged.
sed 's@#include "quickjs/quickjs.h"@#include <quickjs.h>@' "$W/quickjspp/quickjspp.hpp" > "$W/wrapper-current/quickjspp.hpp"
for engine in bellard ng; do
 if [[ $engine == bellard ]]; then INC=$W/quickjs-2026-06-04; LIB=$INC/libquickjs.a; else INC=$W/quickjs-ng; LIB=$W/ng-build/libqjs.a; fi
 set +e
 c++ -std=c++17 -O2 -I"$INC" -I"$W/quickjspp" "$P/quickjspp-smoke.cpp" "$LIB" -lm -ldl -lpthread -o "$W/wrapper-$engine" > "$W/wrapper-bundled-$engine.txt" 2>&1
 bundled=$?
 c++ -std=c++17 -O2 -I"$INC" -I"$W/wrapper-current" "$P/quickjspp-smoke.cpp" "$LIB" -lm -ldl -lpthread -o "$W/wrapper-current-$engine" > "$W/wrapper-current-$engine.txt" 2>&1
 adapted=$?
 set -e
 printf '%s bundled_header_rc=%s adapted_include_rc=%s\n' "$engine" "$bundled" "$adapted"
 if [[ $adapted == 0 ]]; then "$W/wrapper-current-$engine"; fi
done
