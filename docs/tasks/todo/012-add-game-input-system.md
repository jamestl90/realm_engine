# Add Game Input System

Status: todo
Area: Input

## Goal

Add a game-facing input system separate from UI event consumption.

## Context

The engine currently handles SDL events for quit/window behavior and UI dispatch. Game logic needs a clean way to query input state.

## Acceptance Criteria

- Track keyboard and mouse pressed/released/held state.
- Expose logical mouse coordinates.
- Define how UI consumption interacts with game input.
- Provide a minimal API for game code.
- Update architecture or add a small input capability doc if enough implementation exists.

## Notes

Action mapping can be a later layer if raw state queries land first.
