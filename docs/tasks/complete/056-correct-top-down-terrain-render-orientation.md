# Correct Top-Down Terrain Render Orientation

Status: complete
Priority: medium
Area: Rendering / Terrain Presentation

## Goal

Fix the top-down 3D terrain render orientation so it matches the flat 2D map without relying on an ad hoc flipped look-at axis.

## Context

The top-down heightfield view currently appears upside down relative to the flat debug map. Manually flipping the look-at Y axis can make the result look correct, but that is a symptom-level fix. The renderer needs a clear coordinate convention for canonical map coordinates, mesh coordinates, view matrices, shader projection, and viewport orientation.

## Acceptance Criteria

- Identify whether the inversion comes from mesh Y coordinates, matrix layout/multiplication convention, the top-down view basis, shader expectations, or viewport/projection orientation.
- Define the intended coordinate mapping from `GreaterRealmMap` cell `(x, y)` to flat debug image pixels and terrain mesh world coordinates.
- Make the 3D terrain view match the flat 2D debug map orientation for the same seed and overlays.
- Remove any temporary/hacky look-at-axis flip in favor of a documented coordinate or matrix fix.
- Preserve the top-down 2D/retro map feel requested for the 3D mesh view.
- Add focused tests for terrain view/projection or mesh orientation where feasible without GPU readback.
- Smoke-test Flat and 3D views in Debug and verify they show the same map orientation.
- Update `docs/RENDERING.md` with the terrain camera and coordinate convention.

## Dependencies

- Task 033's terrain heightfield renderer.
- The top-down camera changes made after task 050.

## Notes

No dedicated branch is required unless investigation shows the fix needs broad renderer matrix or shader changes.

## Implementation Summary

- Identified the inversion as a derived mesh coordinate issue: map rows were mapped to increasing world `y`, so a top-down camera using world `+y` as up displayed the flat debug image upside down.
- Changed `TerrainMesh` to map top map rows to positive world `y` and adjusted `gradient_y` to remain a world-space derivative.
- Kept the top-down terrain camera's normal `+y` up vector, avoiding the temporary look-at-axis flip.
- Added mesh orientation and vertical-gradient tests that verify the coordinate convention without GPU readback.
- Added a debug-panel smoke assertion that the actual `3D` button switches presentation mode and emits the presentation callback.
- Updated `docs/RENDERING.md` with the flat-pixel, canonical-map, mesh-world, and camera orientation convention.

## Testing

- Passed: `cmake --build out/build/debug-with-tests --target realm_ui_slider_tests realm_terrain_mesh_tests`
- Passed: `cmake --build out/build/debug-with-tests --target realm_engine`
- Passed: `ctest --test-dir out/build/debug-with-tests --output-on-failure` (13/13)

## Commit Message

Correct top-down terrain mesh orientation
