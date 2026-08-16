# Implement Deterministic World Region Addressing

Status: todo
Priority: high
Area: World / Coordinates

## Goal

Implement the approved world-region and chunk identity types, coordinate conversions, and stable per-region seed derivation.

## Context

Region generation, streaming, and persistence all require an identity that is independent from load order, memory address, container order, and implementation-defined hashing.

## Dependencies

- Task 067: approved world spatial hierarchy and generation contract.

## Acceptance Criteria

- Add strongly typed coordinates and identities for the hierarchy levels approved by Task 067.
- Implement checked conversions between world positions, local tiles, chunks, regions, and greater-realm samples where applicable.
- Define consistent behavior at negative coordinates, finite realm bounds, and exact boundaries.
- Derive stable region and subsystem seeds from world identity, region address, generator version, and an explicit domain value.
- Use a documented stable mixing algorithm rather than `std::hash` or container iteration order.
- Provide neighbor and bounds helpers needed by local generation and active-area tracking.
- Keep the module independent from SDL, rendering, ECS, and application policy.
- Add tests for determinism, coordinate round trips, boundary behavior, neighbor relationships, seed-domain separation, and representative extreme values.
- Update architecture documentation with the implemented identity contract.

## Notes

No branch is required unless the approved Task 067 contract reveals a broader cross-module migration.
