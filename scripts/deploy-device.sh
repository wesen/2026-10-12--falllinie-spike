#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
HOST="${HOST:-root@192.168.0.202}"
test -x build-device/falllinie-spike
ssh -o IdentitiesOnly=yes "$HOST" 'mkdir -p /storage/apps/falllinie-spike /storage/roms/ports'
# Keep the previous binary for rollback: mv falllinie-spike.previous falllinie-spike
ssh -o IdentitiesOnly=yes "$HOST" 'cd /storage/apps/falllinie-spike; if [ -f falllinie-spike ]; then cp -p falllinie-spike falllinie-spike.previous; fi'
# Atomic rename allows deployment while an older recording run is still executing.
scp -o IdentitiesOnly=yes build-device/falllinie-spike "$HOST:/storage/apps/falllinie-spike/falllinie-spike.new"
scp -o IdentitiesOnly=yes build-device/test-terrain "$HOST:/storage/apps/falllinie-spike/"
scp -r -o IdentitiesOnly=yes assets "$HOST:/storage/apps/falllinie-spike/"
scp -o IdentitiesOnly=yes scripts/falllinie-spike.sh "$HOST:/storage/roms/ports/falllinie-spike.sh"
ssh -o IdentitiesOnly=yes "$HOST" 'chmod +x /storage/roms/ports/falllinie-spike.sh; cd /storage/apps/falllinie-spike; mv falllinie-spike.new falllinie-spike'
# EmulationStation Ports entry: merge into the existing gamelist, keeping other ports.
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
ssh -o IdentitiesOnly=yes "$HOST" 'cat /storage/roms/ports/gamelist.xml 2>/dev/null || true' > "$TMP/current.xml"
python3 scripts/ports/merge-gamelist.py "$TMP/current.xml" "$TMP/gamelist.xml"
ssh -o IdentitiesOnly=yes "$HOST" 'mkdir -p /storage/roms/ports/images; if [ -f /storage/roms/ports/gamelist.xml ]; then cp -p /storage/roms/ports/gamelist.xml /storage/roms/ports/gamelist.xml.bak; fi'
scp -o IdentitiesOnly=yes scripts/ports/falllinie-spike.png "$HOST:/storage/roms/ports/images/falllinie-spike.png"
scp -o IdentitiesOnly=yes "$TMP/gamelist.xml" "$HOST:/storage/roms/ports/gamelist.xml"
# EmulationStation reads gamelists at start; RESTART_ES=1 restarts it (screen resets briefly).
if [ "${RESTART_ES:-0}" = 1 ]; then ssh -o IdentitiesOnly=yes "$HOST" 'systemctl restart essway'; fi
ssh -o IdentitiesOnly=yes "$HOST" 'sha256sum /storage/apps/falllinie-spike/falllinie-spike /storage/roms/ports/falllinie-spike.sh /storage/roms/ports/gamelist.xml'
