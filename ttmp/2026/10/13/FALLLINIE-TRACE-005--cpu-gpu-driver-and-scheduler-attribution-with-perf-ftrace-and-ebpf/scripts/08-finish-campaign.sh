#!/usr/bin/env bash
# Resume only postprocessing + missing bracket cases from the first actual campaign.
set -euo pipefail
P=$(cd -- "$(dirname -- "$0")" && pwd)
STAMP=20261014T020124Z
OUT=$P/evidence/target-$STAMP
REMOTE=/storage/apps/falllinie-trace005-$STAMP
: "${SSH_ASKPASS:?external credentials required}"
export SSH_ASKPASS_REQUIRE=force DISPLAY=${DISPLAY:-:0}
SSH=(setsid -w ssh -n -o ConnectTimeout=5 -o IdentitiesOnly=yes root@192.168.0.202)
SCP=(setsid -w scp -o ConnectTimeout=5 -o IdentitiesOnly=yes)
"${SSH[@]}" "set -e; cd '$REMOTE/evidence/06-perf-record'; perf report -i perf.data --stdio --no-inline -g none --no-children --sort pid,comm,dso,symbol --percent-limit 0.5 > perf-report-no-inline.txt 2>&1; perf report -i perf.data --stdio --no-inline -g none --stats > perf-statistics.txt 2>&1; perf buildid-list -i perf.data > perf-buildids.txt; perf report -i perf.data --stdio --no-inline -g none --no-children --sort pid,comm,dso --percent-limit 0 > perf-dso.txt"
for file in perf-report-no-inline.txt perf-statistics.txt perf-buildids.txt perf-dso.txt; do
 "${SCP[@]}" "root@192.168.0.202:$REMOTE/evidence/06-perf-record/$file" "$OUT/06-perf-record/"
done
[[ "${ONLY_REPORT:-0}" != 1 ]] || exit 0
REV=$(awk -F= '$1=="source_rev" {print $2}' "$OUT/campaign.txt")
for entry in 07-clocks 08-baseline; do
 order=${entry%%-*}; case=${entry#*-}
 "${SSH[@]}" "set -e; if pgrep -af '[f]alllinie-(spike|perf004|trace005)( |$)'; then exit 2; fi; export XDG_RUNTIME_DIR=/run/0-runtime-dir WAYLAND_DISPLAY=wayland-1 SWAYSOCK=/run/0-runtime-dir/sway-ipc.0.sock SDL_VIDEODRIVER=wayland PULSE_SERVER=unix:/run/0-runtime-dir/pulse/native; timeout --kill-after=5s 90s sh '$REMOTE/05-run-target.sh' '$REMOTE/falllinie-trace005' /storage/apps/falllinie-spike/assets/shaders '$REMOTE/evidence/$entry' '$case' '$REV' '$order'"
 "${SCP[@]}" -r "root@192.168.0.202:$REMOTE/evidence/$entry" "$OUT/"
 echo "CASE $entry complete"
done
printf 'campaign_complete=true\npostprocess_recovered=no-inline\n' >> "$OUT/campaign.txt"
