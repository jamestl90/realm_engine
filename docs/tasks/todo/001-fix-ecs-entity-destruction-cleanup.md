# Fix ECS Entity Destruction Cleanup

Status: todo
Area: ECS

## Goal

Make entity destruction remove or invalidate all components belonging to the destroyed entity, not just invalidate the entity handle.

## Context

`World::destroy_entity()` currently increments the entity generation and returns the index to the free list, but component arrays are type-erased and components are not removed at destroy time.

## Acceptance Criteria

- Destroyed entities no longer leave renderable/updateable stale components behind.
- Destroying an invalid entity is still harmless.
- Component cleanup works for all registered component arrays.
- Add focused tests or a small verification path for create/add/destroy/reuse behavior.

## Notes

This is a high-priority correctness task because stale components can leak into rendering and systems.
