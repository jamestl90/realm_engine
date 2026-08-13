# Add Dependency-Aware Procgen Regeneration

Status: todo
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
