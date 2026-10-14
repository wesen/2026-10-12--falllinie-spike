#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd); R=$(git -C "$P" rev-parse --show-toplevel)
C=$R/ttmp/2026/10/13/FALLLINIE-PERF-004--gl-pass-measurement-and-evidence-driven-native-optimization/scripts/03-mali-counters
STAMP=$(date -u +%Y%m%dT%H%M%SZ); REMOTE=/storage/apps/falllinie-gpu006-$STAMP; OUT=$P/evidence/target-$STAMP
mkdir -p "$OUT"
KEY=${SSH_IDENTITY_FILE:-$HOME/.ssh/id_ed25519}
SSH=(ssh -n -o BatchMode=yes -o IdentitiesOnly=yes -o StrictHostKeyChecking=yes -o ConnectTimeout=10 -i "$KEY" root@192.168.0.202)
SCP=(scp -o BatchMode=yes -o IdentitiesOnly=yes -o StrictHostKeyChecking=yes -o ConnectTimeout=10 -i "$KEY")
"${SSH[@]}" "set -e; if pgrep -af '[f]alllinie-(spike|trace005|pmu|ranges|gpu006)( |$)|[m]ali-counters( |$)|[p]erf (record|stat)'; then exit 2; fi; mkdir '$REMOTE'; mkdir '$REMOTE/evidence'"
"${SCP[@]}" "$R/build-device/falllinie-gpu006" "$C/.work/build-arm/mali-counters" "$P/02-target-case.sh" "root@192.168.0.202:$REMOTE/"
"${SSH[@]}" "chmod +x '$REMOTE/falllinie-gpu006' '$REMOTE/mali-counters'; sha256sum '$REMOTE/falllinie-gpu006' '$REMOTE/mali-counters'" > "$OUT/deployed.sha256"
sha256sum "$R/build-device/falllinie-gpu006" > "$OUT/local-binary.sha256"
[ "$(awk 'NR==1 {print $1}' "$OUT/local-binary.sha256")" = "$(awk 'NR==1 {print $1}' "$OUT/deployed.sha256")" ]
printf 'source=%s\nremote=%s\n' "$(git -C "$R" rev-parse HEAD)" "$REMOTE" > "$OUT/campaign.txt"
n=0
for spec in 384x288:2:0 256x192:1:0 256x192:2:0 384x288:1:0 512x384:1:0 512x384:2:0 384x288:2:0 384x288:2:60; do
 IFS=: read -r size views fps <<< "$spec";n=$((n+1));name=$(printf '%02d-%s-v%s-fps%s' "$n" "$size" "$views" "$fps");status=0
 "${SSH[@]}" "timeout --kill-after=5s 60s sh '$REMOTE/02-target-case.sh' '$REMOTE' '$REMOTE/evidence/$name' '$size' '$views' '$fps'" || status=$?
 "${SCP[@]}" -r "root@192.168.0.202:$REMOTE/evidence/$name" "$OUT/"
 echo "$name status=$status"; [ "$status" = 0 ] || exit "$status"
done
echo complete=true >> "$OUT/campaign.txt"; echo "$OUT"
