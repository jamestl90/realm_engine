# Add Climate-Influenced Terrain Colouring

Status: todo
Area: Procgen / Debug Rendering

## Goal

Visualize greater-realm terrain with continuous elevation and moisture-informed colour rather than terrain-form categories alone.

## Context

Mapgen4's colour mapping combines elevation and rainfall, then applies terrain lighting and water/coast outlines. The current debug map uses discrete terrain-form colours and ocean-depth shading.

## Dependencies

- Task 028: rainfall and moisture fields.

## Acceptance Criteria

- Add an engine-neutral colour mapping from elevation and moisture.
- Preserve water-depth readability and independent coastline metadata.
- Keep categorical terrain-form visualization available as a debug mode.
- Avoid assigning final game biomes or art direction in the engine layer.
- Add image-data tests for parameter response, stable dimensions, and deterministic output.
- Expose the mode only through compile-gated debug tooling.
- Update procgen documentation.

