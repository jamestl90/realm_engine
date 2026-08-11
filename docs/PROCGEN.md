# Procedural Generation Feature Inventory

This document tracks the procedural generation capabilities currently present in the engine.

## Greater Realm Generator

- Generates deterministic greater realm maps from a seed and settings.
- Uses a regular grid with configurable width, height, and cell size.
- Produces a signed landmass elevation field: negative values are water, positive values are land, and zero is the coastline.
- Combines five-octave land-shape noise with Mapgen4's square-distance island constraint and coastline-localized noise.
- Keeps broad land/water topology independent from inland terrain weights.
- Builds normalized final elevation with separate land-relief and water-depth paths.
- Shapes land with base elevation, mountains, ridges, valleys, and controlled terrain noise.
- Applies terrain noise after base relief normalization so its weight remains independently tunable.
- Marks boundary-connected water as ocean.
- Computes distance to coast and local slope.
- Classifies cells as ocean, coast, plains, hills, highlands, or mountains.

## Pipeline

1. Generate a signed broad landmass field from fBm and a square-distance island constraint.
2. Apply controlled noise near the coastline.
3. Convert the field into a signed landmass constraint at sea level.
4. Generate base elevation and inland relief influences.
5. Generate ocean depth separately.
6. Assemble normalized final elevation without changing land/water topology.
7. Classify boundary-connected water as ocean.
8. Compute coast distance and slope.
9. Classify terrain forms.

This follows Mapgen4's layered elevation approach while retaining the engine's current regular-grid representation.

## Debugging And Tests

- The compile-gated `GreaterRealmDebug` module counts terrain forms and converts map data into an engine-neutral RGBA image, including relative ocean-depth shading.
- `TextureManager` uploads the RGBA output without requiring procgen code to depend on SDL or GPU APIs.
- The application-level `GreaterRealmDebugPanel` owns the debug UI, active settings, and regeneration callbacks; `RogueFarmGame` owns preview placement and composes these pieces. The controls expose seed, sea level, land shape, island bias, coastline detail, base relief, mountain, ridge, valley, terrain noise, and ocean depth.
- Island bias defaults to Mapgen4's `0.5`; terrain noise and ocean depth use larger tuning steps and contrast-enhanced debug shading so changes are visible.
- Automated tests cover output shape, deterministic seeds, map lookup, signed constraints, topology stability, ocean connectivity, sea-level response, terrain statistics, and debug-image output.
- Test code is compiled only when `RFD_BUILD_TESTS=ON` and does not enter release builds.

## Not Yet Supported

- Hydrology, drainage basins, rivers, or lakes as a classified terrain form.
- Biomes, climate, rainfall, or weather.
- Resources, settlements, factions, or object placement.
- Local tile generation or world-region streaming.
- Editable landmass constraints.
- Delaunay/Voronoi mesh generation.
