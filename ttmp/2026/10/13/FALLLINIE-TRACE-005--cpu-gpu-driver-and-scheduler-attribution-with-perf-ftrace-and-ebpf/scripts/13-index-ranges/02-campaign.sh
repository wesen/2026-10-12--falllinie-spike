#!/usr/bin/env bash
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd); R=$(git -C "$P" rev-parse --show-toplevel); B=$P/../10-cpu-pmu
STAMP=$(date -u +%Y%m%dT%H%M%SZ); REMOTE=/storage/apps/falllinie-ranges-$STAMP; OUT=$P/evidence/target-$STAMP
mkdir -p "$OUT"
: "${SSH_ASKPASS:?external helper required}"
export SSH_ASKPASS_REQUIRE=force DISPLAY=${DISPLAY:-:0}
SSH=(setsid -w ssh -n -o IdentitiesOnly=yes root@192.168.0.202); SCP=(setsid -w scp -o IdentitiesOnly=yes)
"${SSH[@]}" "set -e; if pgrep -af '[f]alllinie-(spike|trace005|pmu|ranges)( |$)|[p]erf (record|stat)'; then exit 2; fi; mkdir '$REMOTE'; mkdir '$REMOTE/evidence'"
"${SCP[@]}" "$R/build-device/falllinie-ranges" "$B/.work/pmu-clock" "$B/03-target-case.sh" "root@192.168.0.202:$REMOTE/"
"${SSH[@]}" "chmod +x '$REMOTE/falllinie-ranges' '$REMOTE/pmu-clock'; sha256sum '$REMOTE/falllinie-ranges' /usr/lib/libmali.so.1" > "$OUT/deployed.sha256"
printf 'remote=%s\nsource=%s\norder=baseline,ranges,ranges,baseline\n' "$REMOTE" "$(git -C "$R" rev-parse HEAD)" > "$OUT/campaign.txt"
for mode in baseline ranges; do
 arg=; [ "$mode" = baseline ] || arg=--draw-ranges
 "${SSH[@]}" "export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SDL_VIDEODRIVER=wayland; timeout --kill-after=5s 30s '$REMOTE/falllinie-ranges' --assets /storage/apps/falllinie-spike/assets/shaders --no-audio --freeze-scene --frames 1 --cells 7 --seed 42 --no-vsync --render-size 384x288 $arg --capture-dir '$REMOTE/evidence/capture-$mode' --receipt '$REMOTE/evidence/capture-$mode.json' > '$REMOTE/evidence/capture-$mode.log' 2>&1"
 "${SCP[@]}" -r "root@192.168.0.202:$REMOTE/evidence/capture-$mode" "$OUT/"
 "${SCP[@]}" "root@192.168.0.202:$REMOTE/evidence/capture-$mode.json" "root@192.168.0.202:$REMOTE/evidence/capture-$mode.log" "$OUT/"
done
n=0
for mode in 0 1 1 0; do
 n=$((n+1)); name=$(printf '%02d-range%s' "$n" "$mode"); status=0
 "${SSH[@]}" "GAME_BIN='$REMOTE/falllinie-ranges' DRAW_RANGES=$mode timeout --kill-after=5s 65s sh '$REMOTE/03-target-case.sh' '$REMOTE' '$name' sample" || status=$?
 mkdir -p "$OUT/$name"
 "${SSH[@]}" "cd '$REMOTE/evidence/$name' && tar --exclude=perf.data -cf - ." | tar -xf - -C "$OUT/$name"
 echo "$name status=$status"; [ "$status" = 0 ] || exit "$status"
done
echo complete=true >> "$OUT/campaign.txt"; echo "$OUT"
