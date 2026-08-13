# Add Procgen Debug View Selection

Status: complete
Area: Procgen / Debug Tooling

## Goal

Inspect different greater-realm data layers at runtime without regenerating the world.

## Acceptance Criteria

- Add selectable terrain, elevation, landmass, mountain influence, slope, coast distance, humidity, rainfall, moisture, and drainage-flow base views.
- Keep coastline, mountain peaks, rivers, and drainage directions as independently selectable overlays.
- Preserve the current terrain image and overlays as the default configuration.
- Keep view colouring engine-neutral and compile-gated with the existing procgen debug module.
- Retain the current generated map so changing a view rebuilds only the debug image and texture.
- Do not invoke `generate_greater_realm` when a view or overlay changes.
- Add tests for every base view, overlay enable/disable behavior, malformed maps, and default-image compatibility.
- Update `PROCGEN.md` with the available views and overlays.

## Verification

- All eight Debug test suites pass.
- Debug and Release game targets build successfully.
- Live interaction produced texture refreshes without procgen-stage or generated-map logs.
- Captured 1982x1190 window verification confirms all controls and summaries fit without overlap.

## Current State

Task 039 later removed generated weather fields and replaced the climate and drainage-flow views with terrain-only catchment area. This task remains the historical record for the debug-view selection mechanism.
