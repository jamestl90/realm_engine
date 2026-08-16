# Add Greater-Realm Precipitation Normals

Status: todo
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
