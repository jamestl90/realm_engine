# Add Greater Realm Procgen Foundation

Status: inprogress
Area: Procgen
Branch: `proc-gen`
Branch reason: This is a multi-step engine feature stream covering procedural world data, generation, visualization, and later validation tooling.

## Goal

Add the first engine-level procedural generation capability for greater realm landmass generation.

## Context

The current target is broad world shape only: continents, islands, coastlines, ocean versus land, elevation, mountains, highlands, valleys, lower terrain, and basic terrain-form classification.

The reference model is Red Blob Games' Mapgen4 style: terrain should come from a pipeline of layered influences rather than a single noise function.

Out of scope for this task: biomes, weather, clans, settlements, object placement, resources, local tile detail, rivers, and hydrology.

## Acceptance Criteria

- Define reusable greater realm procgen data types.
- Add deterministic generation from a seed.
- Produce elevation, water/ocean flags, coast distance, slope, and terrain-form classification.
- Keep the first generator focused on broad terrain form only.
- Add automated sanity tests for deterministic output and valid generated data.
- Keep test-only code outside the release game executable.

## Notes

Visual validation should follow as a separate slice so generated maps can be judged by eye before adding deeper world systems.
