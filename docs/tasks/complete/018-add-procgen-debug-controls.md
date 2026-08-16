# Add Procgen Debug Controls

Status: complete
Area: Procgen / UI
Branch: `proc-gen`
Branch reason: This continues the procedural generation feature stream and builds on the debug map renderer.

## Goal

Add debug UI controls for tuning the greater realm generator without recompiling.

## Context

The generated map can now be rendered, but tuning requires code edits and rebuilds. The next step is to expose a small debug-only control panel for changing the seed and key generator weights while viewing the result.

## Acceptance Criteria

- Add compile-gated debug controls behind `REALM_ENABLE_PROCGEN_DEBUG_VIEW`.
- Allow changing seed, sea level, land shape, island bias, coastline detail, base relief, mountain weight, ridge weight, valley weight, terrain noise, and ocean depth.
- Keep the expanded panel compact enough to fit comfortably within the logical viewport.
- Regenerate the debug map texture from the UI.
- Show the active settings and basic terrain counts.
- Keep the debug controls out of release builds by default.
- Verify debug game, procgen tests, and release game builds.

## Verification

- The expanded panel builds in a Debug configuration with `REALM_BUILD_TESTS=OFF`.
- `REALM_ENABLE_PROCGEN_DEBUG_VIEW=ON` is present on the game target.
- The panel uses eleven compact control rows within the logical viewport.
- 2026-08-16 closure check: `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 14/14.
- 2026-08-16 closure check: `cmake --build --preset debug-with-tests` and `cmake --build --preset release-no-tests` were already up to date.
- 2026-08-16 closure check: `cmake --build --preset debug-no-tests` passed when run from the Visual Studio developer command environment.

## Notes

Use existing UI controls for now. A dedicated slider control can be added later if this panel becomes important.
