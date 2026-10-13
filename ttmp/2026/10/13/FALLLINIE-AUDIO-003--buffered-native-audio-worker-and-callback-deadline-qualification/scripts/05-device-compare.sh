#!/usr/bin/env bash
# Parent-only bounded target gate. Credentials come from the existing environment.
set -euo pipefail
HERE=$(cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(git -C "$HERE" rev-parse --show-toplevel)
export SSH_ASKPASS_REQUIRE=force DISPLAY=${DISPLAY:-:0}
: "${SSH_ASKPASS:?set existing askpass path}"
: "${ROCKNIX_PW:?credential must be provided outside this script}"
SSH=(setsid -w ssh -n -o ConnectTimeout=5 -o ServerAliveInterval=10 -o ServerAliveCountMax=2 -o IdentitiesOnly=yes root@192.168.0.202)
SCP=(setsid -w scp -o ConnectTimeout=5 -o IdentitiesOnly=yes)
mkdir -p "$HERE/target"
"${SCP[@]}" "$ROOT/build-device/falllinie-spike" root@192.168.0.202:/storage/apps/falllinie-spike/falllinie-spike.new
"${SCP[@]}" "$ROOT/scripts/falllinie-spike.sh" root@192.168.0.202:/storage/roms/ports/falllinie-spike.sh
"${SSH[@]}" 'set -e; pids=$(pgrep -x falllinie-spike || true); if [ -n "$pids" ]; then kill -TERM $pids; timeout 8 flock /tmp/falllinie-spike.lock true; fi; cd /storage/apps/falllinie-spike; chmod +x falllinie-spike.new; mv falllinie-spike.new falllinie-spike'
for mode in direct buffered; do
 "${SSH[@]}" "set -e; export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland; cd /storage/apps/falllinie-spike; mkdir -p /tmp/falllinie-audio003-$mode; ./falllinie-spike --autoplay --dual-screen --place --render-size 384x288 --no-vsync --fps 60 --muted --dev-overlay --audio-buffer 1024 --audio-mode $mode --source-vinyl --seconds 120 --profile /tmp/falllinie-audio003-$mode/frames.csv > /tmp/falllinie-audio003-$mode/run.log 2>&1; tail -4 /tmp/falllinie-audio003-$mode/run.log"
 mkdir -p "$HERE/target/$mode"
 "${SCP[@]}" "root@192.168.0.202:/tmp/falllinie-audio003-$mode/run.log" "root@192.168.0.202:/tmp/falllinie-audio003-$mode/frames.csv" "root@192.168.0.202:/tmp/falllinie-audio003-$mode/frames.csv.gpu.csv" "$HERE/target/$mode/"
done
# Restore a playable title; retain source override in launcher and keep the panel visible.
"${SSH[@]}" 'export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland; cd /storage/apps/falllinie-spike; nohup ./falllinie-spike --play --dual-screen --place --render-size 384x288 --no-vsync --fps 60 --dev-overlay --audio-buffer 1024 --audio-mode buffered --no-vinyl > /tmp/falllinie-audio003-interactive.log 2>&1 </dev/null & echo interactive_pid=$!'
