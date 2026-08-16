# Render Generated Local Regions As Top-Down Tiles

Status: todo
Area: Rendering / Local World

## Goal

Present resident generated regions through the normal top-down 2D tile and sprite pipeline.

## Context

Local procgen output is semantic engine data, not art selection. Rendering needs a policy boundary that lets applications map terrain values to tileset regions while the engine batches, culls, and positions visible tiles.

## Dependencies

- Task 006: texture-atlas region identity policy.
- Task 011: tileset metadata loading.
- Task 069: local terrain generation output.
- Task 070: resident-region lifecycle.

## Acceptance Criteria

- Define an application-supplied mapping from local terrain semantics to tileset or sprite-region choices.
- Render resident local terrain in the existing top-down 2D coordinate and camera model.
- Keep texture and atlas identifiers out of procgen output.
- Cull non-visible regions and tiles without changing generated data or lifecycle ownership.
- Preserve stable tile placement and visual continuity across region boundaries.
- Support debug overlays for region, chunk, and tile boundaries without requiring the 3D terrain path.
- Define batching and update behavior when regions activate, change, or unload.
- Add focused rendering tests plus a representative local-region sandbox view.

## Branch

Use a dedicated branch. This spans world lifecycle, tileset assets, renderer batching, camera behavior, and application presentation policy.

## Scheduling Note

This task remains queued behind local generation. The flat and 3D greater-realm views remain the current map-inspection paths.
