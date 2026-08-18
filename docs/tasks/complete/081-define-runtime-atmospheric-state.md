# Define Runtime Atmospheric State

Status: complete
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

## Implementation Decisions

- Added `world::RuntimeAtmosphericState` and `RuntimeAtmosphericCell` in `world::Weather`.
- Runtime atmospheric cells own current temperature anomaly, pressure normal, runtime wind vector, humidity, cloud cover, active precipitation intensity, and active precipitation type.
- Runtime state identity stores version, schema version, weather seed, region identity, source dimensions/cell size, weather-cell size, source terrain/climate/seasonal fingerprints, settings fingerprint, and simulation tick.
- `RuntimeWeatherSettings` defines validation and identity inputs for weather seed, region identity, schema version, update cadence, weather-cell size, anomaly bounds, pressure/wind/humidity/cloud controls, precipitation threshold, precipitation intensity scale, and state memory.
- The current foundation samples one runtime atmospheric cell per greater-realm cell for deterministic coverage and tests. Later region work may introduce coarser weather cells and interpolation without changing the ownership boundary.
- Runtime atmospheric state is mutable world-simulation data. It does not mutate annual climate normals, seasonal maps, terrain drainage, potential river channels, or biome assignments.

## Commit Message

`feat(world): define runtime atmospheric state`

## Verification

- Added climate/weather tests for runtime atmospheric state identity, source matching, schema/seed/region identity, deterministic addressing, state-copy continuity, normalized field bounds, and procgen/biome immutability.
- `ctest --test-dir out/build/debug-with-tests -R procgen_greater_realm_climate --output-on-failure` passed, 1/1.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- `scripts/build.ps1 -Preset release-no-tests` passed.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF warnings.
