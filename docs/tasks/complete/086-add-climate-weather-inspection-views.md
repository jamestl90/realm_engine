# Add Climate And Weather Inspection Views

Status: complete
Priority: high
Area: World Simulation / Procgen Inspection

## Goal

Expose stable climate and seasonal tendencies in the greater-realm inspection application so developers and users can visually compare generated environmental layers in Debug and Release builds.

## Context

Tasks 074-080 implement annual climate normals, stable biomes, and seasonal temperature and precipitation. Existing greater-realm inspection views show annual temperature, annual precipitation, and biomes only.

## Dependencies

- Task 078: climate, season, and weather layer contract.
- Tasks 079-080: seasonal climate foundation.
- Task 085: procgen inspection views available in Release builds.

## Acceptance Criteria

- Add inspection views for seasonal temperature and seasonal precipitation.
- Keep stable annual temperature, precipitation, terrain, hydrology, and biome views unchanged.
- Add an explicit deterministic year-fraction control without hidden frame-time advancement.
- Keep visualization and controls application-owned; do not make procgen depend on mutable world state.
- Do not regenerate or relabel biomes when seasonal controls change.
- Add focused rendering/control tests and update canonical architecture/procgen documentation.
- Verify Debug and Release builds and the full test suite.

## Out Of Scope

- Runtime runoff, soil moisture, snowpack, discharge, flooding, or erosion.
- Weather fronts, storm entities, long-duration events, particles, or audio.
- A world calendar, automatic time progression, or frame-rate-driven climate.
- Retuning annual climate, seasonal profiles, or biome rules.

## Implementation Decisions

- Added application-owned `GreaterRealmInspectionView` and `GreaterRealmInspectionSettings` rather than extending `procgen::GreaterRealmDebugView` with mutable world-simulation concepts.
- The unified selector retains every existing stable terrain/climate/biome view and adds seasonal temperature and seasonal precipitation.
- Added an application-side climate image compositor. It validates terrain, annual climate, and seasonal provenance before rendering and reuses procgen's public geographic-overlay pass afterward.
- `TestApp` owns seasonal settings/caches/maps. Terrain or annual-climate regeneration rebuilds those derived layers; inspection year controls never invoke terrain, climate-normal, hydrology, or biome generation.
- The year slider supplies explicit normalized calendar input. No inspection time advances from frame time.
- Existing coast, peak, river, and drainage overlays remain available across stable and seasonal views. Flat and `3D` presentation continue to share the same retained RGBA inspection texture.

## Verification

- Built every Debug test executable, including the new climate/weather compositor test and the updated greater-realm inspector panel test.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 17/17.
- `scripts/build.ps1 -Preset debug-no-tests` passed and compiled the unified inspection selector and climate/weather compositor into the Debug executable.
- `scripts/build.ps1 -Preset release-no-tests` passed and compiled the same inspection sources into the Release executable.
- Release visual inspection at the application's 1920x1080 logical layout confirmed the year-fraction control remains readable, does not overlap the unified selector, and leaves the retained greater-realm map visible.
- Focused tests confirmed every seasonal view produces a complete deterministic image, stale source data is rejected, and inspection time controls never invoke stable procgen regeneration.
- Runtime-weather inspection views from the original implementation were removed by the later climate simplification.
- `git diff --check` passed with only the repository's existing LF-to-CRLF warnings.

## Commit Message

`feat(world): add climate and weather inspection views`
