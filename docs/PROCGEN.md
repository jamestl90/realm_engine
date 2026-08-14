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
- Reserves fixed mountain headroom while normalizing land relief, then applies mountain strength only through the peak-distance influence. Changing mountain strength does not alter the denominator or shift uninfluenced terrain, and the default strength preserves the established terrain and drainage output.
- Selects deterministic land-based mountain peaks with configurable spacing, then propagates a jagged distance field to form coherent mountain masses.
- Exports peak records plus per-cell peak distance, influence, and peak flags for hydrology and tooling.
- Applies terrain noise after base relief normalization so its weight remains independently tunable.
- Marks boundary-connected water as ocean.
- Computes distance to coast, explicit coastal-land boundary metadata, and local slope.
- Classifies water as ocean and land as plains, hills, highlands, or mountains, including land directly beside water.
- Treats coastline proximity independently from terrain form so coastal relief is not replaced by a generic beach classification.
- Accepts an optional, lower-resolution authored constraint field with ocean, shallow-water, valley, and mountain tools.
- Bilinearly samples authored constraints and blends them into the automatic signed terrain field.
- Builds a deterministic priority drainage topology with a downslope target or outlet for every cell.
- Exports a hydrologically conditioned elevation so drainage remains acyclic and downhill through depressions without changing visual terrain elevation.
- Accumulates terrain-only contributing area downstream and exports renderer-independent potential river channels with catchment area and a derived display width.
- Does not generate rainfall, humidity, soil moisture, runoff, or current river discharge. Those values belong to future runtime weather and world simulation.

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
10. Build priority drainage and condition depressions for downhill routing.
11. Accumulate contributing terrain area and export potential river channels.

This follows Mapgen4's layered elevation approach while deliberately retaining a regular-grid representation. The renderer can derive a triangulated regular-grid heightfield for 2.5D presentation without replacing or mutating canonical map data.

## Debugging And Tests

- The compile-gated `GreaterRealmDebug` module counts terrain forms and coastal land independently, converts map data into an engine-neutral RGBA image, overlays exported rivers, and marks explicit peak cells.
- The debug image uses relative ocean-depth shading and a one-cell dark coastline accent while preserving the underlying plains, hills, highlands, or mountain colour.
- Runtime base views expose terrain forms, elevation, signed landmass, mountain influence, slope, coast distance, and catchment area.
- Coastlines, mountain peaks, rivers, and sampled drainage directions are independent overlays. Terrain with coastlines, peaks, and rivers enabled remains the default view.
- Changing a base view or overlay rebuilds only the RGBA image and preview texture from the retained map; it does not regenerate procedural data.
- The application can switch between the flat debug texture and a lit oblique `3D` heightfield. The heightfield reuses the active debug colours and overlays, and its elevation scale is presentation-only.
- `TextureManager` uploads the RGBA output without requiring procgen code to depend on SDL or GPU APIs.
- The application-level `GreaterRealmDebugPanel` owns the debug UI, active settings, selected constraint tool, and regeneration callbacks; `RogueFarmGame` owns preview placement, the editable constraint field, and composition. Controls expose terrain, mountain strength, peak spacing/radius/jaggedness, potential-channel catchment threshold, and selectable Ocean, Shallow, Valley, and Mountain brushes.
- Primary-button input over the visible preview maps directly to normalized constraint coordinates and paints continuously while dragged. UI-consumed input and positions outside the preview cannot paint, and generated output is rebuilt at most once per frame while a stroke is active.
- The compile-gated `TerrainConstraintPainting` module owns preview-coordinate conversion and drag state without depending on SDL, UI widgets, rendering, or application classes.
- Island bias follows Mapgen4's `0..1` range and `0.5` default. It changes the signed landmass constraint, so it can affect both coastline topology and water elevation before the separate ocean-depth stage.
- Terrain noise and ocean depth use larger tuning steps and contrast-enhanced debug shading so changes are visible.
- Debug builds report per-stage generation timings plus end-to-end control-to-preview timing through the Debug-only `REALM_ENABLE_PROCGEN_PROFILING` definition; profiling code is compiled out of production builds.
- `REALM_OPTIMIZE_PROCGEN_DEBUG` defaults to `ON`, compiling the interactive procgen runtime with optimization in Debug builds while dedicated test targets retain their normal Debug checks. Disable it when stepping through procgen at instruction level is more important than interactive tuning speed.
- Automated tests cover output shape, deterministic seeds, map lookup, signed constraints, topology stability, ocean connectivity, sea-level response, terrain statistics, peak selection/spacing/distance fields, constraint interpolation/serialization, preview-coordinate mapping, paint interaction state, drainage invariants, catchment accumulation, channel connectivity, and debug-image output.
- Test code is compiled only when `REALM_BUILD_TESTS=ON` and does not enter release builds.

## Not Yet Supported

- Lakes as a classified terrain form, river erosion, deltas, or watershed metadata.
- Runtime weather, precipitation, runoff, soil moisture, or active river discharge; task 040 tracks this future simulation layer.
- Resources, settlements, factions, or object placement.
- Local tile generation or world-region streaming.
- Beach, cliff, rocky-shore, marsh, delta, or other detailed shoreline classification.
- A derived Delaunay/Voronoi render surface; task 032 rejected it as the canonical greater-realm representation.
- Mapgen4's irregular folded mesh around coasts, ridges, valleys, and rivers. The supported 2.5D path is a continuous triangulated regular-grid heightfield instead.
- Continuous elevation-informed terrain colouring.
- Dependency-aware partial regeneration and in-place debug texture updates; task 036 tracks avoiding full-pipeline work for every control change.
