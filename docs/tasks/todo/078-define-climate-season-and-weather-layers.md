# Define Climate, Season, And Weather Layers

Status: todo
Priority: high
Area: Procgen / World Simulation Architecture

## Goal

Define a clean ownership and data-flow boundary between stable biome-driving climate normals, predictable seasonal climate, and changing runtime weather without altering the current greater-realm biome output.

## Context

Tasks 074-077 established stable temperature and precipitation normals for greater-realm biome generation. Their circulation is climatological moisture-transport machinery, not literal wind that remains fixed during play.

The intended world should support generally warm or cool regions while still allowing seasons, heatwaves, cold snaps, storms, drought, and other temporary conditions. Stable climate must continue to describe what a place is usually like; runtime weather must describe what is happening there now.

## Dependencies

- Task 066: climate and biome ownership boundary.
- Tasks 074-077: stable temperature, precipitation, biome, circulation, and aridity generation.
- Task 040: parked runtime weather and runoff proposal, whose eventual implementation should consume this contract.

## Acceptance Criteria

- Define three distinct layers: annual climate normals, seasonal climate evaluation, and transient runtime weather.
- Specify the fields owned by each layer, including annual temperature and precipitation, seasonal amplitude and phase, current temperature anomaly, pressure, wind, humidity, cloud, and active precipitation where appropriate.
- Define the composition of experienced temperature and precipitation from stable, seasonal, transient, and local influences.
- Keep biome assignment dependent only on stable long-term climate and terrain inputs; seasons and short-term weather must not regenerate or relabel biomes.
- Reframe procgen circulation as climatological moisture transport rather than persistent runtime wind, documenting whether public naming should change in a later compatibility task.
- Define ownership, versioning, invalidation, persistence, spatial resolution, and update cadence for every layer.
- Define deterministic seed and time inputs so seasonal and weather behavior can be reproduced without coupling it to frame rate.
- Identify boundaries with hydrology: active rainfall may feed future runoff and discharge, but must not rewrite terrain-only drainage topology or annual precipitation normals.
- Describe how warm regions can experience seasonal or transient cold and cool regions can experience heatwaves without destroying their baseline climate identity.
- Produce an ordered implementation breakdown for seasonal temperature, seasonal precipitation, runtime atmospheric state, weather evolution, and gameplay-facing queries.
- Preserve current greater-realm generation and biome output behavior exactly; this task introduces no weather simulation or biome retuning.
- Update the canonical architecture/procgen documentation and verify it remains consistent with Task 040 and the existing climate contracts.

## Out Of Scope

- Implementing a world calendar or seasons.
- Simulating pressure systems, fronts, clouds, wind, rainfall, storms, drought, or temperature anomalies.
- Runtime runoff, river discharge, flooding, erosion, or terrain mutation.
- Dynamic biome transitions or biome regeneration from weather.
- Local-region generation, rendering, particles, audio, or weather presentation.

## Notes

This is an architecture and task-breakdown change. A dedicated branch is not required unless implementation expands into runtime data structures or simulation code.
