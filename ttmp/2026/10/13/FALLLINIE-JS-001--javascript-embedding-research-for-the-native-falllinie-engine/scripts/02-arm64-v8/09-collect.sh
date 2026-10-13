#!/usr/bin/env bash
# Parent-only receipt retrieval; compare locally; credentials never recorded.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
[[ $# == 2 || $# == 3 ]] || exit 2
dest=$1; engine=$2; remote=${3:-/storage/apps/falllinie-js-probes}
[[ $dest =~ ^[A-Za-z0-9_][A-Za-z0-9_.@-]*$ ]] || exit 2
[[ $engine == bellard || $engine == ng || $engine == v8 ]] || exit 2
[[ $remote =~ ^/storage/apps/[A-Za-z0-9_/-]+$ && $remote != *..* ]] || exit 2
opts=(); if [[ -n ${PROBE_SSH_CONFIG:-} ]]; then opts=(-F "$PROBE_SSH_CONFIG"); fi
R="$P/.work/target-results"; mkdir -p "$R"; test ! -e "$R/$engine.log"
cp "$P/.work/host-results/"{node-oracle.json,workload-expected.json,native-oracle.json,fixture-sha256.txt} "$R/"
scp "${opts[@]}" "$dest:$remote/$engine/result.log" "$R/$engine.log"
scp "${opts[@]}" "$dest:$remote/$engine/preflight.log" "$R/$engine-preflight.log"
scp "${opts[@]}" "$dest:$remote/$engine/process-time.log" "$R/$engine-time.log"
python3 "$P/04-compare.py" "$R" --engines "$engine" | tee "$R/$engine-comparison.json"
echo 'Collected actual target receipts. Parent decides bounded evidence retention.'
