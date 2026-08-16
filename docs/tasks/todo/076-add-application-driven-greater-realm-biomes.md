# Add Application-Driven Greater-Realm Biomes

Status: todo
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
