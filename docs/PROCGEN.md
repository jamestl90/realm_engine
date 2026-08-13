# Procedural Generation Feature Inventory

This document tracks the procedural generation capabilities currently present in the engine.

## Greater Realm Generator

- Generates deterministic greater realm maps from a seed and settings.
- Uses a regular grid with configurable width, height, and cell size.
- The regular grid is the deliberate canonical representation following task 032's measured dual-mesh evaluation; direct indexing, compact implicit adjacency, and local-tile handoff outweigh an authoritative irregular mesh for current engine goals.
- Produces a signed landmass elevation field: negative values are water, positive values are land, and zero is the coastline.
- Combines unscaled five-octave land-shape noise at frequencies `1, 2, 4, 8, 16` with Mapgen4's square-distance island constraint and coastline-localized noise.
- Keeps broad land/water topology independent from inland terrain weights.
- Builds normalized final elevation with separate land-relief and water-depth paths.
- Shapes land with base elevation, explicit mountain peaks, ridges, valleys, and controlled terrain noise.
- Selects deterministic land-based mountain peaks with configurable spacing, then propagates a jagged distance field to form coherent mountain masses.
- Exports peak records plus per-cell peak distance, influence, and peak flags for hydrology and tooling.
- Applies terrain noise after base relief normalization so its weight remains independently tunable.
- Marks boundary-connected water as ocean.
- Computes distance to coast, explicit coastal-land boundary metadata, and local slope.
- Classifies water as ocean and land as plains, hills, highlands, or mountains, including land directly beside water.
- Treats coastline proximity independently from terrain form so coastal relief is not replaced by a generic beach classification.
- Accepts an optional, lower-resolution authored constraint field with ocean, shallow-water, valley, and mountain tools.
- Bilinearly samples authored constraints and blends them into the automatic signed terrain field.
- Generates normalized humidity, rainfall, and moisture from wind direction, raininess, rain shadow, evaporation, ocean sources, and terrain elevation.
- Builds a deterministic priority drainage topology with a downslope target or outlet for every cell.
- Exports a hydrologically conditioned elevation so drainage remains acyclic and downhill through depressions without changing visual terrain elevation.
- Accumulates terrain moisture downstream and exports renderer-independent river segments with flow and width.

## Terrain Constraints

- `TerrainConstraintField` is independent from greater-realm output resolution and supports smooth normalized-coordinate brush stamps.
- Tool values follow Mapgen4: ocean `-0.25`, shallow water `-0.05`, valley `+0.05`, and mountain `+1.0`.
- Unpainted samples have zero influence, preserving byte-for-byte deterministic automatic generation.
- Constraint data uses a versioned, validated little-endian binary representation through `serialize_terrain_constraints` and `deserialize_terrain_constraints`.

## Pipeline

1. Generate a signed broad landmass field from fBm and a square-distance island constraint.
2. Apply controlled noise near the coastline.
3. Convert the field into a signed landmass constraint at sea level.
4. Select spaced mountain peaks and propagate their jagged distance field.
5. Generate base elevation and the remaining inland relief influences.
6. Generate ocean depth separately.
7. Assemble normalized final elevation without changing land/water topology.
8. Classify boundary-connected water as ocean.
9. Compute coast distance, slope, and terrain forms.
10. Generate wind-driven humidity, rainfall, and moisture.
11. Build priority drainage and condition depressions for flow.
12. Accumulate moisture and export river segments.

This follows Mapgen4's layered elevation approach while deliberately retaining a regular-grid representation. A future irregular or triangulated surface may be derived for rendering without replacing canonical map data.

## Debugging And Tests

- The compile-gated `GreaterRealmDebug` module counts terrain forms and coastal land independently, converts map data into an engine-neutral RGBA image, overlays exported rivers, and marks explicit peak cells.
- The debug image uses relative ocean-depth shading and a one-cell dark coastline accent while preserving the underlying plains, hills, highlands, or mountain colour.
- `TextureManager` uploads the RGBA output without requiring procgen code to depend on SDL or GPU APIs.
- The application-level `GreaterRealmDebugPanel` owns the debug UI, active settings, and regeneration callbacks; `RogueFarmGame` owns preview placement, the editable constraint field, and composition. Controls expose terrain, mountain strength, peak spacing/radius/jaggedness, wind, rainfall, evaporation, river-flow, river-threshold, and coordinate-based constraint stamping settings.
- The current Constraint X/Y controls are a temporary debug harness. Unlike Mapgen4, the engine does not yet convert pointer positions on the preview into direct brush strokes; task 035 tracks removing these controls and replacing them with preview painting.
- Island bias follows Mapgen4's `0..1` range and `0.5` default. It changes the signed landmass constraint, so it can affect both coastline topology and water elevation before the separate ocean-depth stage.
- Terrain noise and ocean depth use larger tuning steps and contrast-enhanced debug shading so changes are visible.
- Automated tests cover output shape, deterministic seeds, map lookup, signed constraints, topology stability, ocean connectivity, sea-level response, terrain statistics, peak selection/spacing/distance fields, constraint interpolation/serialization, drainage invariants, climate response, river flow/connectivity, and debug-image output.
- Test code is compiled only when `RFD_BUILD_TESTS=ON` and does not enter release builds.

## Not Yet Supported

- Lakes as a classified terrain form, river erosion, deltas, or watershed metadata.
- Biomes or weather simulation beyond the current static rainfall and moisture fields.
- Resources, settlements, factions, or object placement.
- Local tile generation or world-region streaming.
- Beach, cliff, rocky-shore, marsh, delta, or other detailed shoreline classification.
- Direct pointer painting of terrain constraints on the debug preview.
- A derived Delaunay/Voronoi render surface; task 032 rejected it as the canonical greater-realm representation.
- Mapgen4-style folded terrain geometry or 2.5D projection.
- Climate-influenced terrain colouring.
