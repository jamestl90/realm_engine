# Classify Inland Water

Status: complete
Priority: high
Area: Procgen / Geography

## Goal

Distinguish boundary-connected ocean from enclosed inland water in greater-realm terrain data and debug visualization.

## Context

Greater-realm generation already exports separate `is_water` and `is_ocean` flags, but every water cell is currently assigned the `Ocean` terrain form. This loses a distinction the generator has already computed and makes authored enclosed water look like ocean to downstream systems.

This task adds semantic classification only. It does not add lake filling, water levels, seasonal behavior, runoff, erosion, or local shoreline detail.

## Dependencies

- Task 016: greater-realm terrain classification and ocean connectivity.
- Task 039: stable geography remains separate from runtime weather and discharge.

## Acceptance Criteria

- Add an inland-water terrain form for water cells that are not boundary-connected ocean.
- Preserve `is_water` as the general water predicate and `is_ocean` as the boundary-connectivity predicate.
- Keep elevation, land/water topology, drainage topology, and potential river channels unchanged.
- Represent ocean and inland water distinctly in terrain-form statistics and debug visualization.
- Add deterministic tests covering boundary-connected water, enclosed generated or authored water, terrain-form strings, and unchanged land classification.
- Update `docs/PROCGEN.md` with the supported classification and its limits.
- Run focused procgen tests, full CTest, and Debug/Release builds.

## Notes

Use a neutral engine term such as `InlandWater` if `Lake` would imply hydrological behavior that the generator does not yet model.

No branch is required; this is a narrow extension of existing classification data.

## Implementation Decisions

- Added `TerrainForm::InlandWater` while retaining `is_water` as the general water flag and `is_ocean` as the boundary-connectivity flag.
- Used `InlandWater` rather than `Lake` because classification alone does not model lake retention, shared surface levels, or runtime water behavior.
- Kept terrain generation, elevation, ocean connectivity, drainage, and channel construction unchanged; only the classification derived from existing flags changed.
- Added separate inland-water statistics and a depth-aware teal debug palette distinct from ocean blue.
- Updated the debug-panel coverage summary so land excludes both water forms and inland water is reported separately.

## Verification

- Focused `procgen_greater_realm` and `ui_greater_realm_debug_panel` tests passed.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 14/14.
- `cmake --build --preset debug-with-tests` passed.
- `cmake --build --preset debug-no-tests` passed.
- `cmake --build --preset release-no-tests` passed.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF conversion warnings.
