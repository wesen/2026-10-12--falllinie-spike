#!/usr/bin/env bash
# Reviewed parent-only experiment. Separate candidate; never kills/replaces game.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
R=$(git -C "$P" rev-parse --show-toplevel)
: "${SSH_ASKPASS:?external credentials required}"
export SSH_ASKPASS_REQUIRE=force DISPLAY=${DISPLAY:-:0}
SSH=(setsid -w ssh -n -o ConnectTimeout=5 -o ServerAliveInterval=10 -o ServerAliveCountMax=2 -o IdentitiesOnly=yes root@192.168.0.202)
SCP=(setsid -w scp -o ConnectTimeout=5 -o IdentitiesOnly=yes)
REMOTE=/storage/apps/falllinie-perf004
REV=$(git -C "$R" rev-parse HEAD)
mkdir -p "$P/evidence/target"
"${SSH[@]}" "set -e; if pgrep -x falllinie-spike >/dev/null; then echo REFUSED_game_running; exit 2; fi; test ! -e '$REMOTE'; mkdir -p '$REMOTE/evidence'"
"${SCP[@]}" "$R/build-device/falllinie-perf004" "$P/run-one.sh" "root@192.168.0.202:$REMOTE/"
"${SSH[@]}" "chmod +x '$REMOTE/falllinie-perf004'; sha256sum '$REMOTE/falllinie-perf004'" > "$P/evidence/target/deployed-binary.sha256"
order=0
for case in static-off static-passes static-single static-bypass static-res static-off live-off live-passes; do
 order=$((order+1)); name=$(printf '%02d-%s' "$order" "$case")
 "${SSH[@]}" "export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland; SECONDS_PER_RUN=20 PLACE=1 timeout --kill-after=5s 40s sh '$REMOTE/run-one.sh' '$REMOTE/falllinie-perf004' /storage/apps/falllinie-spike/assets/shaders '$REMOTE/evidence/$name' '$case' '$REV' '$order'"
 "${SCP[@]}" -r "root@192.168.0.202:$REMOTE/evidence/$name" "$P/evidence/target/"
 python3 "$P/analyze.py" "$P/evidence/target/$name/frames.csv" > "$P/evidence/target/$name/analysis.json"
 echo "DONE $name"
done
