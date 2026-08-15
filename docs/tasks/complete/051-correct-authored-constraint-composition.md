# Correct Authored Constraint Composition

Status: complete
Priority: high
Area: Procedural Generation / Terrain Constraints

## Goal

Make each authored terrain constraint influence the signed terrain pipeline once, with consistent ocean, shallow-water, valley, and mountain semantics.

## Context

Task 049 confirmed that an authored value is first blended into the broad signed constraint and then applied a second time as a direct final-relief target. Mapgen4 samples one signed constraint value, perturbs its coastline, and uses its positive magnitude to select local relief; it does not directly blend the same painted value back into final relief afterward.

## Acceptance Criteria

- Remove the second direct final-relief interpolation of authored signed values.
- Route automatic and authored signed constraints through the same coast and land-relief semantics.
- Preserve sparse unpainted influence, independent constraint resolution, deterministic interpolation, and the serialized format unless a versioned change is necessary.
- Verify each tool's topology and relief responsibility at its center, brush shoulder, and outside the brush.
- Ensure local edits do not cause unexplained distant elevation changes.
- Add representative fixed-seed tests for spatial locality, independence, monotonic brush response, and one-stage semantics that would fail under the current double-application behavior.
- Update `docs/PROCGEN.md` and run all procgen tests plus tests-disabled Debug and Release builds.

## Dependencies

- Task 050's corrected land-relief composition.

## Reference

- Mapgen4 `painting.ts` paint values and blending.
- Mapgen4 `map.ts` signed constraint sampling and elevation assignment.

## Implementation Summary

- Audited the current task-050 relief pipeline and confirmed authored constraints are sampled once, blended into `broad_constraint`, perturbed through the same coastline-noise path as automatic terrain, and then consumed through the signed landmass field for water depth and positive-land relief selection.
- Confirmed there is no remaining second direct final-relief interpolation of authored signed values.
- Added fixed-seed regression coverage for Ocean, Shallow, Valley, and Mountain tool semantics at the brush center, brush shoulder, and outside the brush.
- Added a one-stage authored-mountain regression where mountain strength is zero; this proves the painted mountain value cannot bypass the hill/mountain relief pipeline and force final elevation directly.
- Added monotonic brush-strength coverage for authored mountain constraints while preserving exact distant signed/final elevation outside the brush influence.
- Preserved the existing constraint serialization format.
- Updated `docs/PROCGEN.md` with the one-stage authored-constraint composition rule and tool responsibilities.

## Testing

- Passed: `cmake --build out/build/debug-with-tests --target realm_engine_tests realm_procgen_pipeline_tests`
- Passed: `ctest --test-dir out/build/debug-with-tests -R procgen_hydrology_constraints --output-on-failure`
- Passed: `ctest --test-dir out/build/debug-with-tests --output-on-failure` (13/13)
- Passed: `cmake --build --preset debug-no-tests`
- Passed: `cmake --build --preset release-no-tests`

## Commit Message

Verify authored constraints use one-stage composition
