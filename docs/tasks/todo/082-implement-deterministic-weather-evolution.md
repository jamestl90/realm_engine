# Implement Deterministic Weather Evolution

Status: todo
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

