# Evaluate Irregular Dual-Mesh Terrain

Status: todo
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

