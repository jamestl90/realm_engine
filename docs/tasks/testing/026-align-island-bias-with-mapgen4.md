# Align Island Bias With Mapgen4

Status: testing
Area: Procgen / Debug UI

## Goal

Make the greater-realm Island bias control use Mapgen4's range and signed island-constraint formula.

## Context

Mapgen4 combines five-octave fBm with `island * (0.75 - 2 * distance^2)`, then halves the sum. Its Island control defaults to `0.5` and is constrained to `0..1`.

The engine previously exposed values up to `2.0` and separately scaled the fBm term with Land shape, so Island bias was not operating against the same base constraint.

## Acceptance Criteria

- Use Mapgen4's five-octave frequency progression for the broad land-shape field.
- Combine unscaled signed land noise and the island constraint using Mapgen4's formula.
- Clamp Island bias to the `0..1` range and retain the `0.5` default.
- Remove the redundant Land shape weight control and setting.
- Keep the engine's sea-level offset and later coastline-detail stage independent from Island bias.
- Preserve Mapgen4's signed-elevation behavior: Island bias may affect both coastline shape and pre-depth water elevation.
- Add regression coverage for the formula, range, topology, and determinism.
- Update the procgen inventory and verify tests-disabled Debug and Release builds.

## Implementation

- Removed the public Land shape frequency and weight settings and the corresponding debug UI row.
- Fixed the broad land-shape fBm progression to frequencies `1, 2, 4, 8, 16`.
- Changed the signed constraint to `0.5 * (land_noise + island_constraint * island_bias)` before the engine's independent sea-level offset.
- Clamped Island bias to `0..1` in the generator and changed the debug control to the same range with `0.05` increments.
- Retained the `0.5` default and Mapgen4-style influence on signed water elevation.
- Updated generator logging and the procgen feature inventory.

## Verification

- Regression coverage verifies the default, lower and upper clamping, topology response, boundary water, and signed-elevation response.
- All five CTest targets pass, including `procgen_greater_realm`.
- Tests-disabled Debug builds with the procgen debug view enabled.
- Tests-disabled Release builds with procgen debug sources excluded.
- `git diff --check` passes.

## Reference

- Mapgen4 `painting.ts`: automatic signed constraint generation.
- Mapgen4 `mapgen4.ts`: Island default and control range.

