# Refine Greater Realm Landmass Topology

Status: complete
Area: Procgen
Branch: `proc-gen`
Branch reason: This is part of the existing multi-step greater realm procedural generation feature stream.

## Goal

Refine greater realm generation around a Mapgen4-style signed landmass constraint and layered elevation pipeline so broad coastlines remain stable while inland terrain is shaped independently.

## Context

The first generator combined land shape and all terrain influences before applying sea level. As a result, changing mountain, ridge, valley, or terrain-noise settings could also rewrite the coastline. It also treated every water cell as ocean.

Mapgen4 starts from signed elevation constraints, perturbs the coastline near the zero crossing, then handles land relief and ocean depth separately. This task adopts that pipeline structure while retaining the engine's regular grid.

## Acceptance Criteria

- Generate a deterministic signed landmass constraint where negative values are water, positive values are land, and zero is the coastline.
- Apply controlled coastline noise before inland relief.
- Shape land with separate base elevation, mountain, ridge, valley, and terrain-noise influences.
- Shape water depth separately from land relief.
- Keep land/water topology unchanged when only inland terrain weights change.
- Mark boundary-connected water as ocean without treating all water as ocean by definition.
- Preserve normalized final elevation, coast distance, slope, and terrain-form output.
- Add automated tests for determinism, topology stability, elevation ranges, and ocean connectivity.
- Keep procgen tests isolated from release builds.
- Document the current greater realm generation pipeline.

## Verification

- `procgen_greater_realm` passes with signed constraint, topology stability, sea-level response, and ocean-connectivity coverage.
- All five isolated test suites pass together.
- The procgen debug game target builds with tests enabled.
- Tests-disabled Debug and Release game targets build successfully.
- Test code remains isolated behind `REALM_BUILD_TESTS=ON` and does not enter release builds.
- 2026-08-16 closure check: `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 14/14.
- 2026-08-16 closure check: `cmake --build --preset debug-with-tests` and `cmake --build --preset release-no-tests` were already up to date.
- 2026-08-16 closure check: `cmake --build --preset debug-no-tests` passed when run from the Visual Studio developer command environment.

## Notes

Visual judgment across multiple seeds remains part of testing because believable terrain shape is an aesthetic requirement.

This task does not add hydrology, rivers, biomes, local tiles, or a Delaunay/Voronoi mesh.
