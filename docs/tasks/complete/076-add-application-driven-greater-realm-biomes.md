# Add Application-Driven Greater-Realm Biomes

Status: complete
Priority: high
Area: Procgen / Greater Realm Biomes

## Goal

Classify greater-realm cells with application-provided biome rules while keeping biome identity and product meaning outside core terrain and climate generation.

## Context

The reusable engine can evaluate generic environmental rules, but it cannot define which biomes a particular game contains or what those biomes mean. Task 066 therefore assigns rule definitions and opaque IDs to applications and keeps derived assignments in a separate biome map.

## Dependencies

- Task 066: climate and biome ownership contract.
- Task 074: stable temperature normals.
- Task 075: stable precipitation normals.

## Acceptance Criteria

- Define an application-supplied biome rule set with unique opaque biome IDs and explicit validation.
- Allow rules to constrain terrain form, water form, elevation, slope, coast distance, temperature normal, and precipitation normal without requiring every input.
- Define deterministic precedence, tie-breaking, fallback, and unmatched-cell behavior independent from associative-container order.
- Produce a separate `GreaterRealmBiomeMap` with source terrain, climate, and rule-set identity checks.
- Keep biome IDs, names, colours, art, resources, spawn policy, and gameplay effects out of `GreaterRealmCell` and `GreaterRealmClimateMap`.
- Rebuild biome assignment when terrain, climate, or rule data changes without regenerating terrain or climate.
- Add a compile-gated biome debug view that consumes an application-supplied colour table or neutral fallback palette.
- Add tests for rule validation, deterministic assignment, precedence, fallback, water rules, threshold boundaries, rule-order behavior, source mismatch, and regeneration locality.
- Document the engine/application ownership boundary and provide a small in-memory example rule set for tests or sandbox inspection.

## Notes

Asset-file formats and application content packs are out of scope. The first implementation should prove the reusable in-memory contract before connecting it to Task 009 or other asset loaders.

No branch is required unless implementation expands into application asset integration.

## Implementation Decisions

- Added validated ordered `GreaterRealmBiomeRuleSet` data with unique opaque `BiomeId` values, explicit priorities, optional constraints, and an optional fallback ID.
- Rules can independently constrain terrain form, land/ocean/inland-water class, elevation, slope, coast distance, temperature normal, and precipitation normal. All configured range boundaries are inclusive.
- Higher priority wins; equal-priority ties select the first declared rule. Unmatched cells receive the fallback or `INVALID_BIOME_ID`, independent from associative-container ordering.
- Added separate versioned `GreaterRealmBiomeMap` output containing only opaque IDs and fingerprints for terrain, climate, and complete ordered rule-set identity.
- Added `GreaterRealmBiomeGenerationCache`; changed terrain, climate values, rule content, rule order, version, or application identity rebuilds assignment without regenerating terrain or climate.
- Added a `Biome` debug view that consumes an application colour table and uses a deterministic neutral fallback palette for unlisted IDs.
- Added an in-memory `TestApp` inspection rule set and colour table. Its ocean, inland-water, alpine, polar, rainforest, desert, forest, tundra, and grassland meanings remain sandbox application data, not engine definitions.
- Moved the sandbox policy into `GreaterRealmDebugBiomes` so the application-owned rules and colours are measured directly rather than duplicated in procgen tests.
- Calibrated the debug rules against seed `8675309` at `256x192`. After Task 077's circulation correction, desert is about `40%` of land, grassland `28%`, tundra `22%`, forest `8%`, and alpine `2%`. A regression test rejects any single class at `70%` or more and requires at least four land classes above `1%`.
- Kept names, art, resources, spawning, gameplay behavior, persistence formats, and asset loading out of the reusable biome module.

## Verification

- Added `realm_biome_tests` for rule validation, all optional input contracts, deterministic assignment, priority, inclusive boundaries, declaration-order ties, fallback/unmatched behavior, water rules, source identity, climate/rule invalidation, and custom/fallback debug colours.
- Added representative sandbox-distribution coverage after visual review showed the original thresholds classified roughly `90%` of land into similar desert/grassland colours.
- Focused greater-realm, climate, biome, and debug-panel tests passed, 4/4.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- Explicit Debug builds passed for all runtime and test targets.
- `scripts/build.ps1 -Preset debug-no-tests` and `scripts/build.ps1 -Preset release-no-tests` passed.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF conversion warnings.
