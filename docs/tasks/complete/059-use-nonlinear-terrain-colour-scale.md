# Use A Nonlinear Terrain Colour Scale

Status: complete
Priority: medium
Area: Procgen / Debug Rendering

## Goal

Concentrate terrain-colour variation in the elevation range occupied by most generated land while preserving exceptional mountain and summit colours.

## Context

Task 034 placed its main colour transitions across the theoretical normalized land range. Representative default terrain instead clusters around final elevation `0.55..0.66`, leaving most visible terrain inside one low-contrast green segment while uncommon mountain heights own most of the palette.

## Acceptance Criteria

- Replace the broadly spaced terrain ramp with fixed uneven elevation anchors concentrated in the common lowland-to-hill range.
- Preserve fixed cross-map colour semantics; do not normalize colours independently per seed or map histogram.
- Keep shoreline, highland, rock, and exceptional summit ranges visually ordered and continuously interpolated.
- Preserve relative ocean-depth shading, terrain-form tinting, and independent overlays.
- Keep the mapping inside compile-gated engine-neutral debug tooling.
- Add direct colour-data tests for midrange distinction, monotonic elevation response, and deterministic generated image output.
- Update procgen documentation and run focused, full, Debug, and Release verification.

## Dependencies

- Task 034, whose continuous terrain view remains the owning debug mode.
- Task 058, so colour tuning is evaluated after coastline detail no longer leaks into inland elevation.

## Implementation Decisions

- Replaced the three broad relative-height segments with fixed final-elevation anchors at `0.50, 0.54, 0.59, 0.65, 0.75, 0.86, 1.00`.
- Concentrated green-to-hill hue and luminance changes in `0.54..0.65`, the range containing most representative default terrain.
- Reserved the wider upper intervals for earth, rock, and exceptional summits so flat maps cannot consume the whole palette.
- Interpolated continuously between anchors and kept anchor luminance increasing with elevation.
- Reduced the secondary categorical terrain-form tint from `12%` to `8%` so it remains detectable without suppressing elevation contrast.
- Added direct anchor-order and colour-distance tests while retaining deterministic generated-image coverage from task 034.

## Testing

- Passed focused greater-realm colour-data and deterministic image tests.
- Passed `ctest --test-dir out/build/debug-with-tests --output-on-failure` (14/14 tests).
- Passed tests-disabled `debug-no-tests` and `release-no-tests` application builds.
- Rendered and visually inspected the default seed: the nonlinear ramp produced smooth, distinguishable lowland-to-hill variation without banding or false summit colours.
- Passed `git diff --check`.

## Commit Message

Concentrate terrain colours in the common elevation range
Preserve fixed rock and summit semantics across maps
Test nonlinear terrain colour contrast and ordering
