# Add Continuous Elevation-Informed Terrain Colouring

Status: complete
Area: Procgen / Debug Rendering

## Goal

Visualize greater-realm terrain with continuous elevation-informed colour rather than terrain-form categories alone.

## Context

Mapgen4's colour mapping combines multiple fields, then applies terrain lighting and water/coast outlines. The current debug map uses discrete terrain-form colours and ocean-depth shading. Runtime weather must remain separate from this generated geography view.

## Acceptance Criteria

- Add an engine-neutral continuous colour mapping from elevation and terrain form.
- Preserve water-depth readability and independent coastline metadata.
- Keep categorical terrain-form visualization available as a debug mode.
- Avoid assigning final game biomes or art direction in the engine layer.
- Do not use runtime rainfall, humidity, or moisture as generated inputs.
- Add image-data tests for parameter response, stable dimensions, and deterministic output.
- Expose the mode only through compile-gated debug tooling.
- Update procgen documentation.

## Dependencies

- Tasks 050 and 051, so continuous colouring is tuned against corrected land relief and authored-constraint semantics instead of preserving accidental elevation output.

## Implementation Decisions

- Made the existing `Terrain` base view the continuous geography view so flat and 2.5D previews share the improved default without adding runtime rendering policy.
- Mapped normalized land elevation through lowland, upland, exposed-rock, and summit anchors, then blended in a restrained terrain-form tint to retain generated-form readability without hard categorical bands dominating the image.
- Preserved the existing relative ocean-depth mapping and independent coastline overlay unchanged.
- Added `Terrain forms` as a separate selectable base view containing the previous categorical palette.
- Kept the mapping in the compile-gated, engine-neutral `GreaterRealmDebug` module and excluded weather or biome inputs.
- Added direct colour tests and generated image-data coverage for dimensions, determinism, and relief-parameter response.

## Testing

- Passed focused `realm_engine_tests` and `realm_greater_realm_debug_panel_tests` executables.
- Passed `ctest --test-dir out/build/debug-with-tests --output-on-failure` (14/14 tests).
- Passed the tests-disabled `debug-no-tests` application build.
- Passed the tests-disabled `release-no-tests` application build; the debug-only module remained excluded.
- Passed `git diff --check`.

## Commit Message

Add continuous elevation-informed terrain colouring
Preserve categorical terrain forms as a selectable debug view
Test terrain colour determinism and relief response
