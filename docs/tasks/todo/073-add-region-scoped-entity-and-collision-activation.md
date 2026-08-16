# Add Region-Scoped Entity And Collision Activation

Status: todo
Area: World / ECS / Collision

## Goal

Associate runtime entities and collision data with world regions so they activate and deactivate with the resident world area.

## Context

The ECS currently owns one live world with no spatial residency boundary. A segmented open world needs region ownership for generated objects, persistent entities, collision data, and safe unloading without making the ECS itself responsible for procedural generation.

## Dependencies

- Task 014: minimal collision implementation.
- Task 070: active world-region lifecycle.
- Task 071: persisted region modifications and application payload boundary.

## Acceptance Criteria

- Define a lightweight region-ownership component or resource contract for runtime entities and collision data.
- Activate generated and persisted region content only after its region becomes resident.
- Deactivate or destroy transient runtime objects in a deterministic order when a region unloads.
- Preserve durable entity state through the persistence boundary without serializing raw ECS storage or entity handles.
- Load and unload collision data with the owning region and prevent queries from retaining stale references.
- Keep world lifecycle coordination outside procgen and keep application-specific spawn policy outside reusable ECS storage.
- Add tests for activation order, unload cleanup, stale-handle safety, reactivation, persisted identity, and neighboring-region transitions.

## Branch

Use a dedicated branch. This integrates world lifecycle, ECS ownership, collision data, and persistence behavior with broad regression risk.
