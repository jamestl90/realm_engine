# Implement Animation Asset Loader

Status: todo
Area: Assets

## Goal

Implement loading for animation asset metadata.

## Context

`AnimationAsset` and `AnimationFrame` exist, but `load_animation_internal()` is currently a placeholder.

## Acceptance Criteria

- Define a simple JSON format for sprite animation assets.
- Load frames, durations, loop flag, and optional events.
- Populate `AnimationAsset`.
- Add failure logging for malformed files.
- Update `docs/ASSETS.md` and connect with sprite animation docs.

## Notes

This pairs naturally with the sprite animation system, but can be done after a code-driven animation system exists.

## Dependencies

- Task 006 for the atlas-region identity policy.
- Task 005 for the runtime animation contract that loaded assets must drive.
