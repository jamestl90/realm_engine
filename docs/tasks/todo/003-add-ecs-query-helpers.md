# Add ECS Query Helpers

Status: todo
Area: ECS

## Goal

Add simple helpers for iterating entities that have common component combinations.

## Context

Systems currently access component arrays directly. That is efficient, but repeated multi-component lookup loops will become noisy and easy to get wrong.

## Acceptance Criteria

- Provide a small API for iterating entities with two or more required components.
- Preserve data-oriented iteration where practical.
- Avoid adding archetype complexity at this stage.
- Document the intended usage in `docs/ECS.md`.

## Notes

This should be conservative. A tiny helper is better than a full query framework right now.
