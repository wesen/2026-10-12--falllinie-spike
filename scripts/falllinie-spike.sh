#!/usr/bin/env bash
set -euo pipefail
export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock
cd /storage/apps/falllinie-spike
exec ./falllinie-spike --place --play --dual-screen --render-size 384x288 --no-vsync --fps 60 --audio-buffer 1024 --no-vinyl "$@" > /tmp/falllinie-spike-port.log 2>&1
