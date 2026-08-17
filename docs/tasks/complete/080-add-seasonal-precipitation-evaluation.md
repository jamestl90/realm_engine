# Add Seasonal Precipitation Evaluation

Status: complete
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

## Implementation Decisions

- Added seasonal precipitation to `world::SeasonalClimate` alongside seasonal temperature instead of extending `GreaterRealmClimateMap`.
- Added `SeasonalPrecipitationSettings`, `SeasonalPrecipitationMap`, `SeasonalPrecipitationCell`, and `SeasonalPrecipitationEvaluationCache`.
- Seasonal precipitation consumes annual `precipitation_normal`, terrain coordinates, explicit normalized year fraction, profile seed/identity, and seasonal settings. It never reads frame time and does not treat Task 077 climatological transport as runtime wind.
- Defaults use base multiplier amplitude `0.25`, latitude amplitude `0.15`, inland/elevation damping `0.10`, northern wet peak `0.00`, southern wet peak `0.50`, and multiplier bounds `0.25..1.75`.
- The output stores annual precipitation, a seasonal multiplier, and clamped composed seasonal precipitation tendency. It is a probability/intensity tendency for runtime weather, not current rainfall.
- The cache invalidates only seasonal precipitation when terrain identity, annual precipitation normals, seasonal settings/profile identity, or normalized year fraction changes. It does not dirty terrain, annual climate normals, hydrology, potential river channels, debug source data, or biome assignment.

## Commit Message

`feat(world): add seasonal precipitation evaluation`

## Verification

- Added climate tests for seasonal precipitation determinism, calendar repeatability, hemisphere wet/dry phase behavior, validation, cache invalidation, fixed-scale bounds, and biome immutability.
- `ctest --test-dir out/build/debug-with-tests -R procgen_greater_realm_climate --output-on-failure` passed, 1/1.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- `scripts/build.ps1 -Preset release-no-tests` passed.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF warnings.
