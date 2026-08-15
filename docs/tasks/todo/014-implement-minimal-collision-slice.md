# Implement Minimal Collision Slice

Status: todo
Area: Physics

## Goal

Implement the existing collision interfaces as a minimal end-to-end path useful for early gameplay prototypes.

## Context

`Collision.hpp` and `SpatialPartition.hpp` already declare AABB/circle tests, a spatial grid, and an ECS collision system, but no source implementation is compiled and the world does not run the system.

## Acceptance Criteria

- Validate the existing AABB component contract and change it only where tests show a problem.
- Implement the declared grid broadphase and narrow-phase tests.
- Integrate the declared collision system with ECS scheduling and expose its results.
- Include debug draw integration if available.
- Add compile-gated unit tests and a small sandbox verification case when task 013 is available.

## Notes

Keep this intentionally small. Full physics response can come later.
