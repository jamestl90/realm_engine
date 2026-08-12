# Add River Flow Accumulation

Status: testing
Area: Procgen / Hydrology

## Goal

Accumulate moisture along the drainage topology and export a usable greater-realm river network.

## Context

Mapgen4 accumulates moisture in drainage order and derives river width from flow. The current generator exports no flow field or river segments.

## Dependencies

- Task 027: priority drainage topology.
- Task 028: rainfall and moisture fields.

## Acceptance Criteria

- Accumulate upstream moisture into deterministic per-cell flow values.
- Export river segments or edges separately from terrain classification.
- Provide tunable minimum-flow and width mapping parameters.
- Prevent cycles, uphill river segments, and disconnected interior endpoints.
- Keep river data engine-neutral and independent from a particular renderer.
- Add tests for conservation-like accumulation behavior, determinism, connectivity, and thresholding.
- Add compile-gated debug visualization and update procgen documentation.

## Implementation

- Accumulated per-cell moisture in reverse drainage order.
- Exported engine-neutral river segments with source, destination, flow, and derived width.
- Added minimum-flow, flow-scale, and width-scale settings.
- Added a compile-gated blue river overlay and compact flow/threshold controls.

## Verification

- Dedicated tests validate accumulated downstream flow, connected downslope segments, conditioned downhill flow, thresholding, width output, and debug-image visibility.
- All six CTest targets pass.
- Tests-disabled Debug and Release builds succeed.
- The Debug executable passes a native startup smoke test.
