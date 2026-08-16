# Render Combo Box Dropdowns As Popups

Status: complete
Area: UI / Rendering

## Goal

Ensure open combo-box dropdowns render above all normal UI elements regardless of tree sibling order.

## Acceptance Criteria

- Render combo-box headers during normal UI traversal.
- Defer open dropdown backgrounds, hover states, and item text to a final popup pass.
- Preserve existing combo-box hit-testing and selection behavior.
- Support more than one open combo box deterministically.
- Verify the procgen debug-view dropdown renders over the controls beneath it.
- Build Debug and Release and run the UI test suite.

## Verification

- Added a regression test confirming an open dropdown does not change combo-box layout height.
- UI and full engine test suites pass.
- Debug and Release runtime targets build successfully.
