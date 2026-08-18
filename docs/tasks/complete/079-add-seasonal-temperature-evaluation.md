# Add Seasonal Temperature Evaluation

Status: complete
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

## Implementation Decisions

- Added `world::SeasonalTemperatureSettings`, `SeasonalTemperatureMap`, `SeasonalTemperatureCell`, and `SeasonalTemperatureEvaluationCache` as a world-simulation climate module rather than adding seasonal fields to `GreaterRealmClimateMap`.
- Seasonal temperature consumes terrain identity, annual temperature normals, explicit settings/profile identity, map coordinates, and an explicit normalized year fraction. It never reads render frame time, wall-clock time, or engine global time.
- Defaults use normalized seasonal offsets: base amplitude `0.10`, latitude amplitude `0.16`, elevation amplitude `0.04`, maritime damping `0.35`, maritime influence distance `16`, north-edge latitude `+60`, south-edge latitude `-60`, northern peak year fraction `0.50`, and southern peak year fraction `0.00`.
- Optional regional phase and amplitude variation use `profile_seed`, `profile_identity`, and stable coordinates. Their strengths default to `0`, leaving the default seasonal evaluation profile-neutral.
- `SeasonalTemperatureMap` stores version `1`, source terrain identity, annual-temperature fingerprint, seasonal-settings fingerprint, normalized year fraction, and one output cell per greater-realm cell. Each cell carries annual temperature, signed seasonal offset, and clamped composed seasonal temperature.
- The cache invalidates only seasonal output when terrain identity, annual temperature normals, seasonal settings/profile identity, or normalized year fraction changes. It does not dirty procgen climate, precipitation, terrain, hydrology, debug source data, or biome assignment.
- Biomes continue to inspect only stable terrain and annual climate normals. Seasonal temperature is experienced-condition data for future queries, not biome identity.

## Verification

- Built the focused `realm_climate_tests` target after reconfiguring the debug-with-tests build tree for the new source file.
- `ctest --test-dir out/build/debug-with-tests -R procgen_greater_realm_climate --output-on-failure` passed, 1/1.
- Built the focused `realm_biome_tests` target; it was already up to date.
- `ctest --test-dir out/build/debug-with-tests -R procgen_greater_realm_biomes --output-on-failure` passed, 1/1.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- `scripts/build.ps1 -Preset release-no-tests` passed after CMake reconfigured for `src/world/SeasonalClimate.cpp`.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF warnings for touched Markdown, CMake, and test files.
- A targeted trailing-whitespace scan across the new and touched files found no matches.
- The initial direct `cmake --build out/build/debug-with-tests --target realm_climate_tests --config Debug` failed because the Visual Studio environment was not loaded and MSVC could not find standard library headers. Re-running the same build through `VsDevCmd.bat` succeeded.

## Commit Message

`feat(world): add seasonal temperature evaluation`
