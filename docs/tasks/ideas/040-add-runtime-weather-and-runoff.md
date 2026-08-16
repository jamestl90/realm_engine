# Add Runtime Weather And Runoff

Status: idea
Priority: deferred
Area: World Simulation / Weather

## Goal

Simulate weather events at runtime and route precipitation through generated drainage data without mutating generated geography.

## Context

Greater-realm generation owns stable terrain, catchment area, and potential river channels. Rainfall, humidity, and active river discharge are time-varying world state and should be produced by a separate simulation that can account for seasons, biomes, and current weather events.

## Dependencies

- Task 039: terrain-only drainage and potential river channels.
- Task 070: active world-region lifecycle.
- Task 071: region persistence and regeneration deltas.
- Task 066 and its resulting biome or environmental-tendency tasks.

This task is intentionally deferred until those dependencies exist; static procgen must not grow placeholder weather fields in the meantime.

## Acceptance Criteria

- Represent regional weather as runtime state rather than generated cell fields.
- Support precipitation events with configurable duration and intensity.
- Route runoff through existing downslope and catchment topology.
- Represent transient river discharge separately from static channel geometry.
- Allow biome, season, and region data to influence weather probability without predetermining events.
- Define persistence and update-frequency rules suitable for streamed world regions.
- Add deterministic simulation tests using an explicit weather seed or event input.

## Branch

Use a dedicated branch when this task starts. It spans procgen data, world simulation, persistence, and streamed-region updates, so it is expected to require multi-day implementation and review.

## Parking Decision

Runtime weather, runoff, and discharge are not current procgen priorities. Reassess this idea only after world regions and biome tendencies exist and a concrete non-disaster gameplay requirement justifies the simulation cost.
