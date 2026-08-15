# Procedural Generation Feature Inventory

This document tracks the procedural generation capabilities currently present in the engine.

## Greater Realm Generator

- Generates deterministic greater realm maps from a seed and settings.
- Uses a regular grid with configurable width, height, and cell size.
- The regular grid is the deliberate canonical representation following task 032's measured dual-mesh evaluation; direct indexing, compact implicit adjacency, and local-tile handoff outweigh an authoritative irregular mesh for current engine goals.
- Produces a signed landmass elevation field: negative values are water, positive values are land, and zero is the coastline.
- Combines unscaled five-octave land-shape noise at frequencies `1, 2, 4, 8, 16` with Mapgen4's square-distance island constraint and coastline-localized noise.
- Applies Mapgen4's automatic positive-land mountain hint to the signed landmass field before authored constraints and coastline noise, so stronger inland signed constraints can locally select mountain relief without changing land/water sign.
- Keeps broad land/water topology independent from inland terrain weights.
- Builds normalized final elevation with separate land-relief and water-depth paths.
- Shapes land by computing separate low-amplitude hill relief and peak-distance mountain relief, then blending from hills toward mountains with the squared positive signed landmass constraint.
- Keeps base relief as the hill stage. Mountain strength raises only the explicit peak-distance mountain target, so changing it does not shift terrain with no peak influence.
- Applies ridges, valleys, and terrain noise after the hill-to-mountain blend as secondary engine extensions, masked to positive inland constraint strength. Ridges raise relief, valleys lower relief, and terrain noise can move relief both directions without changing land/water topology.
- Selects deterministic mountain peak sites from the canonical grid before considering mutable land/water output, using configurable Poisson-style spacing with a center-biased priority to avoid bucket-aligned artifacts.
- Exports only fixed peak sites that currently sit on land as active mountain peaks; fixed sites under water remain dormant and can become active if authored painting later creates land there.
- Exports peak records plus per-cell peak distance, influence, and peak flags for hydrology and tooling.
- Exports per-cell hill and mountain relief stages for debugging and tests.
- Marks boundary-connected water as ocean.
- Computes distance to coast, explicit coastal-land boundary metadata, and local slope.
- Classifies water as ocean and land as plains, hills, highlands, or mountains, including land directly beside water.
- Treats coastline proximity independently from terrain form so coastal relief is not replaced by a generic beach classification.
- Accepts an optional, lower-resolution authored constraint field with ocean, shallow-water, valley, and mountain tools.
- Bilinearly samples authored constraints and blends them once into the automatic signed terrain field. The resulting signed field controls water/land topology, coastline perturbation, and positive-land relief selection; painted values are not reapplied later as direct final-relief targets.
- Builds a deterministic priority drainage topology with a downslope target or outlet for every cell.
- Exports a hydrologically conditioned elevation so drainage remains acyclic and downhill through depressions without changing visual terrain elevation.
- Accumulates terrain-only contributing area downstream and exports renderer-independent potential river channels with catchment area and a derived display width. Channels may terminate near the coast, but coastal land cells are not exported as river segment endpoints, and near-coast exported segments must move toward the coastline rather than along it.
- Does not generate rainfall, humidity, soil moisture, runoff, or current river discharge. Those values belong to future runtime weather and world simulation.

## Terrain Constraints

- `TerrainConstraintField` is independent from greater-realm output resolution and supports smooth normalized-coordinate brush stamps.
- Tool values follow Mapgen4: ocean `-0.25`, shallow water `-0.05`, valley `+0.05`, and mountain `+1.0`.
- Unpainted samples have zero influence, preserving byte-for-byte deterministic automatic generation.
- Constraint data uses a versioned, validated little-endian binary representation through `serialize_terrain_constraints` and `deserialize_terrain_constraints`.
- Ocean and shallow-water tools write negative signed values and therefore carve water through the same water-depth path as automatic water. Valley and mountain tools write positive signed values and therefore create land that blends from low hill relief toward the peak-distance mountain target according to positive constraint strength.

## Pipeline

1. Generate a signed broad landmass field from fBm and a square-distance island constraint.
2. Apply controlled noise near the coastline.
3. Convert the field into a signed landmass constraint at sea level.
4. Select fixed spaced mountain peak sites, export the land-active subset, and propagate their jagged distance field.
5. Generate base elevation and the remaining inland relief influences.
6. Generate ocean depth separately.
7. Compute low-amplitude hill relief and a peak-distance mountain target.
8. Blend hills toward mountain relief by squared positive signed landmass strength.
9. Apply masked ridge, valley, and terrain-noise extensions.
10. Assemble normalized final elevation without changing land/water topology.
11. Classify boundary-connected water as ocean.
12. Compute coast distance, slope, and terrain forms.
13. Build priority drainage and condition depressions for downhill routing.
14. Accumulate contributing terrain area and export potential river channels.

