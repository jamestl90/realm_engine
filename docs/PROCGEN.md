# Procedural Generation Feature Inventory

This document tracks the procedural generation capabilities currently present in the engine.

## Greater Realm Generator

- Generates deterministic greater realm maps from a seed and settings.
- Uses a regular grid with configurable width, height, and cell size.
- The regular grid is the deliberate canonical representation following task 032's measured dual-mesh evaluation; direct indexing, compact implicit adjacency, and local-tile handoff outweigh an authoritative irregular mesh for current engine goals.
- Produces a signed landmass elevation field following Mapgen4: negative values are water, positive values are land, and zero is the fixed coastline.
- Combines unscaled five-octave land-shape noise at frequencies `1, 2, 4, 8, 16` with Mapgen4's square-distance island constraint and coastline noise sampled at fixed frequencies `16, 32, 64`, weighted `1, 1/2, 1/4`, with compact smooth support at `abs(e) < 0.20`.
- Applies Mapgen4's automatic positive-land mountain hint to the signed landmass field before authored constraints and coastline noise, so stronger inland signed constraints can locally select mountain relief without changing land/water sign.
- Keeps broad land/water topology independent from inland terrain weights.
- Derives an inspectable terrain-character value from the seed. `Seed variation` blends from exact neutral legacy scales at `0` to full deterministic character at `1`; the default is full deterministic character after task 063's representative-seed evaluation. Ruggedness scales relief amplitude, mountain coverage, secondary detail, peak spacing, and peak radius without changing land/water topology.
- Builds normalized final elevation with separate land-relief and water-depth paths around a fixed `0.5` output waterline; adjustable sea level is not a generation input.
- Shapes land by computing separate low-amplitude hill relief and peak-distance mountain relief, then blending from hills toward mountains with the squared positive signed landmass constraint multiplied by the effective mountain-coverage scale. Neutral terrain character keeps that scale at exactly `1`.
- Keeps base relief as the hill stage. Mountain strength raises only the explicit peak-distance mountain target, so changing it does not shift terrain with no peak influence. Coastline perturbation is excluded from the signed constraint used by this relief blend.
- Applies ridges, valleys, and terrain noise after the hill-to-mountain blend as secondary engine extensions, masked to positive inland constraint strength. Ridges raise relief, valleys lower relief, and terrain noise can move relief both directions without changing land/water topology.
- Selects deterministic mountain peak sites from the canonical grid before considering mutable land/water output, using configurable Poisson-style spacing with a center-biased priority to avoid bucket-aligned artifacts.
- Uses fixed sites with a positive unperturbed/authored relief constraint as stable mountain-distance sources. It exports visible peak records only for source sites that remain land after coastline perturbation; authored negative constraints can make sources dormant, while coastline detail cannot reshape the global mountain field.
- Exports peak records plus per-cell peak distance, influence, and peak flags for hydrology and tooling.
- Exports per-cell hill and mountain relief stages for debugging and tests.
- Marks boundary-connected water as ocean and classifies enclosed water separately as inland water.
- Computes distance to coast, explicit coastal-land boundary metadata, and local slope.
- Classifies water as ocean or inland water and land as plains, hills, highlands, or mountains, including land directly beside water.
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
3. Convert the field into a signed landmass constraint with fixed coastline threshold `0`.
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

## Dependency-Aware Regeneration

`GreaterRealmGenerationCache` retains the sampled terrain layers and the last applied settings. Each change is mapped to its earliest dirty stage, then expanded through only the required downstream dependencies:

| Change | Earliest rebuilt stage | Reused work |
|---|---|---|
| Seed, dimensions, cell size, broad landmass/coast controls, terrain frequencies, legacy sea-level setting, authored constraints, or forced regeneration | Terrain fields | None |
| Peak spacing, radius, or jaggedness | Mountain peaks | Terrain fields |
| Base, mountain, ridge, valley, terrain-noise, or ocean-depth strength | Relief | Terrain fields and mountain peaks |
| Terrain-form thresholds | Classification | Terrain fields, peaks, relief, geography metadata, and drainage |
| Explicit drainage invalidation | Drainage | Terrain, relief, and classification |
| River catchment threshold or width scale | River channels | Terrain, classification, and conditioned drainage |
| Debug view or overlay | Debug image | All generated map data |

