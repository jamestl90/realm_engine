# Add Local Top-Down Terrain Generation Foundation

Status: todo
Priority: high
Area: Procgen / Local World

## Goal

Generate a deterministic, engine-neutral local terrain region from a world-region request and coarse greater-realm geography.

## Context

The greater-realm generator provides world-scale landform and hydrology, but the top-down game needs higher-resolution local terrain. Local output must preserve the coarse map's meaning while adding repeatable detail and seamless shared boundaries.

## Dependencies

- Task 067: world spatial hierarchy and generation ownership.
- Task 068: stable region addressing and seed derivation.

## Acceptance Criteria

- Define a local-generation request containing only stable world identity, region address, approved generator settings, and required greater-realm context.
- Export a fixed-shape local terrain grid with engine-neutral geographic values and classifications sufficient for later tile presentation and collision policy.
- Preserve greater-realm land/water, broad elevation, terrain-form, coastline, and river intent without copying coarse cells directly into local tiles.
- Sample noise and inherited fields in stable world coordinates so adjacent regions agree at shared boundaries.
- Generate any region independently and produce identical output regardless of request order or active neighbors.
- Keep biome labels, sprite/tileset IDs, resources, settlements, entities, and runtime weather out of the core output.
- Add an engine-neutral debug image or equivalent inspection output for representative local regions.
- Add tests for determinism, output shape, generation-order independence, shared-edge continuity, coarse-map agreement, and water/land edge cases.
- Document the local generation pipeline and its relationship to `GreaterRealmMap`.

## Branch

Use a dedicated branch. This introduces a new procgen data contract and generation pipeline with non-trivial continuity and determinism requirements.
