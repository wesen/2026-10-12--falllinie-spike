#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
S=$(cd "$P/../.." && pwd)/sources
mkdir -p "$P/.work" "$S"
git clone --depth 1 https://github.com/ARM-software/libGPUCounters.git "$P/.work/libGPUCounters"
REV=$(git -C "$P/.work/libGPUCounters" rev-parse HEAD)
printf 'url=https://github.com/ARM-software/libGPUCounters\nrevision=%s\nfetched_utc=%s\n' "$REV" "$(date -u +%FT%TZ)" > "$S/libGPUCounters-receipt.txt"
# Preserve primary documentation bytes. Full downloaded implementation stays ignored.
cp "$P/.work/libGPUCounters/README.md" "$S/libGPUCounters-README.txt"
cp "$P/.work/libGPUCounters/docs/mali-g52_counter_reference.html" "$S/mali-g52-counter-reference.html"
defuddle parse "https://github.com/ARM-software/libGPUCounters" --md -o "$S/libGPUCounters-page.txt"
defuddle parse "https://github.com/ARM-software/gator" --md -o "$S/gator-page.txt"
printf 'https://github.com/ARM-software/libGPUCounters\nhttps://github.com/ARM-software/gator\nhttps://github.com/ARM-software/libGPUCounters/blob/%s/docs/mali-g52_counter_reference.html\n' "$REV" > "$S/research-urls.txt"
sha256sum "$S/libGPUCounters-README.txt" "$S/mali-g52-counter-reference.html" "$S/libGPUCounters-page.txt" "$S/gator-page.txt" > "$S/SHA256SUMS"
