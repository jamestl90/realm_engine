# Separate Coastline from Terrain Form

Status: complete
Area: Procgen / Debug Rendering

## Goal

Represent the coastline as a land-water boundary without replacing the underlying terrain form or rendering every shore as sand.

## Context

The greater realm generator previously assigned `TerrainForm::Coast` to every land cell within a fixed distance of water. This took priority over elevation-based classification, so coastal hills, highlands, and mountains lost their terrain form and the debug renderer painted the entire coastal band one sandy colour.

Mapgen4 treats the coast as the zero crossing between negative water elevation and positive land elevation. Terrain elevation and rendering remain continuous at that boundary instead of using a universal coast terrain class.

## Acceptance Criteria

- Remove `Coast` as an exclusive `TerrainForm` and remove settings used only by that override.
- Preserve `distance_to_coast` and add explicit coastal-boundary metadata for land cells touching water.
- Classify all land cells as plains, hills, highlands, or mountains from elevation, including cells at the shoreline.
- Render a narrow coastline boundary accent without applying a uniform beach colour.
- Keep coastal terrain colours distinguishable in the debug image.
- Update debug statistics, tests, and procgen documentation for the new data model.
- Verify tests-disabled Debug and Release builds, automated tests, and the native debug runtime.

## Implementation

- Removed `TerrainForm::Coast` and the classification-only `coast_distance` setting.
- Added `GreaterRealmCell::is_coastal` as orthogonal metadata for land directly touching water.
- Retained `distance_to_coast` for proximity queries and future shoreline systems.
- Classified all land from elevation thresholds without a coastal override.
- Replaced the sandy coast palette with a one-cell darkening accent over each coastal cell's existing terrain colour.
- Updated debug statistics to report coastal land independently from terrain-form totals.

## Verification

- Procgen tests verify coastal flags exactly match land touching water.
- Procgen tests verify coastal metadata cannot override elevation-based mountain classification.
- Debug-image tests verify coastal plains and mountains retain distinct colours beneath the boundary accent.
- Tests-disabled Debug and Release builds succeed.
- All five CTest targets pass.
- Release continues to exclude the compile-gated procgen debug sources.
- A native runtime smoke test initializes the updated procgen texture and UI rendering paths without errors.
- `git diff --check` passes.
- 2026-08-16 closure check: `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 14/14.
- 2026-08-16 closure check: `cmake --build --preset debug-with-tests` and `cmake --build --preset release-no-tests` were already up to date.
- 2026-08-16 closure check: `cmake --build --preset debug-no-tests` passed when run from the Visual Studio developer command environment.

## Notes

This task does not add beach, cliff, marsh, delta, or rocky-shore classification. Those remain later shoreline or local-terrain concerns.
