#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
R=$(git -C "$P" rev-parse --show-toplevel)
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
REMOTE=/storage/apps/falllinie-mali-$STAMP
OUT=$P/evidence/target-$STAMP
mkdir "$OUT"
: "${SSH_ASKPASS:?external askpass required}"
export SSH_ASKPASS_REQUIRE=force DISPLAY=${DISPLAY:-:0}
SSH=(setsid -w ssh -n -o IdentitiesOnly=yes -o ConnectTimeout=5 root@192.168.0.202)
SCP=(setsid -w scp -o IdentitiesOnly=yes -o ConnectTimeout=5)
"${SSH[@]}" "set -e; if pgrep -af '[f]alllinie-(spike|perf004|trace005)( |$)|[g]atord|[m]ali-counters( |$)'; then exit 2; fi; mkdir '$REMOTE'; mkdir '$REMOTE/evidence'"
"${SCP[@]}" "$P/.work/build-arm/mali-counters" "$R/build-device/falllinie-trace005" "$P/04-target-case.sh" "root@192.168.0.202:$REMOTE/"
"${SSH[@]}" "chmod +x '$REMOTE/mali-counters' '$REMOTE/falllinie-trace005'; sha256sum '$REMOTE/mali-counters' '$REMOTE/falllinie-trace005'" > "$OUT/remote-sha256.txt"
printf 'remote=%s\nsource=%s\n' "$REMOTE" "$(git -C "$R" rev-parse HEAD)" > "$OUT/campaign.txt"
i=0
for case in idle baseline live bypass baseline; do
 i=$((i+1)); name=$(printf '%02d-%s' "$i" "$case")
 status=0
 "${SSH[@]}" "timeout --kill-after=5s 70s sh '$REMOTE/04-target-case.sh' '$REMOTE/mali-counters' '$REMOTE/falllinie-trace005' '$REMOTE/evidence/$name' '$case'" || status=$?
 "${SCP[@]}" -r "root@192.168.0.202:$REMOTE/evidence/$name" "$OUT/"
 echo "$name status=$status"
 [ "$status" = 0 ] || exit "$status"
done
printf 'complete=true\n' >> "$OUT/campaign.txt"
echo "$OUT"
