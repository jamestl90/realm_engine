# Add Procgen Parameter Contract Tests

Status: todo
Priority: medium
Area: Procedural Generation / Testing

## Goal

Test what each generation parameter is responsible for, rather than merely proving that some output value changed.

## Context

Task 049 found strong coverage for determinism, storage, topology, and hydrology invariants, but several parameter tests accept any aggregate elevation difference above an arbitrary threshold. Those assertions can pass when a control changes the wrong cells, changes too little to matter visually, or introduces distant side effects.

## Acceptance Criteria

- Define locality, independence, monotonicity, topology, and visual-significance contracts for every public greater-realm parameter.
- Exercise representative fixed seeds and lower, default, and upper settings without relying on a single favorable map.
- Add spatial assertions for mountain, valley, ridge, terrain-noise, coastline, sea-level, island-bias, and ocean-depth controls.
- Add peak-distribution and authored-edit stability assertions after tasks 050 through 053 establish the intended behavior.
- Keep all characterization helpers and tests behind `REALM_BUILD_TESTS`; no audit or profiling code may enter tests-disabled or Release builds.
- Document only stable parameter responsibilities in `docs/PROCGEN.md`.

## Dependencies

- Tasks 050, 051, 052, and 053.
