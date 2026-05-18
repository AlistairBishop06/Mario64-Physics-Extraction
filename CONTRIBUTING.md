# Contributing

## Goals

SM64 Physics Sandbox is an educational movement-analysis project. Contributions should improve determinism, inspectability, maintainability, or accuracy without adding copyrighted assets.

## Legal Rules

- Do not commit ROMs, extracted assets, textures, audio, models, or proprietary Nintendo data.
- Do not paste decompiled copyrighted source.
- ROM-dependent behavior should be implemented as local extraction code where users supply their own ROM.
- Generated extraction output belongs in ignored local directories such as `extracted/`.

## Code Style

- Use modern C++17.
- Keep modules small and explicit.
- Avoid global mutable state outside narrow utility systems such as logging.
- Keep physics and rendering separate.
- Prefer deterministic fixed-step simulation logic.
- Use GLM for vector/matrix math.
- Add tests for movement math, serialization, and state transitions when changing behavior.

## Pull Request Checklist

- Project builds with CMake.
- Tests pass with `ctest --output-on-failure`.
- New movement or collision behavior includes focused tests or a clear reason tests are impractical.
- No copyrighted assets or extracted data are included.
- Debug features are useful for analysis rather than cosmetic only.

