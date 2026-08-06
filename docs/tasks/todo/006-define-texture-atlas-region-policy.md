# Define Texture Atlas Region Policy

Status: todo
Area: Rendering

## Goal

Clarify and standardize how sprites refer to atlas regions.

## Context

`Sprite::region_index` is numeric, while `TextureManager::define_region()` stores named string regions. The renderer currently looks up regions by converting `region_index` to a string.

## Acceptance Criteria

- Decide whether atlas regions are numeric, named, or both.
- Update `Sprite`, `TextureManager`, or lookup code to match the decision.
- Ensure the sample/demo still renders correctly.
- Update `docs/RENDERING.md` and `docs/ASSETS.md` if needed.

## Notes

This should be resolved before animation and tileset work lean heavily on atlas regions.
