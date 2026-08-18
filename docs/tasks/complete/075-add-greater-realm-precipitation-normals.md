# Add Greater-Realm Precipitation Normals

Status: complete
Priority: high
Area: Procgen / Greater Realm Climate

## Goal

Generate stable normalized precipitation tendencies for the greater-realm map without reintroducing current weather, humidity, soil moisture, runoff, or discharge.

## Context

Biome variation needs a long-term wet-to-dry climate axis. Task 028's superseded implementation stored transient-sounding humidity, rainfall, and moisture directly on terrain cells and coupled them to river flow. This task instead adds one clearly named climatological normal to the separate climate map.

## Dependencies

- Task 064: ocean and inland-water source classification.
- Task 074: greater-realm climate map and temperature-normal contract.

## Acceptance Criteria

- Add fixed-scale `precipitation_normal` values in `0..1` without per-map renormalization.
- Define explicit prevailing-wind and climatological transport settings with validated ranges and deterministic defaults.
- Treat ocean and inland water as stable moisture sources with separately documented influence.
- Account for elevation-driven orographic lift and downwind rain shadow at greater-realm scale.
- Keep the output independent from drainage area, river-channel thresholds, current rainfall events, humidity, soil moisture, runoff, and discharge.
- Preserve terrain, water topology, climate temperature output, drainage, and potential river channels when precipitation settings change.
- Extend dependency-aware climate invalidation so precipitation changes rebuild precipitation and biome-dependent output only.
- Add a compile-gated precipitation-normal debug view and summary statistics.
- Add tests for shape, fixed range, determinism, wind response, water-source response, orographic response, dry/wet setting response, terrain immutability, and regeneration locality.
- Update `docs/ARCHITECTURE.md` and `docs/PROCGEN.md` with the implemented algorithm boundary and defaults.

## Branch

Use a dedicated branch. Whole-map moisture transport and rain-shadow behavior require algorithm evaluation, visual tuning, and careful regression testing against the established weather boundary.

## Implementation Decisions

- Extended `GreaterRealmClimateCell` with fixed-scale `precipitation_normal` and incremented the climate data version.
- Added validated prevailing-wind and transport settings. Wind defaults west-to-east at `0` degrees; non-finite values restore defaults and directions wrap to `0..360`.
- Used a deterministic wind-projection ordering with aligned eight-neighbor upwind blending. Defaults are ambient moisture `0.18`, ocean source `1.0`, inland-water source `0.65`, and map-unit retention `0.985`.
- Converted transported moisture to background precipitation at `0.35`, added normalized-elevation lift at `1.50`, and carried lift-created rain shadow downwind with strength `0.70` and retention `0.92`.
- Added a `1.0` global precipitation scale for deterministic dry/wet realm variation. Final values clamp to `0..1` without per-map normalization.
- Kept the pass independent from drainage, channels, runtime rain, humidity, soil moisture, runoff, and discharge.
- Split climate cache invalidation into temperature and precipitation stages. Terrain changes rebuild both; settings local to either field preserve the other byte-for-byte.
- Added a `Precipitation normal` debug view with fixed dry/temperate/wet colours and combined temperature/precipitation summary statistics.

## Verification

- Extended `realm_climate_tests` for precipitation shape/range/determinism, setting validation, wind response, separate water-source strengths, orographic lift, downwind shadow, dry/wet scaling, hydrology independence, terrain immutability, and staged regeneration locality.
- Focused greater-realm, climate, biome, and debug-panel tests passed, 4/4.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- Explicit Debug builds passed for all runtime and test targets.
- `scripts/build.ps1 -Preset debug-no-tests` and `scripts/build.ps1 -Preset release-no-tests` passed.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF conversion warnings.
