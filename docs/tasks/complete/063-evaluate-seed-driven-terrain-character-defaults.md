# Evaluate Seed-Driven Terrain Character Defaults

Status: complete
Priority: medium
Area: Procedural Generation / Terrain Tuning

## Goal

Decide whether task 060's seed-driven terrain character should remain enabled by default, be retuned, be made opt-in, or be removed before downstream world-generation work treats it as settled behavior.

## Context

Task 060 added deterministic terrain ruggedness derived from the seed. `seed_terrain_variation` blends from exact neutral legacy generation at `0` to full seed-driven character at `1`, and the neutral path intentionally preserves prior generation scale factors exactly.

The feature produces stronger personality differences between seeds, but its overall value for the intended top-down 2D world is still under consideration. Before adding world regions, placement, biomes, resources, or local tile handoff on top of greater-realm output, the engine should decide whether seed character is a stable generation contract or an experimental debug control.

## Acceptance Criteria

- Compare neutral, partial, and full seed variation across representative seeds.
- Include deliberately flat, ordinary, and rugged seeds discovered by the task 060 test approach.
- Evaluate whether ruggedness changes produce useful top-down world variety without making terrain feel globally over-mountainous or too visually noisy.
- Preserve the guarantee that `seed_terrain_variation = 0` restores exact neutral scale factors and topology behavior.
- Decide the default value for `seed_terrain_variation`.
- Decide whether the debug UI should present seed variation as a normal tuning control, an experimental control, or hide it behind a narrower workflow.
- Add or update tests if the default, range, or scale curves change.
- Update `docs/PROCGEN.md` and any affected task records with the decision.
- Run focused procgen tests, full CTest, and Debug/Release builds after any code or default changes.

## Notes

This is a decision and tuning task. Do not expand it into biomes, resource placement, weather, or local terrain generation.

No branch is required unless evaluation leads to broad generation retuning beyond the seed-character defaults.

## Evaluation

Compared neutral `0`, partial `0.25` and `0.5`, and full `1` seed variation on production-size `256x192` maps using the task 060 seed search range:

| Seed | Role | Variation | Ruggedness | High terrain | Peaks | Average land elevation | Max land elevation |
|---:|---|---:|---:|---:|---:|---:|---:|
| 1867 | flat extreme | 0 | 0.50000 | 0 | 26 | 0.55252 | 0.64239 |
| 1867 | flat extreme | 1 | 0.00005 | 0 | 16 | 0.52967 | 0.54893 |
| 2507 | ordinary | 0 | 0.50000 | 222 | 21 | 0.57200 | 0.70632 |
| 2507 | ordinary | 1 | 0.50156 | 240 | 21 | 0.57232 | 0.70740 |
| 3743 | rugged extreme | 0 | 0.50000 | 0 | 16 | 0.53242 | 0.57809 |
| 3743 | rugged extreme | 1 | 0.99995 | 532 | 47 | 0.56496 | 0.90970 |
| 42 | representative rugged | 0 | 0.50000 | 0 | 21 | 0.54229 | 0.60113 |
| 42 | representative rugged | 1 | 0.86145 | 3499 | 42 | 0.59456 | 0.88523 |
| 314159 | representative flatter | 0 | 0.50000 | 0 | 33 | 0.54584 | 0.62494 |
| 314159 | representative flatter | 1 | 0.38632 | 0 | 28 | 0.53934 | 0.59631 |
| 8675309 | representative rugged | 0 | 0.50000 | 0 | 24 | 0.53777 | 0.59677 |
| 8675309 | representative rugged | 1 | 0.79386 | 696 | 37 | 0.56649 | 0.76882 |

Full variation produced useful realm-scale contrast without changing land/water topology. Ordinary seeds stayed almost neutral, flat seeds became visibly low-relief, and rugged seeds gained highlands, peaks, and stronger maximum relief without making every representative map globally mountainous.

## Decision

Keep full deterministic seed character as the generator default: `seed_terrain_variation = 1`.

Seed variation remains a normal debug tuning control. `0` is the exact neutral comparison mode, not the product default. This decision applies to greater-realm terrain generation only; it does not commit gameplay presentation to 2.5D terrain.

## Implementation

- Added `DEFAULT_SEED_TERRAIN_VARIATION` and wired `GreaterRealmGeneratorSettings::seed_terrain_variation` to it.
- Added regression coverage that the default is full deterministic character and that default output matches explicit full variation for the discovered flat and rugged extreme seeds.
- Updated the hydrology channel-density regression to use a threshold ratio that matches the now-explicit full-character default.
- Updated `docs/PROCGEN.md` with the retained default, neutral comparison behavior, and the boundary between seed-driven terrain character and gameplay presentation.

## Verification

- `cmake --build out\build\debug-with-tests --target realm_engine_tests realm_procgen_regeneration_tests realm_procgen_pipeline_tests realm_procgen_paint_tests realm_mountain_peak_tests realm_procgen_representation_tests realm_ecs_tests realm_ecs_system_tests realm_ecs_query_tests realm_ui_tests realm_ui_repeat_button_tests realm_ui_slider_tests realm_greater_realm_debug_panel_tests realm_terrain_mesh_tests` passed.
- `ctest --test-dir out\build\debug-with-tests -R procgen --output-on-failure` passed, 6/6.
- `ctest --test-dir out\build\debug-with-tests --output-on-failure` passed, 14/14.
- `cmake --build --preset debug-no-tests` passed.
- `cmake --build --preset release-no-tests` passed.
