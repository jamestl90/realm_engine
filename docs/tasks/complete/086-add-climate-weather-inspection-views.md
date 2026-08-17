# Add Climate And Weather Inspection Views

Status: complete
Priority: high
Area: World Simulation / Procgen Inspection

## Goal

Expose stable climate, seasonal tendencies, and transient runtime weather in the greater-realm inspection application so developers and users can visually compare generated environmental layers in Debug and Release builds.

## Context

Tasks 074-083 implement annual climate normals, stable biomes, seasonal temperature and precipitation, runtime atmospheric state, deterministic weather evolution, and composed queries. Existing greater-realm inspection views show annual temperature, annual precipitation, and biomes only.

## Dependencies

- Task 078: climate, season, and weather layer contract.
- Tasks 079-083: seasonal and runtime weather foundation.
- Task 085: procgen inspection views available in Release builds.

## Acceptance Criteria

- Add inspection views for seasonal temperature, seasonal precipitation, runtime temperature anomaly, pressure, runtime wind, humidity, cloud cover, active precipitation, experienced temperature, and experienced precipitation.
- Keep stable annual temperature, precipitation, terrain, hydrology, and biome views unchanged.
- Add explicit deterministic year-fraction and weather-tick controls without hidden frame-time advancement.
- Show runtime wind as runtime atmospheric state, not Task 077 climatological transport.
- Keep visualization and controls application-owned; do not make procgen depend on mutable weather state.
- Do not regenerate or relabel biomes when seasonal or weather controls change.
- Add focused rendering/control tests and update canonical architecture/procgen documentation.
- Verify Debug and Release builds and the full test suite.

## Out Of Scope

- Runtime runoff, soil moisture, snowpack, discharge, flooding, or erosion.
- Weather fronts, storm entities, long-duration events, particles, or audio.
- A world calendar, automatic time progression, or frame-rate-driven weather.
- Retuning annual climate, seasonal profiles, runtime weather, or biome rules.

## Implementation Decisions

- Added application-owned `GreaterRealmInspectionView` and `GreaterRealmInspectionSettings` rather than extending `procgen::GreaterRealmDebugView` with mutable world-simulation concepts.
- The unified selector retains every existing stable terrain/climate/biome view and adds seasonal temperature, seasonal precipitation, runtime temperature anomaly, pressure, runtime wind, humidity, cloud cover, active precipitation, experienced temperature, and experienced precipitation.
- Added an application-side climate/weather image compositor. It validates terrain, annual climate, seasonal, and runtime-weather provenance before rendering and reuses procgen's public geographic-overlay pass afterward.
- Runtime wind uses a wind-speed colour field plus deterministic sampled vector marks. It reads `RuntimeAtmosphericState::wind_x/wind_y` and never visualizes Task 077 climatological transport as persistent wind.
- `TestApp` owns seasonal settings/caches/maps and runtime weather inspection state. Terrain or annual-climate regeneration rebuilds those derived layers; inspection year/tick controls never invoke terrain, climate-normal, hydrology, or biome generation.
- The year slider supplies explicit normalized calendar input. Weather tick decrement/re-evaluation is stateless at the selected tick, while forward increments preserve compatible prior atmospheric state. No inspection time advances from frame time.
- Existing coast, peak, river, and drainage overlays remain available across stable, seasonal, and runtime views. Flat and `3D` presentation continue to share the same retained RGBA inspection texture.

## Verification

- Built every Debug test executable, including the new climate/weather compositor test and the updated greater-realm inspector panel test.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 17/17.
- `scripts/build.ps1 -Preset debug-no-tests` passed and compiled the unified inspection selector and climate/weather compositor into the Debug executable.
- `scripts/build.ps1 -Preset release-no-tests` passed and compiled the same inspection sources into the Release executable.
- Release visual inspection at the application's 1920x1080 logical layout confirmed the new year-fraction and weather-tick controls remain readable in one row, do not overlap the unified selector, and leave the retained greater-realm map visible.
- Focused tests confirmed every seasonal/runtime view produces a complete deterministic image, runtime wind draws sampled vectors, stale source data is rejected, and inspection time controls never invoke stable procgen regeneration.
- `git diff --check` passed with only the repository's existing LF-to-CRLF warnings.

## Commit Message

`feat(world): add climate and weather inspection views`
