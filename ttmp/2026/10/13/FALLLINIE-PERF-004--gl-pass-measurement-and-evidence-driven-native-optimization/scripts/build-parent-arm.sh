#!/bin/sh
# PARENT ONLY: existing offline image + explicitly supplied, previously approved libraries.
# Does not acquire/copy target libraries or invoke the old SSH/docker-build helper.
set -eu
[ "$#" = 1 ] || { echo 'usage: build-parent-arm.sh /absolute/approved/target-libs' >&2; exit 2; }
case "$1" in /*) ;; *) echo 'library directory must be absolute' >&2; exit 2;; esac
LIBS=$(CDPATH= cd -- "$1" && pwd)
[ -f "$LIBS/libSDL2-2.0.so.0" ] || { echo 'approved SDL library missing' >&2; exit 2; }
S=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$S/../../../../../.." && pwd)
REL=${S#"$ROOT/"}/.work/arm
mkdir -p "$ROOT/$REL"
docker image inspect falllinie-spike-toolchain > "$ROOT/$REL/image.json"
sha256sum "$LIBS/libSDL2-2.0.so.0" > "$ROOT/$REL/target-library-hashes.txt"
git -C "$ROOT" rev-parse HEAD > "$ROOT/$REL/source-revision.txt"
git -C "$ROOT" diff --stat >> "$ROOT/$REL/source-revision.txt"
docker run --rm --pull=never --network=none --user "$(id -u):$(id -g)" \
 -v "$ROOT:/work" -v "$LIBS:/approved-target-libs:ro" -w /work \
 -e OUTPUT="$REL" falllinie-spike-toolchain bash -euo pipefail -c '
 aarch64-linux-gnu-g++ -std=c++17 -O2 -Wall -Wextra -pthread -I/usr/include/SDL2 -I/usr/include/aarch64-linux-gnu \
  src/*.cpp -o "$OUTPUT/falllinie-spike" -L/approved-target-libs -Wl,--allow-shlib-undefined -l:libSDL2-2.0.so.0
 aarch64-linux-gnu-readelf -h -l -d "$OUTPUT/falllinie-spike" > "$OUTPUT/elf.txt"
 aarch64-linux-gnu-g++ --version > "$OUTPUT/compiler.txt"
 ' > "$ROOT/$REL/build.log" 2>&1
sha256sum "$ROOT/$REL/falllinie-spike" > "$ROOT/$REL/binary.sha256"
