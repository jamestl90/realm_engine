# Fix ECS Entity And Component Lifecycle

Status: testing
Area: ECS

## Goal

Enforce entity and component lifecycle invariants so destroyed, stale, or invalid entities cannot leave accessible component data behind.

## Context

`World::destroy_entity()` previously recycled entity indices without removing their components. Component insertion also appended duplicates, and component APIs did not reject stale handles.

## Acceptance Criteria

- Remove an entity's components from every registered component array when it is destroyed.
- Keep destruction of invalid or already-destroyed entities harmless.
- Re-adding an existing component updates it without growing component storage.
- Reject component add, remove, get, and has operations for invalid or stale entity handles.
- Preserve correct component-array compaction and entity lookup after removal.
- Add focused tests for create, add, replace, destroy, stale access, and index reuse behavior.
- Keep tests in a separate target that is available only when `REALM_BUILD_TESTS=ON` and is excluded from normal game builds.
- Verify Debug and Release builds.

## Verification

- Debug game and all Debug test targets build successfully.
- `ecs_world_lifecycle`, `procgen_greater_realm`, and `ui_text_measurement` pass.
- Release game builds successfully.
- Test source is isolated under `tests/ecs` and compiled with `REALM_TEST_BUILD=1` only for `realm_ecs_tests`.
