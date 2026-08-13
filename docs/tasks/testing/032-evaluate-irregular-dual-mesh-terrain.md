# Evaluate Irregular Dual-Mesh Terrain

Status: testing
Area: Procgen / Architecture

## Goal

Decide whether greater-realm generation should retain its regular grid or adopt a Delaunay/Voronoi dual mesh like Mapgen4.

## Context

Mapgen4 performs generation and hydrology on an irregular dual mesh. The engine currently uses a regular cell grid that is simpler for storage, region extraction, and future tile generation. A mesh migration would affect most procgen data contracts and should only happen for demonstrated benefits.

## Acceptance Criteria

- Compare grid and dual-mesh behavior for coastline quality, drainage, rivers, memory, generation cost, serialization, and local-tile handoff.
- Prototype only enough dual-mesh data to measure the important tradeoffs.
- Record whether to retain the grid, use a hybrid representation, or migrate generation.
- Identify affected APIs and migration risks for any recommended change.
- Create implementation tasks only if the evaluation supports a change.
- Update architecture and procgen documentation with the decision.

## Evaluation Method

- Audited every current greater-realm module and test for assumptions about coordinates, adjacency, storage, and output.
- Compared the current grid against Mapgen4's region/triangle/side/half-edge representation at an equal terrain-sample count.
- Added a test-only storage and traversal harness for a `256x192` map (`49,152` terrain samples).
- Modelled compact dual-mesh topology from region positions, triangle-side region indices, half-edges, one incident side per region, side lengths, and boundary flags.
- Included a minimal spatial-index allowance required for irregular world-region lookup.
- Verified grid indexing and rectangular region extraction against the real `GreaterRealmMap` API.

## Measurements

| Measurement | Regular grid | Compact dual-mesh estimate |
|---|---:|---:|
| Terrain samples | 49,152 | 49,152 triangles |
| Region/point count | implicit | 24,578 |
| Directed sides | implicit | 147,456 |
| Current terrain-cell storage | 3,342,336 bytes | 2,949,120 bytes after removing grid `x/y` |
| Topology/coordinate storage | 393,216 bytes already included above | 2,113,560 bytes |
| Local-region spatial index | unnecessary | approximately 196,624 bytes |
| Estimated terrain plus topology total | 3,342,336 bytes | 5,259,304 bytes |
| Optimized traversal probe, 80 map passes | 4,146 microseconds, four implicit neighbors | 7,057 microseconds, six explicit neighbors |

The dual estimate is approximately 57% larger at equal terrain-sample count before allocator overhead. Traversal timings are indicative rather than contractual because the two representations visit different neighbor counts.

## Comparison

| Concern | Regular grid | Irregular dual mesh |
|---|---|---|
| Coastline quality | Raster resolution can expose stair steps; localized noise and later rendering can soften them | Better low-resolution polygon silhouettes and fewer axis-aligned edges |
| Drainage and rivers | Current eight-neighbor priority drainage is deterministic and already supports downstream flow | More isotropic adjacency and naturally polygonal river geometry |
| Generation cost | No topology construction; direct linear allocation | Requires point sampling and Delaunay/half-edge construction, although static topology can be cached |
| Memory | Implicit adjacency and direct coordinates | Explicit points, triangle references, half-edges, side data, and lookup index |
| Serialization | Width/height plus contiguous terrain fields | Must serialize or rebuild topology and validate connectivity alongside terrain fields |
| Local tile handoff | Exact rectangular extraction using contiguous rows and direct `x/y` lookup | Requires spatial queries, overlap handling, and rasterization into local tile grids |
| Existing API impact | Matches all current procgen modules | Replaces `width`, `height`, `index`, `cell`, coordinate neighbors, debug rasterization, and many tests |

## Decision

Retain the regular grid as the canonical greater-realm generation and gameplay representation.

Do not migrate climate, hydrology, constraints, mountain fields, world management, or local-tile handoff to a Delaunay/Voronoi dual mesh. The measured memory and integration costs are concrete, while the main benefits are presentation-oriented and do not currently unblock engine goals.

A hybrid remains permissible: task 033 may evaluate a derived irregular or triangulated render surface generated from canonical grid data for coastline presentation, terrain folds, or 2.5D projection. Such a surface must remain disposable render data and must not replace `GreaterRealmMap` unless new evidence justifies reopening this decision.

## Follow-Up

No new implementation task is justified. Task 033 already owns the only promising use of irregular geometry: derived 2.5D/render terrain.

## Verification

- The evaluation harness is compiled only with `RFD_BUILD_TESTS=ON`.
- It verifies direct grid indexing, exact rectangular extraction, dual topology counts, storage accounting, spatial-index requirements, and both traversal paths.
- Measurements were collected from optimized and Debug builds without adding Delaunay dependencies or release code.
- All eight CTest targets pass.
- Tests-disabled Debug and Release preset builds succeed and contain no representation-evaluation target.
- `git diff --check` passes.
