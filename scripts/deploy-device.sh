#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
HOST="${HOST:-root@192.168.0.202}"
test -x build-device/falllinie-spike
ssh -o IdentitiesOnly=yes "$HOST" 'mkdir -p /storage/apps/falllinie-spike /storage/roms/ports'
scp -o IdentitiesOnly=yes build-device/falllinie-spike build-device/test-terrain "$HOST:/storage/apps/falllinie-spike/"
scp -r -o IdentitiesOnly=yes assets "$HOST:/storage/apps/falllinie-spike/"
scp -o IdentitiesOnly=yes scripts/falllinie-spike.sh "$HOST:/storage/roms/ports/falllinie-spike.sh"
ssh -o IdentitiesOnly=yes "$HOST" 'chmod +x /storage/roms/ports/falllinie-spike.sh; cd /storage/apps/falllinie-spike; ./test-terrain'
