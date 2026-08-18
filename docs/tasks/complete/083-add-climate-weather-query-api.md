# Add Climate And Weather Query API

Status: complete
Priority: high
Area: World Simulation / Gameplay Queries

## Goal

Provide a clear query surface for stable climate, seasonal tendency, experienced conditions, and active precipitation so gameplay systems do not reach through procgen, seasonal, and weather ownership boundaries.

## Context

Task 078 defines three distinct environmental layers. Once seasonal evaluation and runtime weather exist, consumers need explicit APIs that distinguish long-term biome-driving climate from current weather and from composed experienced conditions.

## Dependencies

- Task 079: seasonal temperature evaluation.
- Task 080: seasonal precipitation evaluation.
- Task 081: runtime atmospheric state contract.
- Task 082: deterministic weather evolution.

## Acceptance Criteria

- Add query helpers or interfaces for annual climate normals, seasonal temperature/precipitation tendency, runtime atmospheric state, experienced temperature, experienced precipitation, and active precipitation.
- Make each query's determinism, time input, coordinate space, spatial interpolation, and ownership boundary explicit.
- Ensure biome generation and biome queries continue to use only stable terrain and annual climate normals unless an application deliberately adds a separate temporary overlay outside biome identity.
- Keep query composition free of hidden global time and frame-rate coupling.
- Add tests for coordinate sampling, deterministic time input, composition order, missing-weather fallback behavior, and biome/procgen immutability.
- Update canonical architecture/procgen documentation with the gameplay-facing query contract.

## Out Of Scope

- New weather simulation behavior beyond the state/evolution tasks.
- Runtime runoff, discharge, flooding, erosion, or terrain mutation.
- Weather rendering, particles, audio, or presentation.
- Dynamic biome regeneration or biome relabeling.

## Implementation Decisions

- Added `world::ClimateWeatherSample` and `sample_climate_weather_cell`.
- Query samples expose stable annual temperature/precipitation normals, seasonal temperature offset, seasonal temperature normal, seasonal precipitation multiplier, seasonal precipitation tendency, runtime atmospheric fields, active precipitation intensity/type, composed experienced temperature/precipitation, and stable biome ID.
- Missing seasonal or weather inputs fall back to annual normals and calm/currently dry runtime conditions.
- Query composition is explicit: annual climate remains stable, seasonal maps are deterministic calendar inputs, runtime weather provides current anomalies/events, and biome ID is read from the stable biome map only.
- The query API does not read hidden global time, mutate any layer, or regenerate/relabel biomes.

## Commit Message

`feat(world): add climate weather query samples`

## Verification

- Added climate/weather tests for annual-only fallback sampling, seasonal composition, runtime weather composition, active precipitation exposure, stable biome ID sampling, and normalized experienced-condition bounds.
- `ctest --test-dir out/build/debug-with-tests -R procgen_greater_realm_climate --output-on-failure` passed, 1/1.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- `scripts/build.ps1 -Preset release-no-tests` passed.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF warnings.