Any generated-map change also dirties the debug image and texture upload. Authored-constraint owners call `invalidate(TerrainFields)` after mutation; repeated paint samples coalesce into the existing once-per-frame regeneration path. The cache reports rebuilt-stage flags and, when `REALM_ENABLE_PROCGEN_PROFILING` is enabled, per-stage timings. Tests compare partial output byte-for-byte at the data-field level against clean generation and exercise representative full, relief-only, and channel-only paths at `256x192`.

## Mapgen4 Alignment Boundary

Mapgen4 is the reference for the generator's layered terrain behavior, not a requirement for identical storage or rendering. The following differences are intentional:

- `GreaterRealmMap` remains a canonical regular grid. Mountain distance and drainage therefore use deterministic eight-neighbor grid traversals instead of Mapgen4's triangle graph (tasks 031 and 032).
- The engine keeps Mapgen4's fixed signed coastline threshold while exporting final elevation normalized to `0..1`, with water below and land above a fixed `0.5` output waterline.
- The automatic positive-land mountain hint is adopted in the signed constraint stage. Explicit peak fields still own the mountain target shape; the hint only controls where the signed terrain is strong enough to prefer that target.
- Mountain peak sites are sampled independently from authored constraints and land/water classification. Spacing changes resample the fixed site field; jaggedness, radius, mountain strength, and other relief-only controls preserve peak identities.
- Coastline noise retains Mapgen4's fixed `16, 32, 64` spectrum, `1, 1/2, 1/4` weights, `0.01` default, and `0..0.1` range. It intentionally replaces Mapgen4's acknowledged over-broad `1 - e^4` attenuation with compact smooth support at `abs(e) < 0.20`, and the unperturbed broad constraint owns inland relief selection (task 058).
- Seed-driven terrain character is an engine extension beyond Mapgen4. Existing relief and peak controls remain explicit base values, effective ruggedness is exported on the generated map, and setting seed variation to zero restores scale factors of exactly one. Representative flat, ordinary, and rugged seed evaluation retained full variation as the generator default because it adds useful realm-scale character without changing land/water topology or committing gameplay to 2.5D terrain (tasks 060 and 063).
- Ridge, valley, and terrain-noise layers remain engine extensions after the signed-constraint, low-hill, and peak-distance terrain composition and preserve control locality.
- Terrain forms, explicit coastline metadata, coast distance, and slope are engine data contracts beyond Mapgen4's elevation output (tasks 016 and 025).
- Drainage uses conditioned terrain and terrain-only catchment area. Generated rainfall, humidity, moisture, and current river discharge are deliberately excluded in favor of future runtime weather (tasks 027 and 039).
- The 2.5D view derives a continuous regular-grid heightfield and does not adopt Mapgen4's irregular folded render mesh (tasks 032 and 033).

Task 049 records the alignment audit. Differences not listed above require an explicit decision or remediation before they can be treated as intentional.

## Debugging And Tests

