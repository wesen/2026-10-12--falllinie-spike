#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
HOST="${HOST:-root@192.168.0.202}"
node scripts/generate-gl-loader.cjs
mkdir -p target-libs build-device
# Copy dereferenced target libraries, without replacing any device files.
for lib in libSDL2-2.0.so.0 libGLESv2.so.2; do
  ssh -o ConnectTimeout=10 -o IdentitiesOnly=yes "$HOST" "cat /usr/lib/$lib" > "target-libs/$lib"
done
sha256sum target-libs/* > build-device/target-library-hashes.txt
docker build -t falllinie-spike-toolchain -f scripts/toolchain.Dockerfile .
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/work" falllinie-spike-toolchain bash -euo pipefail -c '
  aarch64-linux-gnu-g++ -std=c++17 -O2 -Wall -Wextra -pthread -I/usr/include/SDL2 -I/usr/include/aarch64-linux-gnu \
    src/*.cpp -o build-device/falllinie-spike \
    -Ltarget-libs -Wl,--allow-shlib-undefined -l:libSDL2-2.0.so.0
  aarch64-linux-gnu-readelf -h -l -d build-device/falllinie-spike > build-device/elf.txt
  aarch64-linux-gnu-g++ --version > build-device/compiler.txt
'
file build-device/falllinie-spike
