#!/usr/bin/env bash
# Private local extraction, not a system installation. Source: official Arm download page.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
mkdir -p "$P/.work" "$P/evidence"
URL=https://artifacts.tools.arm.com/arm-performance-studio/2026.5/Arm_Performance_Studio_2026.5_linux_x86-64.tgz
curl --fail --location --retry 2 --max-time 300 --dump-header "$P/evidence/download-headers.txt" "$URL" -o "$P/.work/studio.tgz"
echo 'f9a776cdabdeb56a228c1d2f153eac6067a38a55d7330d6ac8de84824bcd6c3f  '"$P/.work/studio.tgz" | sha256sum -c - > "$P/evidence/archive-verification.txt"
tar -tzf "$P/.work/studio.tgz" > "$P/.work/archive-files.txt"
printf 'url=%s\nfetched_utc=%s\n' "$URL" "$(date -u +%FT%TZ)" > "$P/evidence/compiler-download.txt"
