# Add Runtime Weather And Runoff

Status: idea
Priority: deferred
Area: World Simulation / Weather

## Goal

Simulate weather events at runtime and route precipitation through generated drainage data without mutating generated geography.

## Context

Greater-realm generation owns stable terrain, catchment area, and potential river channels. Rainfall, humidity, and active river discharge are time-varying world state and should be produced by a separate simulation that can account for seasons, biomes, and current weather events.

Task 078 now defines the required layer boundary: annual climate normals remain stable procgen data, seasonal climate is deterministic calendar-scale evaluation, and transient weather is mutable runtime atmospheric state. This idea keeps the later runoff and discharge scope parked rather than absorbing the seasonal/weather foundation tasks.

## Dependencies

- Task 039: terrain-only drainage and potential river channels.
- Task 070: active world-region lifecycle.
- Task 071: region persistence and regeneration deltas.
- Task 074: greater-realm temperature normals.
- Task 075: greater-realm precipitation normals.
- Task 076: application-driven greater-realm biome assignment.
- Task 078: climate, season, and weather layer contract.
- Task 079: seasonal temperature evaluation.
- Task 080: seasonal precipitation evaluation.
- Task 081: runtime atmospheric state contract.
- Task 082: deterministic weather evolution.
- Task 083: climate and weather query API.

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

Runtime runoff, soil moisture, snowpack, active discharge, flooding, and erosion-adjacent behavior are not current procgen priorities. Reassess this idea only after world regions, seasonal tendencies, runtime atmospheric state, weather evolution, and gameplay-facing weather queries exist and a concrete non-disaster gameplay requirement justifies the simulation cost.

Task 078 reconciles this idea by splitting prerequisite climate/weather layers into todo tasks while leaving the runoff/discharge simulation parked here. Do not implement this idea by adding generated humidity, current rainfall, soil moisture, runoff, or discharge fields to greater-realm procgen.
