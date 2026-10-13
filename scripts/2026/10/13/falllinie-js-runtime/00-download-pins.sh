#!/usr/bin/env bash
# Downloads scoped ignored sources only. No installs or production vendoring.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p .work
fetch(){ local file=$1 url=$2 hash=$3; if [[ ! -f .work/$file ]]; then curl -fLsS --max-time 90 "$url" -o ".work/$file"; fi; echo "$hash  .work/$file" | sha256sum -c -; tar -xJf ".work/$file" -C .work; }
fetch quickjs-2026-06-04.tar.xz https://bellard.org/quickjs/quickjs-2026-06-04.tar.xz b376e839b322978313d929fd20663b11ba58b75df5a46c126dd19ea2fa70ad2a
fetch duktape-2.7.0.tar.xz https://duktape.org/duktape-2.7.0.tar.xz 90f8d2fa8b5567c6899830ddef2c03f3c27960b11aca222fa17aa7ac613c2890
if [[ ! -d .work/quickjs-ng ]]; then git clone --depth 1 --branch v0.17.0 https://github.com/quickjs-ng/quickjs.git .work/quickjs-ng; fi
test "$(git -C .work/quickjs-ng rev-parse HEAD)" = 6d46d07d04041b40f4f49eaa7fdebe44c314c699
if [[ ! -d .work/quickjspp ]]; then git clone https://github.com/ftk/quickjspp.git .work/quickjspp; git -C .work/quickjspp checkout 01cdd3047ced48265b127790848a0ca88204f2c7; fi
test "$(git -C .work/quickjspp rev-parse HEAD)" = 01cdd3047ced48265b127790848a0ca88204f2c7
