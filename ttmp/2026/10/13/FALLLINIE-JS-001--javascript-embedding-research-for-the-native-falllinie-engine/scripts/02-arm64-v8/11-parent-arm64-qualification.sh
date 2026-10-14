#!/usr/bin/env bash
# Parent-only after review. Reuse verified private packages from an isolated build.
# Credentials are supplied externally. Never stops games/services or installs libraries.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
[[ $# == 1 ]] || { echo 'usage: 11-parent-arm64-qualification.sh ARTIFACT_SCRIPT_ROOT' >&2; exit 2; }
A=$(cd -- "$1" && pwd)
: "${SSH_ASKPASS:?existing external askpass required}"
export SSH_ASKPASS_REQUIRE=force DISPLAY=${DISPLAY:-:0}
mkdir -p "$P/evidence/target-corrected-v2"
python3 - "$A" "$P" <<'PY'
import hashlib,json,pathlib,sys
artifact,current=map(pathlib.Path,sys.argv[1:])
expected={'bellard':'4a00559d1299eac38310fea8aa27ed809356366c5bba9b31aa3c0e747a84514b','ng':'bf6416eb3933e5385ab2042d034b64a94b711c446ab1353da96426ed47713f85','v8':'df7d4c3b1c9ac9d8231703663c234019c518b9f9093ad4769c91e1df1aa94a87'}
for engine,digest in expected.items():
    archive=artifact/'.work/packages'/f'{engine}-arm64.tar.gz'
    assert hashlib.sha256(archive.read_bytes()).hexdigest()==digest,archive
    manifest=json.loads((artifact/'.work/packages'/engine/'manifest.json').read_text())
    for name,sha in manifest['harness_source_sha256'].items():
        assert hashlib.sha256((current/name).read_bytes()).hexdigest()==sha,name
    print(engine,'archive and current harness hashes PASS')
PY
for engine in bellard ng v8; do
 (cd "$A/.work/packages/$engine"; sha256sum -c SHA256SUMS >/dev/null)
 setsid -w bash "$A/08-deploy.sh" root@192.168.0.202 "$engine" > "$P/evidence/target-corrected-v2/$engine-transfer.log" 2>&1
 set +e
 setsid -w ssh -n -o ConnectTimeout=5 -o ServerAliveInterval=10 -o ServerAliveCountMax=2 -o IdentitiesOnly=yes root@192.168.0.202 "bash /storage/apps/falllinie-js-probes/$engine/run-target.sh 300" > "$P/evidence/target-corrected-v2/$engine-invocation.log" 2>&1
 rc=$?
 set -e
 cat "$P/evidence/target-corrected-v2/$engine-invocation.log"
 if ((rc!=0)); then echo "STOP: $engine target invocation failed rc=$rc; preserve remote receipts" >&2; exit "$rc"; fi
 setsid -w bash "$A/09-collect.sh" root@192.168.0.202 "$engine"
 cp "$A/.work/target-results/$engine"{.log,-preflight.log,-time.log,-comparison.json} "$P/evidence/target-corrected-v2/"
done
cp "$A/.work/target-results/"{node-oracle.json,workload-expected.json,native-oracle.json,fixture-sha256.txt} "$P/evidence/target-corrected-v2/"
python3 "$P/04-compare.py" "$P/evidence/target-corrected-v2" | tee "$P/evidence/target-corrected-v2/comparison.json"
