#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd); R=$(git -C "$P" rev-parse --show-toplevel)
C=$R/ttmp/2026/10/13/FALLLINIE-PERF-004--gl-pass-measurement-and-evidence-driven-native-optimization/scripts/03-mali-counters
STAMP=$(date -u +%Y%m%dT%H%M%SZ);REMOTE=/storage/apps/falllinie-gpu007-$STAMP;OUT=$P/evidence/target-$STAMP
mkdir -p "$OUT"
KEY=${SSH_IDENTITY_FILE:-$HOME/.ssh/id_ed25519}
SSH=(ssh -n -o BatchMode=yes -o IdentitiesOnly=yes -o StrictHostKeyChecking=yes -o ConnectTimeout=10 -i "$KEY" root@192.168.0.202)
SCP=(scp -o BatchMode=yes -o IdentitiesOnly=yes -o StrictHostKeyChecking=yes -o ConnectTimeout=10 -i "$KEY")
"${SSH[@]}" "set -e; if pgrep -af '[f]alllinie-(spike|trace005|pmu|ranges|gpu006)( |$)|[m]ali-counters( |$)|[p]erf (record|stat)'; then exit 2; fi; mkdir '$REMOTE'; mkdir '$REMOTE/evidence'"
"${SCP[@]}" -r "$R/build-device/falllinie-gpu006" "$C/.work/build-arm/mali-counters" "$P/03-target-case.sh" "$P/inputs" "root@192.168.0.202:$REMOTE/"
"${SSH[@]}" "chmod +x '$REMOTE/falllinie-gpu006' '$REMOTE/mali-counters'; sha256sum '$REMOTE/falllinie-gpu006' '$REMOTE/mali-counters'" > "$OUT/deployed.sha256"
sha256sum "$R/build-device/falllinie-gpu006" > "$OUT/local-binary.sha256"
[ "$(awk 'NR==1 {print $1}' "$OUT/deployed.sha256")" = '90f124a7745a37f76175130bd79ef748b741852bccfe58305548f7946c0d82ed' ]
"${SSH[@]}" "cd '$REMOTE' && sha256sum -c inputs/manifest.sha256" > "$OUT/assets-before.txt"
cp "$P/inputs/manifest.json" "$OUT/asset-manifest.json"
printf 'tooling_source=%s\nruntime_source=32b8bfd640f44c465d3f8b6311400f35eac07e81\nremote=%s\n' "$(git -C "$R" rev-parse HEAD)" "$REMOTE" > "$OUT/campaign.txt"
n=0
for mode in baseline post-copy scene-flat both both scene-flat post-copy baseline; do
 n=$((n+1));name=$(printf '%02d-%s' "$n" "$mode");status=0
 "${SSH[@]}" "timeout --kill-after=5s 60s sh '$REMOTE/03-target-case.sh' '$REMOTE' '$REMOTE/evidence/$name' '$mode'" || status=$?
 "${SCP[@]}" -r "root@192.168.0.202:$REMOTE/evidence/$name" "$OUT/"
 echo "$name status=$status";[ "$status" = 0 ] || exit "$status"
done
"${SSH[@]}" "cd '$REMOTE' && sha256sum -c inputs/manifest.sha256" > "$OUT/assets-after.txt"
echo complete=true >> "$OUT/campaign.txt";echo "$OUT"
