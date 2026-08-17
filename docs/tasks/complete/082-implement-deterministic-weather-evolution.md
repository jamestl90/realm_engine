# Implement Deterministic Weather Evolution

Status: complete
Priority: high
Area: World Simulation / Weather

## Goal

Advance runtime atmospheric state deterministically from explicit seeds, simulation time, stable climate normals, seasonal tendencies, and prior weather state.

## Context

Task 081 defines the runtime atmospheric state container. This task adds the evolution step that creates changing conditions such as heatwaves, cold snaps, storms, dry spells, wind shifts, humidity changes, cloud cover, and active precipitation without modifying procgen or biome output.

## Dependencies

- Task 079: seasonal temperature evaluation.
- Task 080: seasonal precipitation evaluation.
- Task 081: runtime atmospheric state contract.

## Acceptance Criteria

- Advance weather on a fixed simulation cadence or explicit event schedule independent from render frame rate.
- Consume weather seed/domain, simulation tick or timestamp, weather-cell/region identity, prior persisted state, annual climate normals, and seasonal tendencies.
- Update current temperature anomaly, pressure, runtime wind, humidity, cloud cover, and active precipitation in a deterministic and reproducible way.
- Allow warm regions to experience cold snaps and cool regions to experience heatwaves without changing annual climate identity or biome assignment.
- Keep active precipitation available for future runoff/discharge while preserving terrain-only drainage topology, catchment area, potential channel geometry, and annual precipitation normals.
- Add tests for deterministic replay, frame-rate independence, save/load continuity, source invalidation, plausible bounds, and biome/procgen immutability.
- Update canonical architecture/procgen documentation with the implemented weather-evolution contract.

## Out Of Scope

- Runtime runoff, river discharge, flooding, erosion, or terrain mutation.
- Weather rendering, particles, audio, or presentation.
- Dynamic biome regeneration or biome relabeling.

## Implementation Decisions

- Added `evolve_runtime_weather` as a deterministic weather evolution function driven by explicit simulation tick input.
- Evolution consumes terrain, annual climate normals, seasonal temperature, seasonal precipitation, runtime weather settings, simulation tick, and optional previous runtime state for continuity.
- Temperature anomaly, pressure, wind, humidity, cloud cover, active precipitation intensity, and rain/snow type are derived from domain-separated deterministic weather fields plus seasonal tendencies.
- The optional previous state is blended only when schema, source, season, settings, seed, region, and timestamp identity are compatible. A save/load-style copy therefore preserves continuity.
- Runtime precipitation is exposed for future runoff/discharge, but this task does not route runoff, change discharge, mutate terrain drainage, or alter potential channels.
- Weather evolution can create heatwaves and cold snaps relative to annual/seasonal climate without changing annual climate identity or biome assignments.

## Commit Message

`feat(world): evolve deterministic runtime weather`

## Verification

- Added climate/weather tests for deterministic replay from explicit simulation ticks, save/load-style continuity from prior state copies, plausible bounds, active precipitation exposure, heatwaves/cold snaps relative to baseline climate, and biome immutability.
- `ctest --test-dir out/build/debug-with-tests -R procgen_greater_realm_climate --output-on-failure` passed, 1/1.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- `scripts/build.ps1 -Preset release-no-tests` passed.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF warnings.
