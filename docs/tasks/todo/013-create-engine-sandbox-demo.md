# Create Engine Sandbox Demo

Status: todo
Area: Demo

## Goal

Create a focused engine sandbox executable that validates reusable engine features without repurposing application-specific game code.

## Context

`RogueFarmGame` currently owns application-specific procgen controls and preview composition, as established by task 024. `docs/PROJECT_BRIEF.md` permits in-repository demos for engine validation, but those demos must remain separate from application/game ownership.

## Acceptance Criteria

- Add a separate sandbox/example target outside `RogueFarmGame`.
- Demonstrate the engine capabilities available when the task starts, beginning with sprite rendering, UI overlay, asset-loaded texture, and entity spawn/despawn.
- Add input, animation, collision, and other slices only when their owning engine tasks are complete; do not reimplement them in the sandbox.
- Keep demo code clearly separated from reusable engine systems.
- Keep `docs/PROJECT_BRIEF.md` ownership boundaries unchanged unless implementation reveals a genuine architectural conflict.

## Notes

This can be incremental, but it must produce an independently buildable engine-validation target rather than renaming or expanding the current application class.
