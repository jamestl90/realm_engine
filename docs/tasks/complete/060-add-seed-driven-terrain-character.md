# Add Seed-Driven Terrain Character

Status: complete
Priority: medium
Area: Procedural Generation / Terrain

## Goal

Allow deterministic seeds to produce a deliberate range from broad flat terrain through rolling relief to rare heavily mountainous realms.

## Context

Current seeds change terrain placement but share fixed relief amplitudes, peak spacing, and peak radius. As a result, maps differ spatially without developing strongly different overall terrain character. This experiment must remain inspectable and reversible because hidden random multipliers can otherwise make explicit controls difficult to reason about.

## Acceptance Criteria

- Derive one deterministic, exported terrain-ruggedness value from the seed.
- Add a `0..1` seed-variation setting that blends between neutral legacy amplitudes and full seed character.
- Guarantee that seed variation `0` produces scale factors of exactly `1` for existing relief and peak controls.
- Use ruggedness to scale hill amplitude, mountain strength, ridge/valley/noise detail, peak spacing, and peak radius.
- Keep island shape, coastline topology, ocean depth, authored constraints, and control monotonicity independent from terrain character.
- Keep existing sliders as explicit base values rather than replacing them with hidden settings.
- Expose seed variation and effective ruggedness in compile-gated debug tooling.
- Add deterministic tests for neutral compatibility, topology preservation, extreme-seed separation, and staged-regeneration equivalence.
- Update procgen documentation and run focused, full, Debug, and Release verification.

## Dependencies

- Task 052 for stable fixed peak selection.
- Task 058 so coastline detail cannot be mistaken for seed-driven inland terrain character.
- Task 059 so flat and mountainous outcomes retain fixed, comparable colour meaning.

## Implementation Decisions

- Added `GreaterRealmTerrainCharacter` with exported ruggedness and effective scale factors for base relief, mountain relief, mountain coverage, secondary detail, peak spacing, and peak radius.
- Derived ruggedness from one salted deterministic seed sample and used a symmetric power curve to make rare near-flat and near-rugged outcomes available without biasing all seeds toward either extreme.
- Added `seed_terrain_variation` with a `0..1` debug slider. The zero branch returns exact default scale factors instead of relying on approximate interpolation.
- Used centered exponential scale curves so ruggedness `0.5` remains exactly neutral while rare extremes separate strongly without making ordinary seeds uniformly mountainous.
- Treated existing relief and peak settings as explicit base values multiplied by the inspectable character scales.
- Added a mountain-coverage scale to the squared positive-constraint blend. This avoids ineffective strength inflation after mountain relief saturates, while preserving the neutral Mapgen4-aligned blend at scale `1`.
- Applied one secondary-detail scale to ridge, valley, and terrain-noise weights while leaving their individual authored balance intact.
- Mapped seed variation to the mountain-peak regeneration stage because topology fields are reused while effective spacing, radius, and all dependent relief are rebuilt.
- Added ruggedness to the debug terrain summary and generation log.
- Added deterministic tests that discover extreme character seeds from a fixed search range, compare each against its own neutral topology, and require strong flat-to-rugged relief separation.

## Verification

- Focused procgen, regeneration, mountain-peak, and debug-panel test executables passed.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 14/14.
- `cmake --build --preset debug-no-tests` passed.
- `cmake --build --preset release-no-tests` passed.
- Visually compared production-size neutral and full-character maps for discovered extreme seeds `1867` and `3743` using fixed terrain colours. Coastlines remained identical; the flat extreme removed inland relief, while the rugged extreme formed broad connected highlands and localized peaks without raising the entire landmass.
- Confirmed seed variation `0` returns exact unit scales, deterministic repeat generation matches exactly, and staged regeneration matches a clean generation.
