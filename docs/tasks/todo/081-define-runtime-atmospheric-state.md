# Define Runtime Atmospheric State

Status: todo
Priority: high
Area: World Simulation / Weather

## Goal

Define the mutable runtime atmospheric state contract, persistence identity, spatial resolution, and gameplay query surface before implementing weather evolution.

## Context

Task 078 separates transient weather from annual climate normals and seasonal evaluation. Runtime systems need explicit ownership for current temperature anomaly, pressure, wind, humidity, clouds, and active precipitation so future weather does not leak into procgen or biome generation.

## Dependencies

- Task 078: climate, season, and weather layer contract.
- Task 079: seasonal temperature evaluation.
- Task 080: seasonal precipitation evaluation.
- Task 070: active world-region lifecycle.
- Task 071: world-region persistence and regeneration deltas.

## Acceptance Criteria

- Define runtime atmospheric fields, including current temperature anomaly, pressure, runtime wind vector, humidity, cloud cover, active precipitation intensity/type, and event/state metadata needed for continuity.
- Define weather-cell or region-scoped spatial resolution, interpolation rules, update cadence, and deterministic identity inputs.
- Define persistence schema/version expectations for active or modified streamed regions, including simulation timestamp and weather seed identity.
- Define invalidation and migration behavior when weather settings, climate sources, region identity, or schema versions change.
- Keep annual climate normals, seasonal samples, terrain drainage, potential river channels, and biome assignments immutable from runtime atmospheric state.
- Add tests for state validation, identity/version mismatch handling, deterministic addressing, serialization round trips if persistence is included, and biome/procgen immutability.
- Update canonical architecture/procgen documentation with the runtime-weather ownership contract.

## Out Of Scope

- Evolving pressure systems, fronts, storms, rainfall, or drought over time.
- Runtime runoff, discharge, flooding, erosion, or terrain mutation.
- Weather rendering, particles, audio, or presentation.
- Dynamic biome regeneration or biome relabeling.

