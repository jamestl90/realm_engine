# Stabilize Mountain Peak Sites

Status: todo
Priority: high
Area: Procedural Generation / Mountains

## Goal

Keep deterministic mountain peak sites stable when local terrain constraints are painted.

## Context

Task 049 measured a locality defect in the current land-dependent bucket candidate selection. For seed `314159` on a `96x72` map, one center mountain brush retained four total peaks but replaced two peak identities and changed 1,027 elevations outside a normalized radius of `0.30`. Mapgen4 samples spaced peak sites before terrain constraints, so painting changes how fixed sites influence terrain rather than relocating the peak field globally.

## Acceptance Criteria

- Generate deterministic, well-spaced peak sites independently from mutable land/water output.
- Adapt Poisson-style spacing to the canonical grid without introducing a dual mesh.
- Keep peak sites stable across authored constraint edits, mountain strength, radius, jaggedness, and other non-spacing relief controls.
- Define how fixed sites under water remain dormant and become relevant if painting creates land.
- Avoid bucket-aligned sampling artifacts and retain deterministic seed behavior.
- Add tests for peak identity stability, spacing, distribution, local edit bounds, and parameter ownership.
- Update `docs/PROCGEN.md` and run all procgen tests plus tests-disabled Debug and Release builds.

## Dependencies

- Task 049 audit findings.

## Reference

- Mapgen4 `generate-points.ts` Poisson peak-site selection.
- Mapgen4 `map.ts` cached peak-distance propagation.
