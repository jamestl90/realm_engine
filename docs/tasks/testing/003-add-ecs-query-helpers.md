# Add ECS Query Helpers

Status: testing
Area: ECS

## Goal

Add simple helpers for iterating entities that have common component combinations.

## Context

Systems previously accessed component arrays directly. That was efficient, but repeated multi-component lookup loops were noisy and easy to get wrong.

## Acceptance Criteria

- Provide mutable and const APIs for iterating entities with two or more required components.
- Iterate the smallest participating component array and avoid per-query heap allocation.
- Skip invalid entities and entities missing any requested component.
- Avoid adding archetype complexity at this stage.
- Migrate sprite render-command collection to the query helper.
- Add isolated regression tests for two- and three-component queries.
- Document intended usage and mutation constraints in `docs/ECS.md`.

## Verification

- `ecs_component_queries` passes in the explicit tests-enabled build.
- All five current test suites pass together.
- Tests-disabled Debug and Release game builds compile the renderer through the query API.
- Query tests remain isolated behind `RFD_BUILD_TESTS=ON` and `RFD_TEST_BUILD=1`.

## Notes

Structural ECS changes must be queued and applied outside a query callback.
