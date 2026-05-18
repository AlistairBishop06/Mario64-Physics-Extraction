# SM64 Physics Sandbox

SM64 Physics Sandbox is a standalone C++17 project for studying Super Mario 64 style movement and collision physics with modern debugging tools. It does **not** include Nintendo assets, ROMs, or extracted copyrighted data.

The project is intended to feel like a professional reverse-engineering and physics analysis tool: deterministic simulation, inspectable movement state, replay comparison, tweakable constants, and visible collision geometry.

## Features

- C++17, SDL2, OpenGL, ImGui, GLM, CMake.
- Fixed timestep simulation decoupled from rendering.
- Modular systems for physics, Mario movement, collision, replay, debug UI, assets, and utilities.
- SM64-style 16-bit angle helpers.
- Starter Mario controller with walking, running, jumping, double/triple jumps, long jump, wall kick, dive, crouch, air control, momentum preservation, slope sliding hooks, and ledge-grab transition hooks.
- Triangle collision world with floor, wall, and ceiling contact queries.
- Debug rendering for velocity vectors, collision normals, floor angle, and primitive collision meshes.
- Replay/ghost skeleton with JSON serialization and timeline sampling.
- Runtime tweak variables and developer console command registry.
- ROM asset extractor CLI that verifies locally supplied ROM hashes and writes local metadata/config outputs only.
- Unit tests for movement math and replay serialization.

## Legal Boundary

This repository must never contain Nintendo assets, ROM files, extracted meshes, sounds, textures, or copyrighted game data. Users may place their own legally obtained ROM under `roms/` locally. The extractor writes output to a local ignored folder such as `extracted/`.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

To build only the core tools and tests without SDL2/OpenGL/ImGui:

```bash
cmake -S . -B build -DSM64PS_BUILD_APP=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run

```bash
./build/sm64_physics_sandbox
```

On Windows multi-config generators:

```powershell
.\build\Release\sm64_physics_sandbox.exe
```

## ROM Extraction

Place a legally obtained ROM in `roms/`, then run:

```bash
./build/sm64ps_asset_extractor roms/sm64.z64 extracted/
```

The current extractor verifies the SHA-1 hash against known SM64 retail ROM hashes and writes local JSON metadata plus movement tuning seed data. It is structured so later commits can add non-redistributable extraction logic without changing the application architecture.

## Project Structure

```text
src/
  assets/       ROM verification and extraction pipeline code
  collision/    triangle collision, floor/wall/ceiling contacts
  debug/        ImGui debug UI, tweak variables, developer console
  mario/        action states, controller, SM64-style movement maths
  physics/      deterministic fixed timestep world stepping
  rendering/    SDL/OpenGL renderer and debug primitive rendering
  replay/       replay recording, serialization, ghost comparison
  surfaces/     surface flags and friction profiles
  util/         logging, config, hashing, profiling helpers
assets_extractor/
tools/
tests/
```

## Development Notes

- Keep simulation code deterministic and independent from renderer frame rate.
- Prefer small data structs and focused systems over large manager classes.
- Treat extracted data as local generated output only.
- Add tests for math helpers and movement transitions before refining controller accuracy.

