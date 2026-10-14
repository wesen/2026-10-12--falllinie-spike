#!/usr/bin/env bash
# One substantive campaign after instrumentation is built. Parent only.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
R=$(git -C "$P" rev-parse --show-toplevel)
: "${SSH_ASKPASS:?external credentials required}"
export SSH_ASKPASS_REQUIRE=force DISPLAY=${DISPLAY:-:0}
SSH=(setsid -w ssh -n -o ConnectTimeout=5 -o ServerAliveInterval=10 -o ServerAliveCountMax=2 -o IdentitiesOnly=yes root@192.168.0.202)
SCP=(setsid -w scp -o ConnectTimeout=5 -o IdentitiesOnly=yes)
REV=$(git -C "$R" rev-parse HEAD)
STAMP=$(date -u +%Y%m%dT%H%M%SZ)
REMOTE=/storage/apps/falllinie-trace005-$STAMP
OUT=$P/evidence/target-$STAMP
BIN=$R/build-device/falllinie-trace005
mkdir "$OUT"
# Refuse to disturb any live Falllinie workload. Does not kill existing processes.
"${SSH[@]}" "set -e; if pgrep -af '[f]alllinie-(spike|perf004|trace005)( |$)' ; then echo REFUSED_game_running; exit 2; fi; mkdir '$REMOTE'; mkdir '$REMOTE/evidence'"
"${SCP[@]}" "$BIN" "$P/05-run-target.sh" "root@192.168.0.202:$REMOTE/"
sha256sum "$BIN" > "$OUT/local-binary.sha256"
"${SSH[@]}" "chmod +x '$REMOTE/falllinie-trace005'; sha256sum '$REMOTE/falllinie-trace005'" > "$OUT/deployed-binary.sha256"
test "$(cut -d' ' -f1 "$OUT/local-binary.sha256")" = "$(cut -d' ' -f1 "$OUT/deployed-binary.sha256")"
printf 'source_rev=%s\nremote=%s\n' "$REV" "$REMOTE" > "$OUT/campaign.txt"
order=0
for case in baseline clocks view passes perf-stat perf-record clocks baseline; do
 order=$((order+1)); name=$(printf '%02d-%s' "$order" "$case")
 status=0
 "${SSH[@]}" "export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland PULSE_SERVER=unix:/run/0-runtime-dir/pulse/native; timeout --kill-after=5s 90s sh '$REMOTE/05-run-target.sh' '$REMOTE/falllinie-trace005' /storage/apps/falllinie-spike/assets/shaders '$REMOTE/evidence/$name' '$case' '$REV' '$order'" || status=$?
 "${SCP[@]}" -r "root@192.168.0.202:$REMOTE/evidence/$name" "$OUT/"
 # Keep raw perf payload out of tracked evidence (including when postprocessing fails).
 if [[ -e "$OUT/$name/perf.data" ]]; then
  mkdir -p "$P/.work/$STAMP/$name"
  mv "$OUT/$name/perf.data" "$P/.work/$STAMP/$name/"
 fi
 echo "CASE $name status=$status"
 if [[ "$status" != 0 ]]; then exit "$status"; fi
 done
printf 'campaign_complete=true\n' >> "$OUT/campaign.txt"
echo "$OUT"
