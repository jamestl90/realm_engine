# Adopt Mapgen4 Island Constraint

Status: testing
Area: Procgen
Branch: `proc-gen`
Branch reason: This continues the existing greater realm procedural generation feature stream.

## Goal

Replace the circular greater-realm landmass bias with Mapgen4's signed fBm and square-distance island constraint.

## Context

The previous generator used aspect-corrected Euclidean distance from the map centre. That guaranteed ocean boundaries but made most seeds look like variations of a circle.

Mapgen4 combines five-octave fBm with an island term based on `max(abs(x), abs(y))`. The square-distance constraint preserves ocean boundaries while allowing noise to use more of the rectangular map.

## Acceptance Criteria

- Remove the aspect-corrected radial landmass falloff.
- Generate the signed base constraint from five-octave fBm and a square-distance island term.
- Preserve sea-level control as an offset to the signed coastline.
- Add a configurable island-bias setting with a debug UI control.
- Default island bias to Mapgen4's `0.5` value.
- Keep coastline noise, inland relief, and ocean depth as later independent stages.
- Allow island bias to control how strongly map boundaries are pushed underwater.
- Verify island bias changes topology without affecting determinism or output invariants.
- Update procgen documentation and build a tests-disabled Debug executable.

## Verification

- `procgen_greater_realm` passes with strong-bias boundary and island-bias topology coverage.
- All five isolated test suites pass together.
- The game builds with `CMAKE_BUILD_TYPE=Debug`, `RFD_BUILD_TESTS=OFF`, and `RFD_ENABLE_PROCGEN_DEBUG_VIEW=ON`.
- The compact debug panel exposes Island bias alongside the other layered controls.

## Notes

Visual comparison across multiple seeds remains part of testing.

This adopts Mapgen4's automatic default constraint. Editable painting and the Delaunay/Voronoi mesh remain out of scope.
