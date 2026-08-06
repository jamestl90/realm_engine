# Add ECS Lifecycle Tests

Status: todo
Area: ECS

## Goal

Add focused tests for entity, component, system, and resource behavior.

## Context

The ECS is central to the engine and already has enough implementation to benefit from regression tests.

## Acceptance Criteria

- Test entity creation, validity, destruction, and index reuse.
- Test add/get/has/remove component behavior.
- Test component-array compaction after removal.
- Test system priority ordering and enabled/disabled behavior.
- Test world resource set/get/remove behavior.

## Notes

Choose the lightest test setup that fits the current CMake/project structure.
