# Implement And Integrate Game Input System

Status: todo
Area: Input

## Goal

Implement and integrate the existing game-facing input interfaces separately from UI event consumption.

## Context

`include/input/InputSystem.hpp` declares keyboard, mouse, and input-state interfaces, but they have no implementation and `Engine` does not own or update an `InputSystem`. Game code therefore still receives raw SDL events after UI dispatch.

## Acceptance Criteria

- Implement the declared keyboard and mouse pressed/released/held state.
- Make `Engine` own the input system and update it once per frame.
- Expose logical mouse coordinates.
- Define how UI consumption interacts with game input.
- Provide a minimal API for game code.
- Add compile-gated tests for state transitions, frame boundaries, logical coordinates, and UI-consumption policy.
- Update architecture or add a small input capability doc if enough implementation exists.

## Notes

Action mapping can be a later layer if raw state queries land first.