- The compile-gated `GreaterRealmDebug` module counts terrain forms and coastal land independently, converts map data into an engine-neutral RGBA image, overlays exported rivers, and marks explicit peak cells.
- The default terrain view maps normalized land elevation through a continuous nonlinear lowland-to-summit colour ramp with fixed anchors at `0.50, 0.54, 0.59, 0.65, 0.75, 0.86, 1.00`. Closely spaced lowland and hill anchors emphasize the range occupied by most generated terrain while fixed rock and summit anchors preserve cross-map height meaning. A restrained terrain-form tint remains secondary; this is geography visualization only and does not assign biomes or consume runtime weather fields.
- The debug image preserves relative water-depth shading, distinguishes ocean from inland water, and retains a one-cell dark coastline accent. A separate `Terrain forms` base view retains the categorical water and land-form palette.
- Runtime base views expose terrain forms, elevation, signed landmass, hill relief, mountain relief, mountain influence, slope, coast distance, and catchment area.
- Coastlines, mountain peaks, rivers, and sampled drainage directions are independent overlays. Terrain with coastlines, peaks, and rivers enabled remains the default view.
- Changing a base view or overlay rebuilds only the RGBA image and preview texture from the retained map; it does not regenerate procedural data.
- The application can switch between the flat debug texture and a lit oblique `3D` heightfield. The heightfield reuses the active debug colours and overlays, and its elevation scale is presentation-only.
- `TextureManager` uploads the RGBA output without requiring procgen code to depend on SDL or GPU APIs. Same-sized debug images update the existing texture and preserve its `TextureID` without a global GPU-idle wait; only dimension changes allocate and swap a texture.
- The application-level `GreaterRealmDebugPanel` owns the debug UI, active settings, selected constraint tool, brush settings, and regeneration callbacks; `TestApp` owns preview placement, the editable constraint field, and composition. Controls expose island bias, seed variation with effective ruggedness summary, coastline detail, land relief controls, peak spacing/radius/jaggedness, ocean depth, potential-channel catchment threshold, a mutually exclusive Ocean/Shallow/Valley/Mountain paint-type row, brush size, and brush strength. Seed variation remains a normal debug tuning control; `0` is the exact neutral comparison mode and `1` is the default generator contract.
- Primary-button input over the visible preview maps directly to normalized constraint coordinates and paints continuously while dragged. Each paint sample carries the selected brush radius and strength into `TerrainConstraintField::paint`; these are stroke policy values and are not serialized into the constraint field. UI-consumed input and positions outside the preview cannot paint, and generated output is rebuilt at most once per frame while a stroke is active.
- The compile-gated `TerrainConstraintPainting` module owns preview-coordinate conversion and drag state without depending on SDL, UI widgets, rendering, or application classes.
- Island bias follows Mapgen4's `0..1` range and `0.5` default. It changes the signed landmass constraint, so it can affect both coastline topology and water elevation before the separate ocean-depth stage.
- Terrain noise and ocean depth use larger tuning steps and contrast-enhanced debug shading so changes are visible.
- Debug builds report per-stage generation timings plus end-to-end control-to-preview timing through the Debug-only `REALM_ENABLE_PROCGEN_PROFILING` definition; profiling code is compiled out of production builds.
- `REALM_OPTIMIZE_PROCGEN_DEBUG` defaults to `ON`, compiling the interactive procgen runtime with optimization in Debug builds while dedicated test targets retain their normal Debug checks. Disable it when stepping through procgen at instruction level is more important than interactive tuning speed.
- Automated tests cover output shape, deterministic seeds, map lookup, signed constraints, topology stability, ocean connectivity, inland-water classification, sea-level invariance, Mapgen4 coastline attenuation, terrain statistics, hill/mountain relief-stage separation, land-relief control ranges, stable fixed peak selection/spacing/distribution/dormancy/distance fields, one-stage authored-constraint composition, constraint interpolation/serialization, preview-coordinate mapping, paint interaction state, brush setting clamping/effect, drainage invariants, catchment accumulation, channel connectivity, staged-regeneration equivalence and timing paths, and debug-image output.
- Test code is compiled only when `REALM_BUILD_TESTS=ON` and does not enter release builds.

## Not Yet Supported

- Lake retention, shared water-surface levels, river erosion, deltas, or watershed metadata. Enclosed water is classified as inland water without implying those hydrological behaviors.
- Runtime weather, precipitation, runoff, soil moisture, or active river discharge; task 040 tracks this future simulation layer.
- Resources, settlements, factions, or object placement.
- Local tile generation or world-region streaming.
- Beach, cliff, rocky-shore, marsh, delta, or other detailed shoreline classification.
- A derived Delaunay/Voronoi render surface; task 032 rejected it as the canonical greater-realm representation.
- Mapgen4's irregular folded mesh around coasts, ridges, valleys, and rivers. The supported 2.5D path is a continuous triangulated regular-grid heightfield instead.
