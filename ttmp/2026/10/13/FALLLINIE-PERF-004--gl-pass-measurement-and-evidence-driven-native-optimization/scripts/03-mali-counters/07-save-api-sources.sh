#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
S=$(cd "$P/../.." && pwd)/sources
LIB=$P/.work/libGPUCounters
cp "$LIB/LICENSE.md" "$S/libGPUCounters-LICENSE.txt"
cp "$LIB/hwcpipe/include/hwcpipe/sampler.hpp" "$S/libGPUCounters-sampler.hpp.txt"
cp "$LIB/backend/device/src/device/hwcnt/sampler/timestamp.hpp" "$S/libGPUCounters-timestamp.hpp.txt"
cp "$LIB/hwcpipe/src/hwcpipe/derived_functions.cpp" "$S/libGPUCounters-derived-functions.cpp.txt"
cp "$LIB/hwcpipe/src/hwcpipe/detail/counter_map/bifrost.cpp" "$S/libGPUCounters-bifrost-map.cpp.txt"
url=https://developer.arm.com/tools-and-software/mali-offline-compiler
printf '%s\n' "$url" >> "$S/research-urls.txt"
status=0
defuddle parse "$url" --md -o "$S/mali-offline-compiler-page.txt" > "$S/mali-offline-compiler-fetch.log" 2>&1 || status=$?
printf 'fetch_exit=%s\n' "$status" >> "$S/mali-offline-compiler-fetch.log"
sha256sum "$S"/libGPUCounters-*.txt "$S/mali-g52-counter-reference.html" "$S/gator-page.txt" > "$S/SHA256SUMS"
