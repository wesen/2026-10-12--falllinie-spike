#!/usr/bin/env bash
set -euo pipefail
root=$(cd "$(dirname "$0")/../../../../.." && pwd)
cd "$root"
build=$(mktemp -d "${TMPDIR:-/tmp}/falllinie-source-audio.XXXXXX")
trap 'rm -rf "$build"' EXIT
read -ra sdl_flags <<< "$(pkg-config --cflags --libs sdl2)"
flags=(-std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -pthread)
if [[ ${AUDIO_SANITIZE:-0} == 1 ]]; then
  flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
fi
"${CXX:-g++}" "${flags[@]}" -Isrc src/audio.cpp tests/test_audio.cpp "${sdl_flags[@]}" -o "$build/test-audio"
"$build/test-audio" "$@"
