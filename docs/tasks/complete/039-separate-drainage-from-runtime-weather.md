# Separate Drainage From Runtime Weather

Status: complete
Area: Procgen / Hydrology

## Goal

Keep generated geography independent from weather by deriving drainage and potential river channels from terrain-only catchment area.

## Context

The greater-realm generator currently creates humidity, rainfall, and moisture during world generation, then uses generated moisture as river flow. Rain should instead be a runtime weather event that may later produce transient runoff and river discharge without rebuilding the world's geography.

## Acceptance Criteria

- Remove generated humidity, rainfall, and moisture from greater-realm cells and generator settings.
- Preserve deterministic drainage topology and accumulate terrain-only contributing area for every cell.
- Export potential river channels using a configurable catchment-area threshold.
- Keep channel width as generated presentation metadata, without representing current water discharge.
- Replace generated-weather controls and debug views with catchment-area terminology.
- Document the boundary between generated geography and future runtime weather.
- Add compile-gated tests for catchment accumulation, deterministic channel export, connectivity, and thresholding.

## Out Of Scope

- Runtime weather events and precipitation.
- Seasonal or event-driven runoff and river discharge.
- Biome-dependent rainfall probability.

## Implementation

- Removed the generated climate pass and all generated humidity, rainfall, and moisture fields and controls.
- Replaced moisture-derived flow with per-cell contributing area accumulated through the existing priority drainage topology.
- Exported potential river channels using a configurable minimum catchment area and derived display width.
- Replaced climate and flow debug views with a logarithmically scaled catchment-area view.
- Added task 040 for runtime weather, runoff, and transient river discharge.

## Verification

- All eight freshly rebuilt Debug test suites pass.
- Focused hydrology tests cover physical cell-area contribution, downstream accumulation, connectivity, threshold independence, deterministic output, and channel visualization.
- Tests-disabled Debug and Release game builds succeed.
- Compiled engine and test sources contain no generated climate API, weather setting, or moisture-derived flow references.
