# Align Land Relief With Mapgen4 Composition

Status: complete
Priority: high
Area: Procedural Generation / Elevation

## Goal

Restore a Mapgen4-style local land-relief composition in which the signed terrain constraint controls the blend between low-amplitude hills and peak-distance mountains.

## Context

Task 049 confirmed that the current generator adds base fBm, mountain influence, ridges, and valleys into one globally normalized relief value. Mapgen4 instead derives low hills and a peak-distance mountain profile, then blends between them using the squared positive signed constraint. Task 031 required this behavior, but its implementation retained the older additive relief stack.

## Acceptance Criteria

- Define low-amplitude hill relief and a peak-distance mountain target as separate, inspectable stages.
- Use positive signed terrain constraint strength to blend hills toward mountain relief locally; do not raise all inland terrain merely because a global mountain control changed.
- Evaluate Mapgen4's automatic positive-land mountain hint and either adopt it or record why the explicit peak field supersedes it.
- Keep ridge, valley, and terrain-noise controls only as secondary engine extensions with documented stage ordering and masks.
- Preserve signed land/water topology and the normalized `0..1` final-elevation data contract.
- Retune defaults deliberately rather than preserving accidental aggregate output.
- Add lower/default/upper tests across representative fixed seeds for local significance, monotonicity, topology independence, and visible peak-versus-hill separation, including base, ridge, valley, terrain-noise, and mountain controls owned by this stage.
- Update `docs/PROCGEN.md` and verify all procgen tests plus tests-disabled Debug and Release builds.

## Dependencies

- Task 049 audit findings.

## Reference

- Mapgen4 `painting.ts` automatic constraint generation.
- Mapgen4 `map.ts` `assignTriangleElevation` hill/mountain blend.

## Implementation Notes

- Added inspectable per-cell `hill_relief` and `mountain_relief` stages.
- Adopted Mapgen4's automatic positive-land mountain hint in the signed terrain field before authored constraints and coastline noise. The explicit peak-distance field still owns mountain target shape; the hint supplies local positive signed-constraint strength for the hill-to-mountain blend.
- Replaced the additive globally normalized land stack with a Mapgen4-style blend: low-amplitude hill relief blends toward peak-distance mountain relief by `max(landmass_elevation, 0)^2`.
- Kept ridge, valley, and terrain-noise controls as secondary extensions after the blend, masked by positive inland constraint strength.
- Removed the second authored-constraint interpolation from final relief; authored constraints now affect relief through the signed landmass field.
- Retuned default `river_min_drainage_area` from `800` to `700` because the corrected relief stage made the old default suppress the fixed-seed default potential river network.
- Added debug base views for hill relief and mountain relief.

## Testing

- Passed: `ctest --preset debug-with-tests` (11/11 tests).
- Passed: `cmake --preset debug-no-tests && cmake --build --preset debug-no-tests`.
- Passed: `cmake --preset release-no-tests && cmake --build --preset release-no-tests`.
- Note: the first direct build attempt outside `VsDevCmd.bat` failed before project compilation because MSVC standard include paths were unavailable (`cstddef`, `cstdint`, `stdarg.h`). Verification was rerun successfully through the Visual Studio developer environment.

## Commit Message

```text
Align land relief with Mapgen4 composition
```
