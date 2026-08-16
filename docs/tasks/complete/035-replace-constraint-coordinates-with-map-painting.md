# Replace Constraint Coordinates With Map Painting

Status: complete
Area: Procgen / Debug Tooling

## Goal

Remove the temporary Constraint X/Y controls and edit terrain constraints directly by painting on the greater-realm preview.

## Context

The current debug panel uses normalized Constraint X/Y controls followed by Ocean, Shallow, Valley, or Mountain stamp buttons. These controls were added only to exercise the engine-owned constraint field before preview pointer input existed.

Mapgen4 selects a terrain tool and converts pointer positions on the visible map into normalized constraint-field coordinates while the user paints. Constraint X/Y is not part of its interface and should not become permanent engine tooling.

## Prerequisite

Direct pointer painting on the greater-realm preview must be implemented and verified before Constraint X/Y is removed. The temporary coordinate controls must remain usable until painting fully replaces their ability to position and apply constraints.

## Acceptance Criteria

- Remove the Constraint X and Constraint Y rows from the debug panel.
- Replace one-shot coordinate stamp buttons with selectable Ocean, Shallow, Valley, and Mountain tools.
- Convert pointer positions over the rendered preview into normalized map coordinates.
- Paint continuously while the primary pointer button is held over the preview.
- Keep brush radius, falloff, and strength behavior in the engine-owned `TerrainConstraintField` module.
- Prevent painting when the pointer is outside the preview or interacting with UI controls.
- Preserve Clear constraints and deterministic regeneration behavior.
- Avoid regenerating more often than required for responsive painting.
- Add compile-gated tests for coordinate conversion and painting interaction state.
- Update `PROCGEN.md` after Constraint X/Y has been removed.

## Notes

This task changes debug tooling only. The serialized constraint format and generator integration from task 030 should remain unchanged.

## Implementation

- Removed the temporary Constraint X/Y rows and one-shot stamp behavior.
- Added selectable Ocean, Shallow, Valley, and Mountain brush buttons with visible selected state.
- Added the compile-gated, engine-neutral `TerrainConstraintPainting` module for preview-coordinate conversion, tool selection, duplicate suppression, and drag lifecycle.
- Added UI-aware game event delivery so preview painting ignores consumed UI input while mouse release still reliably ends a stroke.
- Composed paint samples with the existing `TerrainConstraintField` in `RogueFarmGame` without changing constraint serialization or generator integration.
- Batched paint-triggered generation so multiple pointer samples regenerate at most once per frame.
- Cancelled active strokes on pointer release, window focus loss, and window mouse leave.

## Verification

- Dedicated compile-gated tests cover preview corners and center, invalid/outside coordinates, selected tools, blocked UI input, drag state, duplicate suppression, release behavior, and constraint-field integration.
- All nine freshly configured test suites pass.
- Tests-disabled Debug and Release builds succeed.
- Release excludes the debug paint module and application behavior.
- Source audit confirms Constraint X/Y remains only in the historical task 030 context and its supersession note.
