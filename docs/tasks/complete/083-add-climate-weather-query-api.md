# Add Climate And Weather Query API

Status: complete
Priority: high
Area: World Simulation / Gameplay Queries

## Goal

Provide a clear query surface for stable climate and seasonal tendency so gameplay systems do not reach through procgen and seasonal ownership boundaries.

## Context

Task 078 originally defined climate, seasonal, and runtime-weather layers. Runtime weather has since been removed from the active engine; the remaining useful query contract distinguishes long-term biome-driving climate from deterministic seasonal tendencies.

## Dependencies

- Task 079: seasonal temperature evaluation.
- Task 080: seasonal precipitation evaluation.

## Acceptance Criteria

- Add query helpers or interfaces for annual climate normals and seasonal temperature/precipitation tendency.
- Make each query's determinism, time input, coordinate space, spatial interpolation, and ownership boundary explicit.
- Ensure biome generation and biome queries continue to use only stable terrain and annual climate normals unless an application deliberately adds a separate temporary overlay outside biome identity.
- Keep query composition free of hidden global time and frame-rate coupling.
- Add tests for coordinate sampling, deterministic time input, composition order, missing-seasonal fallback behavior, and biome/procgen immutability.
- Update canonical architecture/procgen documentation with the gameplay-facing query contract.

## Out Of Scope

- Runtime weather simulation behavior.
- Runtime runoff, discharge, flooding, erosion, or terrain mutation.
- Weather rendering, particles, audio, or presentation.
- Dynamic biome regeneration or biome relabeling.

## Implementation Decisions

- Added `world::ClimateWeatherSample` and `sample_climate_weather_cell`.
- Query samples expose stable annual temperature/precipitation normals, seasonal temperature offset, seasonal temperature normal, seasonal precipitation multiplier, seasonal precipitation tendency, and stable biome ID.
- Missing seasonal inputs fall back to annual normals.
- Query composition is explicit: annual climate remains stable, seasonal maps are deterministic calendar inputs, and biome ID is read from the stable biome map only.
- The query API does not read hidden global time, mutate any layer, or regenerate/relabel biomes.

## Commit Message

`feat(world): add climate weather query samples`

## Verification

- Added climate/weather tests for annual-only fallback sampling, seasonal composition, stable biome ID sampling, and biome/procgen immutability.
- Runtime-weather query fields from the original implementation were removed by the later climate simplification.
- `ctest --test-dir out/build/debug-with-tests -R procgen_greater_realm_climate --output-on-failure` passed, 1/1.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- `scripts/build.ps1 -Preset release-no-tests` passed.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF warnings.
