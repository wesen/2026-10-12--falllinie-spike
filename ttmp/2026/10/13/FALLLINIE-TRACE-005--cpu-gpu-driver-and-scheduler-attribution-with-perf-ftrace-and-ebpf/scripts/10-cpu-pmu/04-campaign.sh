#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
R=$(git -C "$P" rev-parse --show-toplevel)
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
REMOTE=/storage/apps/falllinie-pmu-$STAMP
OUT=$P/evidence/target-$STAMP
mkdir -p "$OUT" "$P/.work"
REL=${P#"$R/"}
docker run --rm --pull=never --network=none --user "$(id -u):$(id -g)" -v "$R:/work" -w /work falllinie-spike-toolchain aarch64-linux-gnu-g++ "$REL/clock.cpp" -o "$REL/.work/pmu-clock"
: "${SSH_ASKPASS:?external credential helper required}"
export SSH_ASKPASS_REQUIRE=force DISPLAY=${DISPLAY:-:0}
SSH=(setsid -w ssh -n -o IdentitiesOnly=yes -o ConnectTimeout=5 root@192.168.0.202)
SCP=(setsid -w scp -o IdentitiesOnly=yes -o ConnectTimeout=5)
"${SSH[@]}" "set -e; if pgrep -af '[f]alllinie-(spike|trace005|pmu)( |$)|[m]ali-counters( |$)|[p]erf (record|stat)'; then exit 2; fi; mkdir '$REMOTE'; mkdir '$REMOTE/evidence'"
"${SCP[@]}" "$R/build-device/falllinie-pmu" "$P/.work/pmu-clock" "$P/01-inventory.sh" "$P/03-target-case.sh" "root@192.168.0.202:$REMOTE/"
"${SSH[@]}" "chmod +x '$REMOTE/falllinie-pmu' '$REMOTE/pmu-clock'; sh '$REMOTE/01-inventory.sh'" > "$OUT/inventory.txt" 2>&1
printf 'remote=%s\nsource=%s\n' "$REMOTE" "$(git -C "$R" rev-parse HEAD)" > "$OUT/campaign.txt"
"${SSH[@]}" "sha256sum '$REMOTE/falllinie-pmu' '$REMOTE/pmu-clock'" > "$OUT/deployed.sha256"
n=0
for group in none cpu_cycles,inst_retired l1d_cache,l1d_cache_refill l2d_cache,l2d_cache_refill br_retired,br_mis_pred_retired l1d_tlb,l1d_tlb_refill sample none; do
 n=$((n+1)); name=$(printf '%02d-%s' "$n" "${group//,/-}")
 status=0
 "${SSH[@]}" "timeout --kill-after=5s 65s sh '$REMOTE/03-target-case.sh' '$REMOTE' '$name' '$group'" || status=$?
 # Do not download raw perf recordings into tracked evidence.
 "${SSH[@]}" "cd '$REMOTE/evidence/$name' && tar --exclude=perf.data -cf - ." | tar -xf - -C "$(mkdir -p "$OUT/$name"; echo "$OUT/$name")"
 echo "$name status=$status"
 [ "$status" = 0 ] || exit "$status"
done
echo complete=true >> "$OUT/campaign.txt"
echo "$OUT"
