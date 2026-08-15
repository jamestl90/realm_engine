# Align Coastline Noise Attenuation

Status: todo
Priority: medium
Area: Procedural Generation / Coastlines

## Goal

Resolve the undocumented difference between the engine's narrow coastline-noise mask and Mapgen4's signed-constraint attenuation.

## Context

The engine currently fades coastline noise to zero once the absolute broad constraint reaches `0.30`. Mapgen4 scales coastline detail with `1 - e^4`, which is strongest near zero but remains continuous across most of the signed range. Task 049 classified the difference as unjustified rather than immediately declaring either result preferable.

## Acceptance Criteria

- Characterize both attenuation functions across fixed seeds and control extremes.
- Adopt Mapgen4's attenuation or record a concrete visual and behavioral reason for retaining an engine-specific mask.
- Keep coastline detail independent from island bias, sea level, inland relief, and ocean depth.
- Verify deterministic topology response across representative fixed seeds, bounded inland influence, continuous lower/default/upper parameter response, and no land-relief or water-depth coupling.
- Add focused regression tests and update `docs/PROCGEN.md` with the decision.

## Dependencies

- Task 050, so coastline evaluation uses the corrected relief pipeline.
