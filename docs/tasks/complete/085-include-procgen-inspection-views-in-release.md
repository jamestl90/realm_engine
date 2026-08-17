# Include Procgen Inspection Views In Release

Status: complete
Priority: high
Area: Build / Procgen Inspection

## Goal

Keep greater-realm terrain, climate, biome, and related inspection views available in shipped Release builds so applications can expose them to users without requiring a special Debug binary.

## Context

The existing `REALM_ENABLE_PROCGEN_DEBUG_VIEW` gate excludes the entire greater-realm inspection panel from the `release-no-tests` preset. These views are useful as application-facing world inspection tools even when Debug-only profiling is not present.

## Acceptance Criteria

- Enable greater-realm inspection views in the shipped Debug and Release presets.
- Keep the build option configurable for embedders that deliberately want to omit the inspection surface.
- Keep procgen timing and profiling instrumentation Debug-only.
- Ensure changed preset cache variables are reapplied in existing build trees.
- Update canonical architecture and procgen documentation.
- Verify Debug and Release application builds and the full test suite.

## Out Of Scope

- Adding new climate or weather visualization modes.
- Runtime weather simulation changes.
- Runtime runoff, discharge, flooding, or erosion.

## Implementation Decisions

- `REALM_ENABLE_PROCGEN_DEBUG_VIEW` remains a configurable build option for compatibility, but now defaults to `ON` and is explicitly enabled by all shipped presets.
- The existing macro and internal debug-oriented type names remain unchanged to avoid an unrelated compatibility rename. Canonical documentation describes the surface as procgen inspection functionality.
- `REALM_ENABLE_PROCGEN_PROFILING` remains tied to Debug configuration, so Release includes inspection views without timing instrumentation.
- `scripts/build.ps1` reapplies the selected configure preset before every build so preset cache changes take effect in existing build directories.

## Verification

- `scripts/build.ps1 -Preset release-no-tests` passed and compiled `GreaterRealmDebugPanel.cpp`, `GreaterRealmDebug.cpp`, climate, biome, terrain painting, seasonal climate, and weather sources into the Release executable.
- `out/build/release-no-tests/CMakeCache.txt` reports `REALM_ENABLE_PROCGEN_DEBUG_VIEW:BOOL=ON`.
- `scripts/build.ps1 -Preset debug-no-tests` passed.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 16/16.
- `git diff --check` passed with only the repository's existing LF-to-CRLF warnings.

## Commit Message

`build: include procgen inspection views in release`
