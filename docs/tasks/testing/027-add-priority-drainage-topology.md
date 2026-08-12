# Add Priority Drainage Topology

Status: testing
Area: Procgen / Hydrology

## Goal

Build deterministic downslope links and a drainage order that can carry water from terrain cells toward the ocean.

## Context

Mapgen4 seeds a priority traversal from deep ocean and assigns each terrain element a downslope connection. The greater-realm generator currently computes local slope magnitude only; it has no connected drainage topology.

## Dependencies

- Stable greater-realm elevation output.

## Acceptance Criteria

- Export a downslope target or outlet state for every cell.
- Export an ordering suitable for downstream flow accumulation.
- Handle depressions deterministically without cycles.
- Route boundary-connected water toward valid ocean outlets.
- Keep hydrology independent from debug rendering and application code.
- Add tests for determinism, acyclic flow, valid neighbors, and outlet reachability.
- Document the new map data and pipeline stage.

## Implementation

- Added a deterministic priority-flood traversal seeded from ocean cells, with boundary fallback outlets for maps without ocean.
- Exported per-cell downslope indices, outlet flags, conditioned drainage elevation, and a complete downstream-first drainage order.
- Kept visual elevation unchanged while conditioning depressions for acyclic downstream flow.

## Verification

- Dedicated tests validate complete unique ordering, adjacent downslope links, downhill conditioned elevation, cycle freedom, and outlet reachability.
- All six CTest targets pass.
- Tests-disabled Debug and Release builds succeed.
- The Debug executable passes a native startup smoke test.
