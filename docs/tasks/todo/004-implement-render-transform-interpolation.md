# Implement Render Transform Interpolation

Status: todo
Area: Rendering

## Goal

Make sprite rendering use real interpolation between fixed updates.

## Context

`Renderer::render(world, alpha)` receives an interpolation alpha, but currently uses the current transform position because previous transform state is not tracked.

## Acceptance Criteria

- Add a previous/current transform strategy for renderable entities.
- Sprite positions interpolate smoothly between fixed updates.
- Non-moving entities render unchanged.
- Update `docs/RENDERING.md` to remove or revise the current interpolation limitation.

## Notes

Keep the design compatible with the fixed timestep loop.
