# Localize Coastline Detail

Status: complete
Priority: high
Area: Procedural Generation / Coastlines

## Goal

Keep coastline detail confined to the signed shoreline boundary so it cannot inject high-frequency variation into inland terrain relief.

## Context

Task 053 copied Mapgen4's `1 - e^4` attenuation. That formula retains 93.75% strength at `abs(e) = 0.5`, and Mapgen4's own source marks its broad spread as undesirable. The perturbed signed field is also reused by the engine for hill-to-mountain blending and inland extension masks, making the leakage visible across the continuous terrain-colour view.

## Acceptance Criteria

- Replace polynomial attenuation with compact smooth support around the unperturbed signed coastline.
- Retain the existing fixed `16, 32, 64` noise spectrum, relative weights, default, and control range.
- Use the unperturbed broad constraint for inland relief blending and relief-extension masks.
- Continue using the perturbed constraint for coastline topology and near-shore water depth.
- Preserve authored-constraint composition and deterministic generation.
- Prove that changing only coastline detail leaves signed terrain and final elevation unchanged outside the support band.
- Update the Mapgen4 alignment boundary and procgen documentation.
- Run focused, full, Debug, and Release verification.

## Reference

- Mapgen4 `map.ts` applies `1 - e^4`, immediately notes that it spreads farther than intended, and then reuses the perturbed value as the hill-to-mountain blend weight.

## Dependencies

- Task 053, whose spectrum and parameter scale are retained while its attenuation is superseded.

## Implementation Decisions

- Replaced `1 - e^4` with `1 - smoothstep(0, 0.20, abs(e))`, evaluated against the unperturbed broad constraint. This gives coastline noise smooth compact support and exactly zero influence beyond the boundary band.
- Retained task 053's fixed `16, 32, 64` samples, `1, 1/2, 1/4` weights, `0.01` default, and `0..0.1` control range.
- Retained the unperturbed broad constraint on each generated cell as `relief_constraint`.
- Used `relief_constraint` for ridge and valley masks, hill-to-mountain blending, extension masks, and inland rise.
- Used positive `relief_constraint` sites as stable mountain-distance sources while continuing to export visible peak markers only for sites that remain land after coastline perturbation.
- Kept the coast-perturbed `landmass_elevation` as the exported topology field and water-depth input.
- Replaced the test that required mid-range inland perturbation with deterministic locality coverage across three seeds.

## Testing

- Passed focused greater-realm procgen locality tests after the final invariant assertion.
- Passed staged-regeneration equivalence tests.
- Passed mountain peak distance-field tests.
- Passed `ctest --test-dir out/build/debug-with-tests --output-on-failure` (14/14 tests).
- Passed tests-disabled `debug-no-tests` and `release-no-tests` application builds.
- Passed `git diff --check`.

## Commit Message

Localize coastline detail to the shoreline boundary
Keep inland relief and mountain sources independent from coastline noise
Test deterministic coastline locality across representative seeds
