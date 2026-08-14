# Add Press-And-Hold Stepper Repeat

Status: complete
Area: UI

## Goal

Allow users to hold a numeric stepper's increment or decrement button to change its value repeatedly after a short initial delay.

## Context

The engine's `+` and `-` controls currently require a separate click for every value change. Standard stepper controls apply one step immediately, then begin repeating at a faster cadence when the button remains held. This behavior should be implemented once in the reusable UI layer so procgen controls and future engine tools behave consistently.

## Acceptance Criteria

- Apply one increment or decrement immediately when a stepper button is pressed.
- Begin repeated changes only after a configurable initial hold delay.
- Repeat at a configurable, frame-rate-independent interval while the same button remains held.
- Stop repeating on pointer release, pointer cancellation, focus loss, or when the pointer interaction is otherwise terminated.
- Respect each control's existing step size, minimum, maximum, and disabled state.
- Do not emit redundant value-change callbacks after a value reaches its limit.
- Keep click behavior unchanged for users who release before the hold delay expires.
- Implement the behavior in shared UI code rather than in `GreaterRealmDebugPanel` or other application-specific panels.
- Add focused tests for immediate activation, delayed repeat, repeat cadence, release/cancellation, bounds, and coarse frame-time updates.
- Update `docs/UI.md` with the supported interaction once implemented.

## Notes

No dedicated branch is required. This is a focused reusable UI enhancement with limited merge risk.

## Implementation

- Added a reusable `ui::RepeatButton` that activates on press, waits 400 ms by default, and then repeats every 75 ms by default.
- Made repeat timing configurable and accumulated elapsed intervals so behavior remains independent of frame rate.
- Allowed repeat callbacks to stop the hold sequence when no further value change is possible.
- Routed pointer release back to the surface where the interaction began and cancelled active interactions on pointer leave or window focus loss.
- Replaced the greater-realm debug panel's numeric `+` and `-` buttons with repeat buttons while leaving command buttons as ordinary buttons.
- Prevented bounded procgen settings from regenerating when an attempted step would not change their value.

## Testing

- Added `tests/ui/RepeatButtonTests.cpp`, compiled only by the `REALM_BUILD_TESTS` test configuration with `REALM_TEST_BUILD` defined.
- Verified immediate activation, initial delay, repeat cadence, coarse frame-time catch-up, short presses, release, cancellation, pointer leave, disabled state, bounds, and timing sanitization.
- Built `out/build/debug-no-tests/realm_engine.exe` successfully.
- Ran all 10 CTest suites successfully from `out/build/debug-with-tests`.
- Built `out/build/release-no-tests/realm_engine.exe` successfully with tests disabled.
