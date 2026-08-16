# Add Procgen Brush Settings

Status: complete
Priority: medium
Area: Procedural Generation / Terrain Constraint Painting

## Goal

Expose brush settings for authored terrain-constraint painting so users can tune brush size and strength while editing the greater-realm constraint field.

## Context

The current debug panel supports selecting Ocean, Shallow, Valley, and Mountain tools and painting directly onto the visible preview, but brush behavior is fixed. Once parameter controls use sliders, the same control style can expose brush settings without making the panel feel like a wall of stepper buttons.

## Acceptance Criteria

- Add brush-size control for terrain constraint painting.
- Add brush-strength or influence control if it fits the existing constraint-field semantics; otherwise record why brush strength remains fixed.
- Use the slider control introduced by task 054 for brush settings.
- Keep brush labels above sliders, matching the parameter-control layout.
- Show current brush setting values in the labels.
- Ensure painting uses the selected settings for press, drag, and continuous strokes without changing behavior outside the preview.
- Preserve deterministic constraint sampling and serialization format unless a versioned change is required.
- Add focused tests for brush setting clamping, paint radius/effective influence, stroke continuity, and no-paint behavior outside the preview.
- Update `docs/PROCGEN.md` with the supported authored-painting controls.

## Dependencies

- Task 054, so brush controls can reuse the slider UI pattern.
- Task 030's editable terrain constraints and task 035's map-preview painting behavior.

## Notes

Brush settings are application/debug editing policy. Reusable terrain constraint sampling and serialization should remain engine-neutral.

## Implementation Summary

- Added `TerrainConstraintBrushSettings` with normalized brush radius and strength, including clamping helpers and defaults matching the previous fixed behavior.
- Extended paint samples and sessions so press, drag, and continuous strokes carry the current brush radius and strength into `TerrainConstraintField::paint`.
- Added brush size and brush strength sliders to the greater-realm debug panel using the reusable `ui::Slider` pattern from task 054.
- Reworked the paint controls into a clearer mutually exclusive Ocean/Shallow/Valley/Mountain paint-type row, with selected-state styling separate from command buttons.
- Preserved the terrain constraint serialization format; brush size and strength are stroke policy, not stored field data.
- Updated `docs/PROCGEN.md` with the supported authored-painting controls.

## Testing

- Passed: `cmake --build out/build/debug-with-tests --target realm_procgen_paint_tests realm_greater_realm_debug_panel_tests realm_engine`
- Passed: `ctest --test-dir out/build/debug-with-tests --output-on-failure` (13/13)

## Commit Message

Add adjustable procgen brush settings
