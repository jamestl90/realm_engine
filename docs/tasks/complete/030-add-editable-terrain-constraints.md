# Add Editable Terrain Constraints

Status: complete
Area: Procgen / Tooling

## Goal

Allow authored ocean, shallow-water, valley, and mountain constraints to guide procedural greater-realm generation.

## Context

Mapgen4 separates a low-resolution signed constraint field from generated terrain and lets users paint into it. The engine currently supports automatic constraints only.

## Acceptance Criteria

- Define an engine-owned, serializable signed constraint-field format.
- Bilinearly sample constraints independently of output-map resolution.
- Support ocean, shallow-water, valley, and mountain authoring strengths.
- Blend authored constraints into the layered generator without breaking deterministic unedited output.
- Keep editing UI in tooling or application code, outside the core generator.
- Add tests for interpolation, serialization, determinism, and local edit influence.
- Add compile-gated debug editing controls and update procgen documentation.

## Implementation

- Added `TerrainConstraintField` with independent resolution, smooth normalized-coordinate painting, and bilinear sampling.
- Added Mapgen4-compatible ocean, shallow-water, valley, and mountain signed tool values.
- Added validated, versioned little-endian binary serialization and deserialization.
- Blended optional authored constraints into signed topology and relief without changing unedited generation.
- Added application-owned constraint storage and compile-gated coordinate/stamp/clear controls.

## Verification

- Dedicated tests validate tool values, brush falloff, bilinear sampling, exact serialization round trips, malformed-data rejection, local influence, distant stability, and deterministic edited output.
- All six CTest targets pass.
- Tests-disabled Debug and Release builds succeed.
- The Debug executable passes a native startup smoke test.
- 2026-08-16 closure check: `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 14/14.
- 2026-08-16 closure check: `cmake --build --preset debug-with-tests` and `cmake --build --preset release-no-tests` were already up to date.
- 2026-08-16 closure check: `cmake --build --preset debug-no-tests` passed when run from the Visual Studio developer command environment.

## Notes

The Constraint X/Y and stamp buttons are a temporary test interface for the engine-owned constraint field. Mapgen4 paints constraints directly from pointer coordinates on its map; task 035 explicitly tracks removing Constraint X/Y and implementing equivalent preview painting.

## Current State

Task 035 later replaced the temporary Constraint X/Y harness with selectable tools and direct continuous painting on the greater-realm preview.
