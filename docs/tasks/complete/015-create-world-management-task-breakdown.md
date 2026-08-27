# Create World Management Task Breakdown

Status: complete
Area: World

## Goal

Create a focused set of world management tasks after procedural map rendering is working well enough to expose the real engine needs.

## Context

The project is expected to support a large, segmented open world with significant procedural content. World management will likely include regions, chunks, streaming, persistence, entity activation, collision loading, and save-state boundaries.

This should not be broken down too early. The task breakdown should happen after the engine can render a procedural map slice, so the world management plan is informed by actual map generation, rendering, and data-shape constraints.

## Acceptance Criteria

- Review the procedural map rendering implementation and identify what world management support it needs.
- Create separate task files for the first practical world management slices.
- Cover region/chunk identity, active area tracking, loading/unloading lifecycle, procedural generation hooks, and persistence boundaries if they are still relevant.
- Mark any tasks that justify branch work with branch metadata and a short reason.
- Keep smaller tasks branch-free unless they are part of a larger coherent work stream.

## Notes

This is a planning task. Do not implement world management here unless the breakdown reveals a very small prerequisite that belongs with the planning work.

## Review Findings

- `GreaterRealmMap` is a complete coarse geography grid with stable coordinates, elevation, terrain form, coastline, drainage, rivers, and mountain metadata.
- The greater-realm grid has no contract for playable-world scale, region identity, chunk subdivision, local tiles, streaming, or persistence.
- The derived 3D heightfield is an inspection view and must not define gameplay coordinates or streaming boundaries.
- ECS entities currently live in one `World`; there is no region ownership, activation boundary, or collision-data lifecycle.
- Asset types anticipate tilesets, but local generation should export semantic terrain data rather than texture IDs or application-specific sprite choices.
- No existing save format owns generated-region identity, generator-version compatibility, or persistent modifications.

## Breakdown

- Task 067 defines the spatial hierarchy and generation ownership contract before coordinate types are implemented.
- Task 068 implements stable region/chunk addressing and deterministic seed derivation.
- Task 069 adds the first engine-neutral local top-down terrain generation slice.
- Task 070 adds active-area tracking plus region generation, activation, and unloading lifecycle.
- Task 071 defines versioned persistence around deterministic regeneration and stored modifications.
- Task 072 renders resident generated terrain through the normal top-down 2D tile/sprite path.
- Task 073 associates runtime entities and collision data with region activation.

## Implementation Decisions

- A greater-realm cell remains a coarse geography sample; it is not implicitly a streamable region or gameplay tile.
- Task 067 must decide the exact region/chunk/local-tile mapping and terminology before implementation fixes dimensions or coordinate conversions.
- Local generation must be deterministic per world identity and region address, independent of generation order, and continuous at shared boundaries.
- Procgen owns reusable semantic geography. Applications own biome definitions, tileset selection, sprite art, and product-specific placement policy.
- The playable world remains top-down 2D. The existing 3D heightfield remains a derived map/world inspection view.
- Region lifecycle, persistence, rendering, and ECS activation remain separate modules so synchronous generation can land before asynchronous streaming or save integration.
- Watersheds and runtime weather are optional future consumers and do not block the first local-world slices.
- No world-management code was implemented in this planning task.

## Verification

- Confirmed the breakdown covers region/chunk identity, active-area tracking, generation hooks, loading/unloading lifecycle, persistence, top-down presentation, and runtime entity/collision activation.
- Confirmed Tasks 067-073 have unique IDs, valid folder/status metadata, resolvable task references, explicit dependencies, and branch guidance proportional to their scope.
- Updated Task 066 so the spatial contract is its world-region prerequisite and parked Task 065 remains optional.
- Removed the obsolete parked runtime-weather/runoff dependency from the world-management breakdown.
- `git diff --check` passed; targeted trailing-whitespace checks found no issues.
- No build or runtime tests were required because this task changes planning and documentation only.

## Post-Completion Priority

Tasks 067-073 remain queued future work, scheduled after the current grand-scale greater-realm generation phase. None of them block continued greater-realm generation.