This follows Mapgen4's layered elevation approach while deliberately retaining a regular-grid representation. The renderer can derive a triangulated regular-grid heightfield for 2.5D presentation without replacing or mutating canonical map data.

## Mapgen4 Alignment Boundary

Mapgen4 is the reference for the generator's layered terrain behavior, not a requirement for identical storage or rendering. The following differences are intentional:

- `GreaterRealmMap` remains a canonical regular grid. Mountain distance and drainage therefore use deterministic eight-neighbor grid traversals instead of Mapgen4's triangle graph (tasks 031 and 032).
- The engine keeps a signed landmass constraint for topology but exports final elevation normalized to `0..1`, with an independent sea-level offset and water-depth path (tasks 016, 023, and 026).
- The automatic positive-land mountain hint is adopted in the signed constraint stage. Explicit peak fields still own the mountain target shape; the hint only controls where the signed terrain is strong enough to prefer that target.
- Mountain peak sites are sampled independently from authored constraints and land/water classification. Spacing changes resample the fixed site field; jaggedness, radius, mountain strength, and other relief-only controls preserve peak identities.
- Ridge, valley, and terrain-noise layers remain engine extensions after the signed-constraint, low-hill, and peak-distance terrain composition and preserve control locality.
- Terrain forms, explicit coastline metadata, coast distance, and slope are engine data contracts beyond Mapgen4's elevation output (tasks 016 and 025).
- Drainage uses conditioned terrain and terrain-only catchment area. Generated rainfall, humidity, moisture, and current river discharge are deliberately excluded in favor of future runtime weather (tasks 027 and 039).
- The 2.5D view derives a continuous regular-grid heightfield and does not adopt Mapgen4's irregular folded render mesh (tasks 032 and 033).

Task 049 records the alignment audit. Differences not listed above require an explicit decision or remediation before they can be treated as intentional.

## Debugging And Tests

- The compile-gated `GreaterRealmDebug` module counts terrain forms and coastal land independently, converts map data into an engine-neutral RGBA image, overlays exported rivers, and marks explicit peak cells.
- The debug image uses relative ocean-depth shading and a one-cell dark coastline accent while preserving the underlying plains, hills, highlands, or mountain colour.
- Runtime base views expose terrain forms, elevation, signed landmass, hill relief, mountain relief, mountain influence, slope, coast distance, and catchment area.
- Coastlines, mountain peaks, rivers, and sampled drainage directions are independent overlays. Terrain with coastlines, peaks, and rivers enabled remains the default view.
- Changing a base view or overlay rebuilds only the RGBA image and preview texture from the retained map; it does not regenerate procedural data.
- The application can switch between the flat debug texture and a lit oblique `3D` heightfield. The heightfield reuses the active debug colours and overlays, and its elevation scale is presentation-only.
- `TextureManager` uploads the RGBA output without requiring procgen code to depend on SDL or GPU APIs.
- The application-level `GreaterRealmDebugPanel` owns the debug UI, active settings, selected constraint tool, brush settings, and regeneration callbacks; `RogueFarmGame` owns preview placement, the editable constraint field, and composition. Controls expose terrain, mountain strength, peak spacing/radius/jaggedness, potential-channel catchment threshold, a mutually exclusive Ocean/Shallow/Valley/Mountain paint-type row, brush size, and brush strength.
- Primary-button input over the visible preview maps directly to normalized constraint coordinates and paints continuously while dragged. Each paint sample carries the selected brush radius and strength into `TerrainConstraintField::paint`; these are stroke policy values and are not serialized into the constraint field. UI-consumed input and positions outside the preview cannot paint, and generated output is rebuilt at most once per frame while a stroke is active.
- The compile-gated `TerrainConstraintPainting` module owns preview-coordinate conversion and drag state without depending on SDL, UI widgets, rendering, or application classes.
- Island bias follows Mapgen4's `0..1` range and `0.5` default. It changes the signed landmass constraint, so it can affect both coastline topology and water elevation before the separate ocean-depth stage.
- Terrain noise and ocean depth use larger tuning steps and contrast-enhanced debug shading so changes are visible.
- Debug builds report per-stage generation timings plus end-to-end control-to-preview timing through the Debug-only `REALM_ENABLE_PROCGEN_PROFILING` definition; profiling code is compiled out of production builds.
- `REALM_OPTIMIZE_PROCGEN_DEBUG` defaults to `ON`, compiling the interactive procgen runtime with optimization in Debug builds while dedicated test targets retain their normal Debug checks. Disable it when stepping through procgen at instruction level is more important than interactive tuning speed.
- Automated tests cover output shape, deterministic seeds, map lookup, signed constraints, topology stability, ocean connectivity, sea-level response, terrain statistics, hill/mountain relief-stage separation, land-relief control ranges, stable fixed peak selection/spacing/distribution/dormancy/distance fields, one-stage authored-constraint composition, constraint interpolation/serialization, preview-coordinate mapping, paint interaction state, brush setting clamping/effect, drainage invariants, catchment accumulation, channel connectivity, and debug-image output.
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
