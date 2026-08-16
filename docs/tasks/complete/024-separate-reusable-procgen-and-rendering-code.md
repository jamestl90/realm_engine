# Separate Reusable Procgen And Rendering Code

Status: complete
Area: Architecture / Procgen / Rendering

## Goal

Keep reusable engine and procgen tooling out of the host application class.

## Context

The host application previously owned reusable terrain statistics, greater realm debug colourization, SDL surface generation, raw texture conversion, and retained UI rendering in addition to application-facing procgen controls and preview composition.

## Acceptance Criteria

- Audit the host application entry point and classify each responsibility as application-specific or reusable engine/tooling behavior.
- Move greater realm terrain counting and debug-map visualization out of the host application entry point into an appropriate reusable module.
- Keep the core procgen generator independent of SDL and rendering APIs; place visualization adapters at the procgen/rendering boundary.
- Extract reusable texture, sprite, or UI render-pass lifecycle behavior where the audit shows it is engine-wide.
- Keep procgen settings ownership, control layout, labels, callbacks, and preview composition in application or demo code.
- Avoid introducing an abstraction for behavior that is genuinely unique to this application.
- Preserve the `REALM_ENABLE_PROCGEN_DEBUG_VIEW` compile gate and keep debug visualization code out of release builds by default.
- Add focused tests for extracted engine-neutral logic and verify tests-disabled Debug and Release builds.
- Update architecture and procgen documentation to record the resulting ownership boundary.

## Implementation

- Added engine-neutral terrain statistics, palette mapping, and RGBA image generation in `GreaterRealmDebug`.
- Added generic tightly packed RGBA8 texture upload to `TextureManager`; SDL surface conversion now reuses that path.
- Moved retained UI rendering and UI-tree shutdown ownership into `Engine`.
- Moved the host-facing control panel into `GreaterRealmDebugPanel`.
- Kept generator settings, regeneration timing, preview entity placement, scale, and texture replacement orchestration in host application code.
- Compile-gated both the visualization module and application panel out of normal Release builds.

## Verification

- Tests-disabled Debug build succeeds with `REALM_ENABLE_PROCGEN_DEBUG_VIEW=ON`.
- Tests-disabled Release build succeeds with `REALM_ENABLE_PROCGEN_DEBUG_VIEW=OFF`.
- Release build graph contains neither `GreaterRealmDebug.cpp` nor `GreaterRealmDebugPanel.cpp`.
- All five CTest targets pass, including procgen coverage for terrain counts, RGBA output, depth shading, and malformed map storage.
- A native runtime smoke test initializes the sprite pipeline, procgen texture, font system, retained UI, and renderer without errors.
- `git diff --check` passes.

## Notes

This task changes ownership and dependency direction without changing generated terrain or redesigning the controls.
