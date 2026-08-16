# Integrate Sprites With 2.5D Terrain Depth

Status: todo
Area: Rendering / World Presentation

## Goal

Allow world sprites to share the 2.5D terrain camera, positioning, and depth model so entities can be placed convincingly on the derived heightfield.

## Context

Task 033 adds a render-only terrain heightfield, but the existing sprite pass remains a screen-oriented 2D pass composed after terrain. Production 2.5D gameplay needs an explicit world-space sprite path rather than changing current UI or flat sprite behavior.

## Dependencies

- Task 033's terrain heightfield and camera representation.
- A defined policy for mapping canonical grid coordinates and elevation to rendered world positions.

## Acceptance Criteria

- Share terrain view/projection data with a world-sprite rendering path.
- Position world sprites from canonical map coordinates and sampled elevation.
- Define billboard orientation, pivot, scale, and sorting/depth behavior.
- Preserve the existing flat sprite and UI rendering paths.
- Verify occlusion against terrain and other world sprites with representative tests or a focused sandbox scene.
- Document performance and batching implications for hundreds to thousands of sprites.
