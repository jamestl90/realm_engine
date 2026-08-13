# Evaluate 2.5D Terrain Geometry

Status: todo
Area: Procgen / Rendering Architecture

## Goal

Decide whether Mapgen4-style ridge, valley, river, and mountain geometry belongs in the engine's greater-realm presentation.

## Context

Mapgen4 folds its terrain mesh around coastlines, valleys, ridges, and peaks and renders it with an oblique 2.5D projection. This engine is currently 2D, with 2.5D only a possible direction, so the representation and rendering cost need evaluation before implementation.

## Dependencies

- Task 027 for meaningful valley and drainage topology.
- Task 031 for explicit peak geometry.
- Task 032's decision to retain the regular grid as canonical data; any irregular geometry prototyped here must be derived render data.

## Acceptance Criteria

- Define the visual and gameplay benefits expected from 2.5D terrain.
- Prototype representative coast, ridge, valley, mountain, and river geometry.
- Assess camera, sprite-depth, batching, collision, and world-streaming implications.
- Record whether terrain geometry is runtime data, render-only derived data, or out of scope.
- Create scoped implementation tasks only if 2.5D is adopted.
- Update rendering and procgen documentation with the decision.

Branch: `procgen-2-5d-terrain-prototype`
Branch reason: This prototype crosses procgen and rendering architecture and should remain isolated from production behavior until reviewed.
