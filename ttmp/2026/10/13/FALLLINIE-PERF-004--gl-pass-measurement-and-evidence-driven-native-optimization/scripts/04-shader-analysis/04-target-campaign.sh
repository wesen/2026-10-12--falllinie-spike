#!/usr/bin/env bash
# Same measured game binary and counter probe; only the selected post asset differs.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
R=$(git -C "$P" rev-parse --show-toplevel)
C=$P/../03-mali-counters
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
REMOTE=/storage/apps/falllinie-post-$STAMP
OUT=$P/evidence/target-$STAMP
mkdir "$OUT"
: "${SSH_ASKPASS:?external credentials required}"
export SSH_ASKPASS_REQUIRE=force DISPLAY=${DISPLAY:-:0}
SSH=(setsid -w ssh -n -o IdentitiesOnly=yes -o ConnectTimeout=5 root@192.168.0.202)
SCP=(setsid -w scp -o IdentitiesOnly=yes -o ConnectTimeout=5)
"${SSH[@]}" "set -e; if pgrep -af '[f]alllinie-(spike|trace005)( |$)|[g]atord|[m]ali-counters( |$)'; then exit 2; fi; mkdir '$REMOTE'; mkdir '$REMOTE/evidence'"
"${SCP[@]}" "$R/build-device/falllinie-trace005" "$C/.work/build-arm/mali-counters" "$C/04-target-case.sh" "$R/assets/hud-font.bmp" "$R/assets/glyphs.bmp" "root@192.168.0.202:$REMOTE/"
"${SCP[@]}" -r "$P/inputs/assets/baseline" "$P/inputs/assets/combined" "root@192.168.0.202:$REMOTE/"
"${SSH[@]}" "chmod +x '$REMOTE/falllinie-trace005' '$REMOTE/mali-counters'; sha256sum '$REMOTE/falllinie-trace005' '$REMOTE/mali-counters' '$REMOTE/baseline/post_fs.glsl' '$REMOTE/combined/post_fs.glsl'" > "$OUT/deployed.sha256"
printf 'remote=%s\nsource=%s\ngame_runtime=209a6d0\norder=baseline,combined,combined,baseline\n' "$REMOTE" "$(git -C "$R" rev-parse HEAD)" > "$OUT/campaign.txt"
# Identical static fixture, seven cells to include terrain beyond full-fog depth.
for variant in baseline combined; do
 "${SSH[@]}" "export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SDL_VIDEODRIVER=wayland; timeout --kill-after=5s 30s '$REMOTE/falllinie-trace005' --assets '$REMOTE/$variant' --freeze-scene --frames 1 --cells 7 --seed 42 --no-vsync --render-size 384x288 --capture-dir '$REMOTE/evidence/capture-$variant' > '$REMOTE/evidence/capture-$variant.log' 2>&1"
 "${SCP[@]}" -r "root@192.168.0.202:$REMOTE/evidence/capture-$variant" "$OUT/"
 "${SCP[@]}" "root@192.168.0.202:$REMOTE/evidence/capture-$variant.log" "$OUT/"
done
i=0
for variant in baseline combined combined baseline; do
 i=$((i+1)); name=$(printf '%02d-%s' "$i" "$variant")
 status=0
 "${SSH[@]}" "ASSETS='$REMOTE/$variant' timeout --kill-after=5s 70s sh '$REMOTE/04-target-case.sh' '$REMOTE/mali-counters' '$REMOTE/falllinie-trace005' '$REMOTE/evidence/$name' live" || status=$?
 "${SCP[@]}" -r "root@192.168.0.202:$REMOTE/evidence/$name" "$OUT/"
 echo "$name status=$status"
 [ "$status" = 0 ] || exit "$status"
done
printf 'complete=true\n' >> "$OUT/campaign.txt"
echo "$OUT"
