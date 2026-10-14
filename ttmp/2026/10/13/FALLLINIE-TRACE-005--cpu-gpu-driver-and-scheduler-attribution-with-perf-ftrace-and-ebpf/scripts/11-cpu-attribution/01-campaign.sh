#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
R=$(git -C "$P" rev-parse --show-toplevel)
B=$P/../10-cpu-pmu
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
REMOTE=/storage/apps/falllinie-attribution-$STAMP
OUT=$P/evidence/target-$STAMP
mkdir -p "$OUT"
: "${SSH_ASKPASS:?external helper required}"
export SSH_ASKPASS_REQUIRE=force DISPLAY=${DISPLAY:-:0}
SSH=(setsid -w ssh -n -o IdentitiesOnly=yes root@192.168.0.202)
SCP=(setsid -w scp -o IdentitiesOnly=yes)
"${SSH[@]}" "set -e; if pgrep -af '[f]alllinie-(spike|trace005|pmu)( |$)|[p]erf (record|stat)'; then exit 2; fi; mkdir '$REMOTE'; mkdir '$REMOTE/evidence'"
"${SCP[@]}" "$R/build-device/falllinie-pmu" "$B/.work/pmu-clock" "$B/03-target-case.sh" "root@192.168.0.202:$REMOTE/"
"${SSH[@]}" "chmod +x '$REMOTE/falllinie-pmu' '$REMOTE/pmu-clock'; sha256sum '$REMOTE/falllinie-pmu' /usr/lib/libmali.so.1" > "$OUT/deployed.sha256"
printf 'source=%s\nremote=%s\nruntime=323b095\n' "$(git -C "$R" rev-parse HEAD)" "$REMOTE" > "$OUT/campaign.txt"
n=0
for event in cpu_cycles l1d_cache_refill br_mis_pred_retired l1d_tlb_refill cpu_cycles; do
 n=$((n+1)); name=$(printf '%02d-%s' "$n" "$event"); status=0
 "${SSH[@]}" "SAMPLE_EVENT=armv8_cortex_a55/$event/ SAMPLE_FREQ=199 timeout --kill-after=5s 65s sh '$REMOTE/03-target-case.sh' '$REMOTE' '$name' sample" || status=$?
 mkdir -p "$OUT/$name"
 "${SSH[@]}" "cd '$REMOTE/evidence/$name' && tar --exclude=perf.data -cf - ." | tar -xf - -C "$OUT/$name"
 echo "$name status=$status"
 [ "$status" = 0 ] || exit "$status"
done
echo complete=true >> "$OUT/campaign.txt"
echo "$OUT"
