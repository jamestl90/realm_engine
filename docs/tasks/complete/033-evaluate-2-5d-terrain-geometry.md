# Evaluate 2.5D Terrain Geometry

Status: complete
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

- Add a `Flat | 3D` presentation control without regenerating or mutating the map.
- Derive a continuous regular-grid heightfield mesh from canonical greater-realm cells.
- Render the heightfield with a fixed oblique camera, depth buffering, terrain colour, surface normals, and directional lighting.
- Provide an elevation-scale control for inspecting relief.
- Keep existing flat debug views and overlays available.
- Define the visual and gameplay benefits expected from 2.5D terrain.
- Prototype representative coast, ridge, valley, and mountain geometry while retaining river-route presentation.
- Assess camera, sprite-depth, batching, collision, and world-streaming implications.
- Record whether terrain geometry is runtime data, render-only derived data, or out of scope.
- Create scoped implementation tasks only if 2.5D is adopted.
- Update rendering and procgen documentation with the decision.

Branch: `procgen-2-5d-terrain-prototype`
Branch reason: This prototype crosses procgen and rendering architecture and should remain isolated from production behavior until reviewed.

## Decision

Adopt a continuous regular-grid heightfield as render-only derived data. Canonical greater-realm cells remain authoritative for generation, gameplay queries, collision, persistence, and future region streaming. The prototype deliberately does not adopt Mapgen4's irregular folded mesh.

The 3D presentation improves elevation, ridge, valley, coastline, ocean-floor, and river-route inspection while preserving the existing flat debug workflow. It uses one indexed terrain draw and does not regenerate procedural data when presentation mode or elevation scale changes.

## Implications

- Camera: the prototype uses a fixed oblique orthographic debug camera; gameplay camera control is outside this task.
- Sprites: existing sprites render in a later 2D pass and do not yet share terrain projection or depth. Task 047 tracks that integration.
- Batching: the current `256x192` map is one draw with 49,152 vertices and 292,230 indices.
- Collision: rendered triangles do not become collision geometry; gameplay remains tied to canonical grid data.
- Streaming: future world regions should derive and upload independently replaceable mesh chunks from canonical region data.

## Implementation

- Added a reusable CPU heightfield mesh builder with positions, elevation gradients, colours, and 32-bit triangle indices.
- Added an engine-owned terrain renderer, terrain shaders, reflected uniforms, depth buffering, oblique camera fitting, and directional lighting.
- Added application-level `Flat | 3D` presentation and elevation-scale controls while retaining all flat views and overlays.
- Made depth allocation and the terrain-to-sprite pass transition conditional on 3D presentation, leaving Flat mode on the existing colour-only path.
- Kept all procedural generation output and settings unchanged.

## Testing

- Added compile-gated terrain mesh tests covering mesh shape, centring, indices, colours, gradients, elevation bounds, and invalid input.
- Built `debug-no-tests`, `debug-with-tests`, and `release-no-tests` successfully.
- Ran all 11 CTest targets successfully, including `rendering_terrain_mesh`.
- Confirmed the Release build graph excludes terrain test sources and bundles both terrain shaders plus reflection data.
- Smoke-tested the Release executable and visually verified Flat and 3D Debug presentations, lighting, overlays, camera framing, and pass transitions without GPU validation errors.
