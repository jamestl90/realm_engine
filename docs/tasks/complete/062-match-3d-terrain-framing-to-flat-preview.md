# Match 3D Terrain Framing to Flat Preview

Status: complete
Priority: medium
Area: Rendering / Terrain

## Goal

Make the top-down 3D terrain occupy approximately the same visible area as the flat preview.

## Context

The 3D viewport uses the correct debug-panel boundary, but its orthographic projection multiplies the measured half-extents by `0.68`. An exact fit is `0.50`, so the current safety margin makes the mesh appear substantially smaller than the flat view.

## Acceptance Criteria

- Reduce the orthographic framing margin to a small anti-clipping allowance.
- Keep the complete terrain mesh visible at the current top-down camera orientation.
- Preserve viewport placement, mesh geometry, elevation scale, and terrain colours.
- Build and visually verify the Debug application.
- Run the terrain-rendering and full automated test suites.

## Dependencies

- Task 061 for the shared flat and 3D viewport boundary.

## Implementation Decisions

- Reduced the orthographic half-extent multiplier from `0.68` to `0.52`. Exact edge fit is `0.50`, so the new value retains a narrow anti-clipping border while removing most of the previous empty framing.
- Left viewport placement, camera orientation, mesh construction, elevation scale, lighting, and colours unchanged.

## Verification

- `realm_terrain_mesh_tests` passed.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 14/14.
- Debug and Release runtime builds passed.
- Launched the native application, switched to 3D mode, and visually confirmed the terrain fills the shared viewport closely with a narrow border and no clipped coastline.
