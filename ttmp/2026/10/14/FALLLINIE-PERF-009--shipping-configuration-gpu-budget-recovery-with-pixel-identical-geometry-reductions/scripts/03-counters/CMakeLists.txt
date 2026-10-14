cmake_minimum_required(VERSION 3.13.5)
project(falllinie_perf009_counters LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 14)
set(HWCPIPE_WERROR OFF CACHE BOOL "Third-party warning policy" FORCE)
# Reuse the pinned PERF-004 libGPUCounters checkout (378c7b5) without copying it.
set(LIBGPUCOUNTERS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../13/FALLLINIE-PERF-004--gl-pass-measurement-and-evidence-driven-native-optimization/scripts/03-mali-counters/.work/libGPUCounters)
add_subdirectory(${LIBGPUCOUNTERS} libGPUCounters)
add_executable(mali-counters probe.cpp)
target_link_libraries(mali-counters PRIVATE hwcpipe device pthread)
