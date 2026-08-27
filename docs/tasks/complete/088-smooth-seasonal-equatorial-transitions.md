# Smooth Seasonal Equatorial Transitions

Status: complete
Priority: high
Area: World Simulation / Climate

## Goal

Remove the artificial equatorial seam from seasonal temperature and precipitation while preserving deterministic calendar-driven evaluation and stable biome ownership.

## Context

Seasonal evaluation currently chooses the northern or southern phase with a binary latitude-sign branch. Default hemisphere phases are opposite, so adjacent cells on either side of the equator can receive nearly opposite seasonal offsets despite being geographically close.

## Acceptance Criteria

- Blend northern and southern seasonal phases smoothly across an explicit equatorial transition band.
- Apply the same continuity rule to seasonal temperature and seasonal precipitation.
- Keep clear opposite-season behavior outside the transition band.
- Version and fingerprint the changed seasonal data contract and settings.
- Preserve terrain, annual climate normals, hydrology, biomes, and seasonal-climate ownership boundaries.
- Add focused tests for equatorial continuity, deterministic output, setting validation, and hemisphere opposition.
- Update canonical architecture/procgen documentation.
- Verify the focused tests, full suite, and Debug and Release builds.

## Out Of Scope

- Climate change or drifting annual normals.
- Retuning annual temperature, annual precipitation, or biome rules.
- Weather fronts, atmospheric circulation cells, runoff, or erosion.

## Implementation Decisions

- Added `equatorial_transition_degrees` to both seasonal settings structures. It is the latitude half-width of the handoff, defaults to `15` degrees, and clamps to `1..90` degrees.
- Each evaluator computes both hemisphere phase waves. A smoothstep latitude weight selects the full northern wave at and above the positive transition edge, the full southern wave at and below the negative edge, and a continuous interpolation between them.
- Regional phase variation shifts both hemisphere waves equally, preserving their configured opposition while retaining deterministic regional character.
- Bumped seasonal temperature and precipitation map versions from `1` to `2` and included transition width in settings fingerprints.
- Added data-level continuity coverage at 193 rows and rendered-pixel coverage for the seasonal temperature and seasonal precipitation inspector views.

## Verification

- Focused `ctest --test-dir out/build/debug-with-tests -R climate --output-on-failure` passed, 2/2.
- Data-level tests confirmed opposite northern/southern phases, a neutral equatorial midpoint, and bounded changes between immediately adjacent north/south rows for temperature and precipitation.
- Rendered-pixel tests passed for the seasonal temperature and seasonal precipitation inspector views.
- Runtime-weather inspection coverage from the original implementation was removed by the later climate simplification.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 17/17.
- `scripts/build.ps1 -Preset debug-no-tests` passed.
- `scripts/build.ps1 -Preset release-no-tests` passed.
- `git diff --check` and a targeted trailing-whitespace scan passed with only the repository's existing LF-to-CRLF warnings.

## Commit Message

`fix(world): smooth seasonal equatorial transitions`
