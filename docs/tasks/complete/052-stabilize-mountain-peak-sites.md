# Stabilize Mountain Peak Sites

Status: complete
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
- Add lower/default/upper tests across representative fixed seeds for peak identity stability, spacing, distribution, local edit bounds, jaggedness/radius response, and parameter ownership.
- Update `docs/PROCGEN.md` and run all procgen tests plus tests-disabled Debug and Release builds.

## Dependencies

- Task 049 audit findings.

## Reference

- Mapgen4 `generate-points.ts` Poisson peak-site selection.
- Mapgen4 `map.ts` cached peak-distance propagation.

## Implementation Summary

- Replaced land-dependent bucket candidate selection with deterministic fixed peak-site sampling across the canonical grid.
- Preserved Poisson-style minimum spacing by greedily accepting fixed sites in deterministic priority order; selected water sites reserve spacing but stay dormant until they are land.
- Added a static center priority bias so the fixed site field avoids boundary-heavy artifacts without depending on mutable terrain output.
- Kept active peak export land-filtered, so authored ocean paint can make a fixed site dormant and later mountain paint can reactivate the same site without relocating distant peaks.
- Expanded mountain peak tests for lower/default/upper spacing scenarios, deterministic identity, spacing, distribution, relief-control ownership, and authored dormancy/reactivation.

## Testing

- Focused `procgen_mountain_peaks` CTest passed.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 13/13.
- `cmake --build --preset debug-no-tests` passed.
- `cmake --build --preset release-no-tests` passed.
- `git diff --check` passed with only existing LF-to-CRLF warnings on touched files.

## Commit Message

Stabilize mountain peak sites
