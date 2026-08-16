# Align Sea Handling With Mapgen4

Status: complete
Priority: high
Area: Procedural Generation / Terrain

## Goal

Stop the sea parameter from changing inland terrain height and align land/water handling with Mapgen4's fixed signed coastline model.

## Context

The current generator uses `sea_level` both as an offset in signed landmass generation and as the baseline for final normalized land elevation. This means changing the sea slider can alter broad topology and rescale the center of a landmass even when water never reaches it. Mapgen4 keeps the coastline at signed elevation `0`: positive signed values produce land relief, negative signed values produce ocean depth, and there is no global sea parameter that raises or compresses all land.

## Acceptance Criteria

- Remove adjustable sea level from signed landmass generation.
- Keep the fixed signed coastline threshold at `0`, with positive values as land and negative values as water.
- Preserve the engine's normalized `0..1` elevation contract with a fixed normalized waterline for final output.
- Ensure changing `GreaterRealmGeneratorSettings::sea_level` no longer changes generated map data, inland land heights, terrain forms, hydrology, peak identities, or debug output.
- Keep island bias, authored constraints, coastline noise, and ocean depth as the controls for topology/detail/depth.
- Update procgen docs and tests for the Mapgen4-aligned sea contract.
- Run all procgen tests plus tests-disabled Debug and Release builds.

## Dependencies

- Task 050 Mapgen4-style land-relief composition.
- Task 051 one-stage authored constraint composition.
- Task 052 stable mountain peak sites.

## Reference

- Mapgen4 `painting.ts` signed elevation field.
- Mapgen4 `map.ts` land/ocean elevation assignment.

## Implementation Summary

- Added `NORMALIZED_WATERLINE` as the engine's fixed normalized output waterline.
- Removed adjustable `sea_level` from signed landmass generation and final land/water elevation assembly.
- Kept land/water topology owned by the fixed signed `0` coastline, island bias, coastline noise, and authored signed constraints.
- Updated debug terrain shading and 3D mesh relative heights to use the fixed waterline instead of `GreaterRealmGeneratorSettings::sea_level`.
- Removed the Sea slider from the debug panel so the UI no longer exposes a non-Mapgen4 generation control.
- Updated tests to prove changing `sea_level` does not alter generated map data, land area, terrain forms, hydrology-relevant output, or debug terrain shading.

## Testing

- `cmake --build out/build/debug-with-tests` passed.
- `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 13/13.
- `cmake --build --preset debug-no-tests` passed.
- `cmake --build --preset release-no-tests` passed.
- Task-scoped `git diff --check` passed with only LF-to-CRLF warnings.
- Full `git diff --check` was run and still reports unrelated trailing whitespace in `src/ecs/World.cpp:69`, which was already outside this task's files and was left untouched.

## Commit Message

Align sea handling with Mapgen4
