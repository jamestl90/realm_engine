# Add Dependency-Aware Procgen Regeneration

Status: complete
Area: Procgen / Debug Tooling / Performance

## Goal

Reuse unaffected greater-realm generation stages when a tuning control changes instead of rebuilding the complete map pipeline and GPU texture every time.

## Context

The 256x192 generator meets the immediate sub-50 ms target when the interactive procgen runtime is optimized, but every control currently regenerates terrain fields, mountain peaks, relief, classification, drainage, channels, the debug image, and a new texture. Many settings affect only a subset of those stages.

## Acceptance Criteria

- Define explicit dependencies and dirty states for terrain fields, peaks, relief, classification, drainage, channels, debug image, and texture upload.
- Regenerate the full pipeline for seed, dimensions, broad landmass, sea-level, and authored-constraint changes where required.
- Reuse upstream results for mountain, relief, drainage, and channel-only settings.
- Avoid rebuilding drainage when the conditioned elevation and water topology are unchanged.
- Update the existing debug texture in place, without creating a new texture or waiting for global GPU idle on each change.
- Preserve deterministic output and current engine-neutral procgen boundaries.
- Add tests that verify partial paths produce the same map data as a clean full regeneration.
- Add timing coverage for representative full and partial changes at 256x192.
- Update `PROCGEN.md` with the resulting invalidation behavior.

## Dependencies

- Tasks 050 through 053. The generation stage contracts must settle before they become cache and invalidation boundaries.

## Implementation Summary

- Added `GreaterRealmGenerationCache` with explicit terrain-field, mountain-peak, relief, classification, drainage, river-channel, debug-image, and texture-upload dirty stages.
- Retained sampled terrain layers and prior settings so regeneration starts at the earliest affected stage and expands through required dependents.
- Kept authored constraints explicit: painting and clearing invalidate terrain fields, while repeated stroke samples remain coalesced to one regeneration per frame.
- Made the debug panel's Regenerate command an explicit full-pipeline request; ordinary setting callbacks rely on settings-difference planning.
- Added same-size RGBA texture updates that preserve the existing `TextureID` and submit transfer work without `SDL_WaitForGPUIdle`. Dimension changes still create and swap a texture.
- Kept stage timing instrumentation behind `REALM_ENABLE_PROCGEN_PROFILING` and added a dedicated test target for exact partial/full equivalence and representative `256x192` timings.
- Updated `docs/PROCGEN.md` with the invalidation table and texture-upload behavior.

## Testing

- Focused staged-regeneration, greater-realm, and debug-panel tests passed.
- Staged-regeneration tests verified exact map equivalence for peak-, relief-, classification-, drainage-, channel-, debug-image-, texture-only, authored-constraint, sea-level, and forced full paths.
- Representative unoptimized Debug-test timings at `256x192`: full `297.04 ms`, relief-only `113.49 ms`, and channel-only `23.38 ms`; skipped stages reported zero time.
- Full Debug suite passed: 14/14 tests.
- Standalone `debug-no-tests` and `release-no-tests` builds succeeded.
- `git diff --check` passed.


## Commit Message

Add dependency-aware procgen regeneration

Update debug textures in place without GPU idle waits

