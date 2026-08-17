# Add Seasonal Temperature Evaluation

Status: todo
Priority: high
Area: World Simulation / Climate

## Goal

Evaluate predictable seasonal temperature offsets from stable greater-realm temperature normals without changing terrain, annual climate normals, or biome assignments.

## Context

Task 078 separates annual climate normals from seasonal climate and transient weather. Temperature is the first seasonal slice because it can prove deterministic time inputs, amplitude/phase ownership, and experienced-temperature composition without requiring runtime weather simulation.

## Dependencies

- Task 074: greater-realm temperature normals.
- Task 078: climate, season, and weather layer contract.

## Acceptance Criteria

- Define seasonal temperature settings with amplitude, phase, hemisphere/latitude behavior, optional elevation or maritime modulation, validation rules, and deterministic defaults.
- Consume annual `temperature_normal`, map coordinates, explicit calendar input, and a seed/profile identity; do not consume frame time.
- Produce a seasonal temperature offset or seasonal temperature sample that can be composed with annual normal and future weather anomaly.
- Preserve existing greater-realm terrain, climate-normal, precipitation, biome, drainage, and debug-output behavior when seasonal settings change.
- Define versioning, invalidation, spatial resolution, cache behavior, and persistence policy for seasonal temperature data.
- Add tests for determinism, calendar repeatability, hemisphere phase opposition, amplitude bounds, source invalidation, and biome immutability.
- Update canonical architecture/procgen documentation with the implemented seasonal-temperature contract.

## Out Of Scope

- Runtime pressure, wind, humidity, clouds, storms, or active precipitation.
- Seasonal precipitation.
- Runtime runoff, discharge, flooding, erosion, or terrain mutation.
- Dynamic biome regeneration or biome relabeling.

