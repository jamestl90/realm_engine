# Add Seasonal Precipitation Evaluation

Status: todo
Priority: high
Area: World Simulation / Climate

## Goal

Evaluate predictable wet and dry seasonal tendencies from stable greater-realm precipitation normals without treating climatological transport as runtime wind.

## Context

Task 078 defines annual precipitation normal as long-term climate and seasonal precipitation as a deterministic calendar-scale modulation. Task 077 circulation remains climatological moisture transport only; this task must not introduce persistent runtime airflow.

## Dependencies

- Task 075: greater-realm precipitation normals.
- Task 077: seed-varied climatological moisture transport.
- Task 078: climate, season, and weather layer contract.
- Task 079: seasonal temperature evaluation, for shared calendar/profile patterns where useful.

## Acceptance Criteria

- Define seasonal precipitation settings with amplitude, phase, wet/dry profile controls, validation rules, and deterministic defaults.
- Consume annual `precipitation_normal`, explicit calendar input, map coordinates, and a seed/profile identity; do not consume frame time.
- Produce a seasonal precipitation tendency or multiplier that future weather can use for event probability and intensity.
- Preserve annual precipitation normals, terrain-only hydrology, potential river channels, biome assignment, and current debug output when seasonal settings change.
- Define versioning, invalidation, spatial resolution, cache behavior, and persistence policy for seasonal precipitation data.
- Add tests for determinism, calendar repeatability, amplitude bounds, wet/dry phase behavior, source invalidation, and biome immutability.
- Update canonical architecture/procgen documentation with the implemented seasonal-precipitation contract.

## Out Of Scope

- Active rainfall events, storms, pressure, humidity, clouds, or runtime wind.
- Runtime runoff, discharge, flooding, erosion, or terrain mutation.
- Replacing Task 077 climatological transport with weather simulation.
- Dynamic biome regeneration or biome relabeling.

