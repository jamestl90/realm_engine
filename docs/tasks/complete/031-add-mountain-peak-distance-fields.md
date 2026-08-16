# Add Mountain Peak Distance Fields

Status: complete
Area: Procgen / Elevation

## Goal

Generate explicit mountain peaks and use distance fields to shape coherent mountain masses.

## Context

Mapgen4 preselects peak locations and propagates a jagged distance field from them. The current generator uses ridged noise masks, which create high terrain but not explicit peaks with controlled spacing and falloff.

## Acceptance Criteria

- Select deterministic peak locations with configurable density or spacing.
- Compute a deterministic, optionally jagged distance field from peaks.
- Blend peak elevation with the existing land constraint and low-amplitude hill relief.
- Export enough peak metadata for later hydrology and debug inspection.
- Add tests for peak determinism, spacing, distance monotonicity, and parameter response.
- Add compile-gated debug visualization and update procgen documentation.

## Implementation

- Added a dedicated `MountainPeaks` procgen module.
- Selects deterministic land peak candidates with configurable minimum spacing and an inland priority bias.
- Propagates a multi-source grid distance field with deterministic per-edge jaggedness.
- Replaced the previous ridged-noise mountain source while retaining independent ridge, valley, and terrain-noise layers.
- Exported peak records and per-cell distance, influence, and peak metadata.
- Added mountain-strength, peak-spacing, peak-radius, and peak-jaggedness debug controls.
- Added explicit peak markers to the compile-gated debug image.

## Verification

- A dedicated test-only executable covers deterministic peak identity, minimum spacing, metadata, descending distance paths, jaggedness response, radius response, peak-count response, topology independence, and debug-image markers.
- All seven CTest targets pass.
- Tests-disabled Debug and Release builds succeed.
- The Debug executable passes a native startup smoke test with the expanded peak controls.
- `git diff --check` passes.
- 2026-08-16 closure check: `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 14/14.
- 2026-08-16 closure check: `cmake --build --preset debug-with-tests` and `cmake --build --preset release-no-tests` were already up to date.
- 2026-08-16 closure check: `cmake --build --preset debug-no-tests` passed when run from the Visual Studio developer command environment.
