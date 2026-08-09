# Add Procgen Debug Controls

Status: testing
Area: Procgen / UI
Branch: `proc-gen`
Branch reason: This continues the procedural generation feature stream and builds on the debug map renderer.

## Goal

Add debug UI controls for tuning the greater realm generator without recompiling.

## Context

The generated map can now be rendered, but tuning requires code edits and rebuilds. The next step is to expose a small debug-only control panel for changing the seed and key generator weights while viewing the result.

## Acceptance Criteria

- Add compile-gated debug controls behind `RFD_ENABLE_PROCGEN_DEBUG_VIEW`.
- Allow changing seed, sea level, mountain weight, ridge weight, valley weight, and terrain noise weight.
- Regenerate the debug map texture from the UI.
- Show the active settings and basic terrain counts.
- Keep the debug controls out of release builds by default.
- Verify debug game, procgen tests, and release game builds.

## Notes

Use existing UI controls for now. A dedicated slider control can be added later if this panel becomes important.
