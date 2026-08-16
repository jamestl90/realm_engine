# Add Greater-Realm Temperature Normals

Status: todo
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
