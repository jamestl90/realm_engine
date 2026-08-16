# Add Active World Region Lifecycle

Status: todo
Priority: high
Area: World / Streaming

## Goal

Track the desired active area and coordinate deterministic region generation, residency, activation, and unloading.

## Context

Local regions must have an explicit lifecycle before rendering, collision, entities, persistence, or asynchronous work can safely attach to them. The first implementation can remain synchronous while defining boundaries suitable for later background generation.

## Dependencies

- Task 068: stable region identities and neighborhood helpers.
- Task 069: local terrain generation request and output.

## Acceptance Criteria

- Represent region lifecycle states and legal transitions without coupling them to rendering or application classes.
- Compute a desired active set from one or more focus regions and a configurable extent.
- Generate and activate newly required regions, retain shared residents, and deactivate/unload regions that leave the desired set.
- Expose narrow hooks for generation, activation, deactivation, and unload handling.
- Make repeated updates idempotent and lifecycle ordering deterministic.
- Define ownership and lifetime rules for resident generated data and failure states.
- Keep the initial implementation synchronous while avoiding APIs that prevent later queued/background generation.
- Add tests for active-set changes, boundary movement, idempotence, transition ordering, failed generation, and unload behavior.
- Document how applications drive focus without owning the lifecycle implementation.

## Branch

Use a dedicated branch. This establishes shared world-runtime state and lifecycle contracts that later rendering, persistence, ECS, and collision systems will consume.

## Scheduling Note

This task remains queued after local generation exists; it is not part of the current grand-scale map work.
