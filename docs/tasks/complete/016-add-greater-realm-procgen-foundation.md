# Add Greater Realm Procgen Foundation

Status: complete
Area: Procgen
Branch: `proc-gen`
Branch reason: This is a multi-step engine feature stream covering procedural world data, generation, visualization, and later validation tooling.

## Goal

Add the first engine-level procedural generation capability for greater realm landmass generation.

## Context

The target is broad world shape only: continents, islands, coastlines, ocean versus land, elevation, mountains, highlands, valleys, lower terrain, and basic terrain-form classification.

The reference model is Red Blob Games' Mapgen4 style: terrain should come from a pipeline of layered influences rather than a single noise function.

Out of scope for this task: biomes, weather, clans, settlements, object placement, resources, local tile detail, rivers, and hydrology.

## Acceptance Criteria

- Define reusable greater realm procgen data types.
- Add deterministic generation from a seed.
- Produce elevation, water/ocean flags, coast distance, slope, and terrain-form classification.
- Keep the first generator focused on broad terrain form only.
- Add automated sanity tests for deterministic output and valid generated data.
- Keep test-only code outside the release game executable.

## Verification

- Greater realm data and deterministic generation are implemented under `include/procgen` and `src/procgen`.
- Procgen tests cover deterministic output, valid map storage, topology, elevation, water/ocean classification, coast response, and terrain forms.
- The procgen test target passes.
- Debug and Release game builds pass, with test-only code excluded from the game executable.
- Subsequent visualization and tuning tasks have exercised the generated output through the runtime rendering path.

## Notes

Visual validation continued through separate tasks so generated maps could be judged independently from the generator foundation.
