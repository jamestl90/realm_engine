# Expand ECS System And Resource Tests

Status: complete
Area: ECS

## Goal

Extend ECS regression coverage beyond entity and component lifecycle behavior.

## Acceptance Criteria

- Test system priority ordering.
- Test enabled and disabled system behavior.
- Test system removal.
- Test world resource set, get, replacement, and removal behavior.
- Keep the tests in an isolated target available only when `REALM_BUILD_TESTS=ON`.

## Verification

- `ecs_system_resources` passes in an explicit tests-enabled build.
- All four current test suites pass together.
- A fresh tests-disabled configuration builds `realm_engine` without creating or compiling test targets.
- ECS test sources require the test-only `REALM_TEST_BUILD` definition.
- 2026-08-16 backlog closure: all 14 CTest targets passed, including `ecs_system_resources`, and fresh Debug and Release runtime builds succeeded.

## Notes

Entity and component lifecycle coverage belongs to task 001.
