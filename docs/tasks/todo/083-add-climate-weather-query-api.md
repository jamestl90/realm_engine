# Add Climate And Weather Query API

Status: todo
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

