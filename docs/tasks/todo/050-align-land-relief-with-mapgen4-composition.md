# Align Land Relief With Mapgen4 Composition

Status: todo
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
- Add fixed-seed tests for local significance, monotonicity, topology independence, and visible peak-versus-hill separation.
- Update `docs/PROCGEN.md` and verify all procgen tests plus tests-disabled Debug and Release builds.

## Dependencies

- Task 049 audit findings.

## Reference

- Mapgen4 `painting.ts` automatic constraint generation.
- Mapgen4 `map.ts` `assignTriangleElevation` hill/mountain blend.
