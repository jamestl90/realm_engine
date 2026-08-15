# Implement Tileset Loader

Status: todo
Area: Assets

## Goal

Implement loading for tileset metadata and texture linkage.

## Context

`TilesetAsset` has grid, tile property, terrain, animation, and region helpers, but `load_tileset_internal()` is currently a placeholder.

## Acceptance Criteria

- Define the supported tileset JSON subset.
- Load grid metadata, tile count, tile size, margin, spacing, and image path.
- Load tile collision/custom properties where practical.
- Create or reference the tileset texture.
- Update `docs/ASSETS.md`.

## Notes

This does not need to implement a full tilemap renderer yet.

## Dependencies

- Task 006 for the atlas-region identity policy used by tiles and tile animations.
