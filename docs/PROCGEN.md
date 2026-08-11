# Procedural Generation Feature Inventory

This document tracks the procedural generation capabilities currently present in the engine.

## Greater Realm Generator

- Generates deterministic greater realm maps from a seed and settings.
- Uses a regular grid with configurable width, height, and cell size.
- Produces a signed landmass elevation field: negative values are water, positive values are land, and zero is the coastline.
- Uses low-frequency land shape, edge falloff, and coastline-localized noise to form continents, islands, and coastlines.
- Keeps broad land/water topology independent from inland terrain weights.
- Builds normalized final elevation with separate land-relief and water-depth paths.
- Shapes land with base elevation, mountains, ridges, valleys, and controlled terrain noise.
- Marks boundary-connected water as ocean.
- Computes distance to coast and local slope.
- Classifies cells as ocean, coast, plains, hills, highlands, or mountains.

## Pipeline

1. Generate the broad landmass field.
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

- A compile-gated debug view renders terrain forms as a coloured texture.
- Debug UI controls expose seed, sea level, and major terrain weights.
- Automated tests cover output shape, deterministic seeds, map lookup, signed constraints, topology stability, ocean connectivity, and sea-level response.
- Test code is compiled only when `RFD_BUILD_TESTS=ON` and does not enter release builds.

## Not Yet Supported

- Hydrology, drainage basins, rivers, or lakes as a classified terrain form.
- Biomes, climate, rainfall, or weather.
- Resources, settlements, factions, or object placement.
- Local tile generation or world-region streaming.
- Editable landmass constraints.
- Delaunay/Voronoi mesh generation.
