# Add Greater-Realm Temperature Normals

Status: complete
Priority: high
Area: Procgen / Greater Realm Climate

## Goal

Add a separate greater-realm climate data contract and generate stable normalized temperature tendencies from terrain and explicit latitude context.

## Context

Task 066 assigns timeless climate normals to procgen while keeping them out of canonical terrain cells and runtime weather. Temperature is the first climate layer because it has clear stable inputs and establishes the climate-map ownership and regeneration contract needed by later precipitation and biome work.

## Dependencies

- Task 039: generated geography remains separate from runtime weather.
- Task 064: stable ocean and inland-water classification.
- Task 066: climate and biome ownership contract.

## Acceptance Criteria

- Add `GreaterRealmClimateMap` and per-cell climate data with source-map dimensions and identity checks.
- Add explicit north-edge and south-edge latitude settings with validated geographic ranges and map-row interpolation.
- Generate fixed-scale `temperature_normal` values in `0..1` without per-map renormalization.
- Derive temperature from absolute latitude, elevation cooling, water-proximity moderation, and optional broad seed-domain variation with documented weights.
- Define values for both land and water cells so the field remains spatially continuous and usable by application rules.
- Keep temperature generation separate from `GreaterRealmCell`, terrain settings, terrain regeneration, hydrology, runtime weather, and application biome labels.
- Add dependency-aware climate invalidation so terrain or temperature-setting changes rebuild temperature output, while debug-view changes rebuild visualization only.
- Add a compile-gated temperature-normal debug view and summary statistics.
- Add tests for shape, fixed range, determinism, latitude response, elevation response, maritime moderation, seed-domain behavior, terrain immutability, and regeneration locality.
- Update `docs/ARCHITECTURE.md` and `docs/PROCGEN.md` with the implemented data contract and defaults.

## Notes

No branch is required unless implementation reveals that the new derived-map cache must change broader procgen ownership.

## Implementation Decisions

- Added a versioned `GreaterRealmClimateMap` with one `GreaterRealmClimateCell` per source terrain cell. It stores source seed, dimensions, cell size, and a deterministic fingerprint of the elevation and water fields consumed by temperature generation.
- Kept climate output and settings in `Climate.hpp`; no climate fields or controls were added to `GreaterRealmCell` or `GreaterRealmGeneratorSettings`.
- Clamped north/south edge latitudes to `-90..+90` and linearly interpolated map rows between them. Defaults cover `+60` at the north edge through `-60` at the south edge.
- Used fixed temperature anchors: absolute latitude establishes the baseline, normalized land height applies `0.35` cooling, water proximity applies `0.20` moderation toward `0.5` over `16` map units, and domain-separated three-octave noise adds optional `0.08` broad variation at frequency `2.5`.
- Included both ocean and inland water as maritime sources and defined temperature values on water cells without treating water depth as altitude.
- Added a separate climate cache that detects setting and source-fingerprint changes. `TestApp` explicitly invalidates it after terrain-source stages, while debug view and overlay callbacks continue to rebuild only the retained image/texture.
- Added a compile-gated `Temperature normal` base view shared by flat and `3D` previews. Stale climate/terrain pairings are rejected, and the panel reports fixed-scale mean and observed range statistics.

## Verification

- Added `realm_climate_tests` for shape, range, determinism, latitude validation/response, elevation cooling, maritime moderation, seed-domain behavior, source identity, terrain immutability, stale debug data, and regeneration locality.
- Focused greater-realm, climate, and debug-panel tests passed, 3/3.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 15/15.
- The explicit Debug test/runtime target build passed for all 16 targets.
- `scripts/build.ps1 -Preset debug-no-tests` passed with the climate debug view enabled.
- `scripts/build.ps1 -Preset release-no-tests` passed with debug visualization excluded and the climate module retained.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF conversion warnings.
