# Create Engine Sandbox Demo

Status: todo
Area: Demo

## Goal

Create a focused engine sandbox executable that validates reusable engine features without repurposing product-specific host application code.

## Context

The current host application owns procgen controls and preview composition, as established by task 024. In-repository demos must remain separate from reusable engine ownership and exist only to validate engine systems.

## Acceptance Criteria

- Add a separate sandbox/example target outside the current host application entry point.
- Demonstrate the engine capabilities available when the task starts, beginning with sprite rendering, UI overlay, asset-loaded texture, and entity spawn/despawn.
- Add input, animation, collision, and other slices only when their owning engine tasks are complete; do not reimplement them in the sandbox.
- Keep demo code clearly separated from reusable engine systems.
- Keep architecture ownership boundaries unchanged unless implementation reveals a genuine architectural conflict.

## Notes

This can be incremental, but it must produce an independently buildable engine-validation target rather than renaming or expanding the current application class.
