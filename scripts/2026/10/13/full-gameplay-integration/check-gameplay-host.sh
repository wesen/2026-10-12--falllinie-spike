#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../../../../.." && pwd)
cd "$root"
mkdir -p build/gameplay-check
common=(src/terrain.cpp src/game.cpp src/world.cpp src/pose.cpp src/particles.cpp)
for test in gameplay gameplay_world gameplay_pose; do
  c++ -std=c++17 -O2 -Wall -Wextra -Isrc "tests/test_${test}.cpp" "${common[@]}" -o "build/gameplay-check/test-${test}"
  "build/gameplay-check/test-${test}"
done
# Link the current integration-owned main without modifying its build inputs.
c++ -std=c++17 -O2 -Wall -Wextra -Isrc $(pkg-config --cflags sdl2) src/main.cpp src/renderer.cpp src/game_renderer.cpp src/profiler.cpp "${common[@]}" $(pkg-config --libs sdl2) -pthread -o build/gameplay-check/falllinie-spike
printf '%s\n' 'host executable link passed (not run; no device/graphics access)'
