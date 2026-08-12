# Add Rainfall And Moisture Fields

Status: testing
Area: Procgen / Climate

## Goal

Generate deterministic rainfall, humidity, and terrain moisture fields for later rivers and biome generation.

## Context

Mapgen4 models wind ordering, evaporation, raininess, and rain shadow. The engine currently has elevation only and cannot vary river flow or future biomes by water supply.

## Dependencies

- Stable greater-realm elevation output.

## Acceptance Criteria

- Add configurable wind direction, raininess, rain-shadow, and evaporation inputs.
- Export normalized rainfall and moisture data without assigning biomes.
- Account for ocean moisture sources and elevation-driven rain shadow.
- Keep the result deterministic for identical seeds and settings.
- Add tests for value ranges, determinism, wind response, and dry/wet parameter response.
- Expose only controls useful for evaluating this layer in the compile-gated debug view.
- Update the procgen inventory and pipeline documentation.

## Implementation

- Added configurable wind angle, raininess, rain shadow, and evaporation settings.
- Added a deterministic wind-ordered climate pass with ocean humidity sources and elevation-driven orographic rainfall.
- Exported normalized humidity, rainfall, and moisture on every greater-realm cell.
- Added compact compile-gated controls for each climate parameter.

## Verification

- Dedicated tests validate normalized ranges, deterministic output, zero-rain and wet response, wind-direction response, and terrain-topology independence.
- All six CTest targets pass.
- Tests-disabled Debug and Release builds succeed.
- The Debug executable passes a native startup smoke test.
