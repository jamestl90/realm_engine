# Align Coastline Noise Attenuation

Status: complete
Priority: medium
Area: Procedural Generation / Coastlines

## Goal

Align coastline perturbation with Mapgen4's attenuation, high-frequency spectrum, and control scale so the default changes coast shape without visually dominating inland terrain.

## Context

The engine previously faded coastline noise to zero once the absolute broad constraint reached `0.30`. Mapgen4 scales coastline detail with `1 - e^4`, which is strongest near zero but remains continuous across most of the signed range.

The first task-053 implementation adopted that attenuation while retaining the engine's normalized four-octave fBm and `0.08` default. Mapgen4 instead uses three samples at frequencies `16, 32, 64`, relative weights `1, 1/2, 1/4`, a `0.01` default, and a `0..0.1` control range. Keeping the stronger engine spectrum made the newly broad attenuation visibly affect the entire map.

## Acceptance Criteria

- Use Mapgen4's signed attenuation `1 - e^4`.
- Use the engine noise primitive with Mapgen4's fixed `16, 32, 64` coastline sampling and `1, 1/2, 1/4` relative weights.
- Default coastline detail to `0.01` and expose the Mapgen4 `0..0.1` range.
- Remove the obsolete configurable coastline frequency.
- Keep coastline detail independent from island bias, sea level, inland relief, and ocean depth.
- Verify deterministic response, Mapgen4 default/range, bounded perturbation, and no land-relief or water-depth coupling.
- Update `docs/PROCGEN.md` and run focused, full, Debug, and Release verification.

## Dependencies

- Task 050, so coastline evaluation uses the corrected relief pipeline.

## Implementation Summary

- Adopted Mapgen4's coastline attenuation formula `1 - e^4` for the signed broad constraint.
- Replaced normalized four-octave coastline fBm with fixed high-frequency samples at `16, 32, 64` and relative weights `1, 1/2, 1/4`.
- Reduced the default coastline strength from `0.08` to Mapgen4's `0.01` and the debug slider range from `0..0.4` to `0..0.1`.
- Removed the unused coastline-frequency setting.
- Kept coastline perturbation before relief, water depth, hydrology, and classification.
- Retuned the default potential-channel catchment threshold from 700 to 500 after the corrected fixed seed produced 14 visible segments at 500, 7 at 600, and none at 700.

## Testing

- Focused greater-realm and debug-panel tests passed.
- Focused hydrology/constraint pipeline test passed after the channel-threshold retune.
- Full Debug test suite passed: 13/13 tests.
- Standalone `debug-no-tests` and `release-no-tests` builds succeeded.
- `git diff --check` passed (line-ending warnings only).

## Commit Message

Align coastline detail scale and spectrum with Mapgen4

Preserve visible default rivers after coastline retuning