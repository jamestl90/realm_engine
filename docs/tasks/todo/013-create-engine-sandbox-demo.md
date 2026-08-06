# Create Engine Sandbox Demo

Status: todo
Area: Demo

## Goal

Turn the current sample game code into a focused engine sandbox/demo.

## Context

`RogueFarmGame` currently exercises sprite rendering and UI controls. A sandbox should intentionally prove engine features without becoming the main home for a full game.

## Acceptance Criteria

- Decide whether the sandbox lives under `src/game`, `examples`, or another project structure.
- Demonstrate sprite rendering, UI overlay, input, asset-loaded texture, entity spawn/despawn, and animation when available.
- Keep demo code clearly separated from reusable engine systems.
- Update `docs/PROJECT_BRIEF.md` if the repo/game boundary changes.

## Notes

This can be incremental. The first pass can simply formalize the existing demo's role.
