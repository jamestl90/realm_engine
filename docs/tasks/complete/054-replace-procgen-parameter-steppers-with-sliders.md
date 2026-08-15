# Replace Procgen Parameter Steppers With Sliders

Status: complete
Priority: medium
Area: UI / Procedural Generation Debug Controls

## Goal

Replace the greater-realm debug panel's numeric parameter `+` and `-` stepper rows with slider controls that make parameter ranges, current values, and coarse tuning easier to read and adjust.

## Context

The current procgen controls expose many tunable settings through compact label plus decrement/increment buttons. This works for precise stepping but makes ranges hard to scan and requires many clicks for larger changes. Sliders are a better fit for continuous generator parameters such as sea level, island bias, relief weights, peak settings, terrain noise, ocean depth, elevation scale, and channel threshold.

## Acceptance Criteria

- Add or reuse a shared UI slider control rather than implementing slider behavior only inside `GreaterRealmDebugPanel`.
- Replace greater-realm debug panel parameter steppers with labeled slider rows.
- Place each slider below its label text, with the visible label showing the parameter name and current formatted value.
- Preserve each setting's existing minimum, maximum, step/granularity, clamping behavior, regeneration callback, and presentation-update callback semantics.
- Support pointer drag, click-to-set or equivalent direct manipulation, keyboard/focus behavior if consistent with existing UI controls, disabled state, and cancellation/focus-loss behavior.
- Keep command buttons such as `Regenerate`, `Random Seed`, presentation mode, overlays, clear constraints, and brush tool selection as buttons.
- Ensure labels and sliders fit within the current debug panel columns without text overlap or layout jumps.
- Add focused UI tests for slider value mapping, clamping, callbacks, dragging/cancellation, and bounds.
- Add or update debug-panel tests or a focused smoke path proving procgen settings still update and regenerate appropriately.
- Update `docs/UI.md` or another owning UI capability document if slider support becomes a reusable engine control.

## Dependencies

- Existing greater-realm debug panel controls from tasks 018, 037, 038, and 046.

## Notes

No dedicated branch is required. This is a focused UI/control improvement unless slider support expands into broader UI architecture work.

## Implementation Summary

- Added reusable `ui::Slider` with range clamping, step snapping, value callbacks, drag/click input, disabled visuals, pointer cancellation, and rectangular renderer support.
- Routed active pointer motion to the pressed UI surface so drag controls continue to receive movement while held.
- Converted greater-realm tuning rows and elevation scale to label-above-slider controls while keeping Seed, Regenerate, Random Seed, view, overlay, clear, and brush-tool actions as buttons.
- Added a debug-panel smoke test that drives the actual slider event path and verifies generator sliders regenerate while the elevation presentation slider only emits a presentation update.
- Updated `docs/UI.md` with the reusable slider capability.

## Testing

- Passed: `cmake --build out/build/debug-with-tests --target realm_ui_slider_tests realm_terrain_mesh_tests`
- Passed: `cmake --build out/build/debug-with-tests --target realm_engine`
- Passed: `cmake --build out/build/debug-with-tests --target realm_greater_realm_debug_panel_tests`
- Passed: `ctest --test-dir out/build/debug-with-tests --output-on-failure` (13/13)

## Commit Message

Add reusable sliders for procgen tuning controls
