# Add Sprite Animation System

Status: todo
Area: Rendering

## Goal

Implement a system that advances `SpriteAnimation` and updates the sprite region/frame used for rendering.

## Context

`SpriteAnimation` exists as component data, but no system currently drives animation playback.

## Acceptance Criteria

- Add an animation update system or equivalent engine path.
- Support frame duration, current frame, looping, playing/paused state.
- Update sprite region/frame data in a way the renderer already understands.
- Add a small demo or verification case.
- Update `docs/RENDERING.md`.

## Notes

This task can start with simple atlas-region frame indices before asset-driven animation loading exists.
