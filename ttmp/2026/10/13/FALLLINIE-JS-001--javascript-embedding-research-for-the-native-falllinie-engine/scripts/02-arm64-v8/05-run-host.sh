#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd); W="$P/.work"
tag=${1:-host-results}; [[ $tag =~ ^[A-Za-z0-9_-]+$ ]] || exit 2
R="$W/$tag"; mkdir -p "$R"
cp "$W"/{node-oracle.json,workload-expected.json,fixture-sha256.txt} "$R/"
{ date -u +%FT%TZ; uname -sm; lscpu; awk '/^model name|^cpu MHz/ {print; if(++n==16)exit}' /proc/cpuinfo; echo 'sequential=true timeout_per_probe_seconds=120 external_process_elapsed=/usr/bin/time'; } > "$R/environment.log"
for engine in bellard ng v8; do
 # One process at a time; output is private until deliberately retained as evidence.
 LD_LIBRARY_PATH="$W/host/lib" /usr/bin/time -f 'process_elapsed_s=%e max_rss_kib=%M exit=%x includes_platform_and_teardown=true' -o "$R/$engine-time.log" timeout 120 "$W/host/$engine-probe" "$W/pure.js" "$W/probe.js" 300 > "$R/$engine.log" 2>&1
 cat "$R/$engine-time.log"
done
timeout 30 "$W/host/native-oracle" > "$R/native-oracle.json"
timeout 30 "$W/host/native-bench" > "$R/native-bench.log"
python3 "$P/04-compare.py" "$R" | tee "$R/comparison.json"
