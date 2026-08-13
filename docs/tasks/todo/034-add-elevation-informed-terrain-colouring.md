# Add Continuous Elevation-Informed Terrain Colouring

Status: todo
Area: Procgen / Debug Rendering

## Goal

Visualize greater-realm terrain with continuous elevation-informed colour rather than terrain-form categories alone.

## Context

Mapgen4's colour mapping combines multiple fields, then applies terrain lighting and water/coast outlines. The current debug map uses discrete terrain-form colours and ocean-depth shading. Runtime weather must remain separate from this generated geography view.

## Acceptance Criteria

- Add an engine-neutral continuous colour mapping from elevation and terrain form.
- Preserve water-depth readability and independent coastline metadata.
- Keep categorical terrain-form visualization available as a debug mode.
- Avoid assigning final game biomes or art direction in the engine layer.
- Do not use runtime rainfall, humidity, or moisture as generated inputs.
- Add image-data tests for parameter response, stable dimensions, and deterministic output.
- Expose the mode only through compile-gated debug tooling.
- Update procgen documentation.
