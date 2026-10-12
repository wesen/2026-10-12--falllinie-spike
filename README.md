# Falllinie native terrain / print-shader spike

C++17 + SDL2 platform layer, OpenGL ES renderer loaded through SDL_GL_GetProcAddress.
This is a scoped graphics qualification, not the complete game port.

Authoritative design/tasks/detailed diary: sibling claw-stuff docmgr ticket `FALLLINIE-GLES-001`.
Original source SHA256: `cb9b4befd0252859d9c45246f94fbd7dd1662c6d034c6ccb95fbe16947e8b556`.
Original GLSL shader bodies are preserved under assets/shaders; CPU oracle under fixtures.

## Build

Desktop: `cmake -S . -B build && cmake --build build` (SDL2/GLES3 headers required).

Device: `bash scripts/build-device.sh` with working SSH authentication. It builds in
an isolated pinned Debian container, uses ARM64 headers/compiler, and links the actual
target SDL library. Target libs and binaries are ignored, not vendored.

First hardware gate: ES 3.2 / Mali-G52, GLSL ES 3.20; bounded triangle run
completed 298 frames in five seconds, GL error 0.

SELECT+START on retrogame_joypad or ESC quits. No global Sway configuration changes.
