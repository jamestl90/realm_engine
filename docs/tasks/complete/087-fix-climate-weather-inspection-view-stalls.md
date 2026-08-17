# Fix Climate And Weather Inspection View Stalls

Status: complete
Priority: high
Area: World Simulation / Procgen Inspection

## Goal

Make seasonal and runtime climate/weather inspection views switch promptly at greater-realm resolution without changing their rendered values or any generation behavior.

## Context

The application compositor validates all source maps before rendering, but then calls `sample_climate_weather_cell` for every output pixel. That safe query revalidates the climate source fingerprint on every call, turning a 256x192 image into quadratic work and causing user-visible stalls exceeding 30 seconds.

## Acceptance Criteria

- Validate terrain, climate, seasonal, and runtime-weather provenance once per inspection image.
- Compose each already-validated output cell in constant time.
- Preserve every inspection palette, overlay, sampled wind vector, and composed climate/weather value.
- Preserve terrain, climate-normal, hydrology, biome, seasonal, and weather generation behavior.
- Add a greater-realm-resolution performance regression test.
- Verify the focused test, full suite, and Debug and Release builds.

## Out Of Scope

- Changing the public climate/weather query contract.
- Retuning climate, seasons, weather, or biomes.
- Background rendering, multithreading, or GPU visualization.

## Implementation Decisions

- Retained the compositor's existing whole-image provenance checks, which reject stale terrain, climate, seasonal, or weather inputs before allocating pixels.
- Replaced the per-pixel safe world-query call with constant-time composition from already-validated climate, seasonal, and weather cells. The composed fields and clamping match `sample_climate_weather_cell` exactly.
- Left the public checked query unchanged. This fix does not broaden its contract or weaken validation for gameplay consumers.
- Added a 256x192 render regression covering `Experienced precipitation`, the last affected view, with a conservative two-second interaction budget.

## Verification

- Focused Debug regression rendered a 256x192 `Experienced precipitation` image in 12 ms and passed.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 17/17; the climate/weather inspection target completed in 0.56 seconds including map and weather setup.
- `scripts/build.ps1 -Preset debug-no-tests` passed.
- `scripts/build.ps1 -Preset release-no-tests` passed.
- The Release application remained responsive after selecting the first affected `Seasonal Temperature` view.
- `git diff --check` passed with only the repository's existing LF-to-CRLF warnings.

## Commit Message

`fix(world): prevent climate inspection view stalls`
