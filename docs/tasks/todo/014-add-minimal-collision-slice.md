# Add Minimal Collision Slice

Status: todo
Area: Physics

## Goal

Implement a minimal collision path useful for early gameplay prototypes.

## Context

Physics/collision headers exist, but there is not yet a small end-to-end collision workflow.

## Acceptance Criteria

- Add an AABB collision component or confirm the existing component shape.
- Add simple broadphase support, likely grid-based.
- Expose collision queries or a collision system.
- Include debug draw integration if available.
- Add a small verification/demo case.

## Notes

Keep this intentionally small. Full physics response can come later.
