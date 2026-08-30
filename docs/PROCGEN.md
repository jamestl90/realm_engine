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
- Builds a deterministic priority drainage topology with a downslope target or outlet for every cell. Ocean and inland-water cells are drainage terminals.
- Exports a hydrologically conditioned elevation so drainage remains acyclic and downhill through depressions without changing visual terrain elevation.
- Accumulates terrain-only contributing area downstream and exports renderer-independent potential river channels with catchment area and a derived display width. Channels may terminate near the coast or at inland water. Coastal land cells are not exported as river segment endpoints except for the final adjacent approach into an inland-water terminal, and near-coast exported segments must move toward the coastline rather than along it.
- Does not generate current rainfall, humidity, soil moisture, runoff, or current river discharge. Those values belong to future world simulation only if gameplay needs them.

## Climate And Biome Ownership

- Stable greater-realm climate belongs to procgen as a derived layer, separate from canonical terrain and hydrology.
- Climate output belongs in a versioned `GreaterRealmClimateMap` with one `GreaterRealmClimateCell` per greater-realm cell. Source seed, dimensions, cell size, and a fingerprint of consumed terrain elevation/water data reject stale pairings.
- The implemented `temperature_normal` and `precipitation_normal` use a fixed `0..1` scale whose meaning is retained across maps; observed per-map ranges are never renormalized.
- Temperature normal consumes explicit latitude context, elevation, maritime moderation from ocean or inland-water proximity, and optional broad deterministic variation.
- Precipitation normal is a long-term climatological tendency derived from ocean and inland-water sources, latitude-dependent circulation, elevation, orographic lift, and carried rain shadow without representing a specific rain event.
- Humidity, soil moisture, current rainfall, runoff, and active river discharge are not climate-normal aliases and are not generated procgen fields.
- Applications own biome definitions and IDs. The reusable classifier consumes application-supplied rules and produces a separate `GreaterRealmBiomeMap`; it does not add hard-coded biome labels to `GreaterRealmCell`.
- Biome rules may inspect stable terrain and climate inputs. Applications separately own biome names, colours, art, resources, and gameplay behavior.
- Generated climate and biome arrays are regenerated from versioned inputs by default. Terrain changes invalidate climate and biome output, climate changes invalidate climate and biome output, and biome-rule changes invalidate biome output only.
- Tasks 074-076 implement temperature normals, precipitation normals, and application-driven biome classification without requiring local-region generation or runtime weather.

## Climate And Seasons

Greater-realm climate is intentionally split into stable annual normals and deterministic seasonal evaluation with separate owners, clocks, invalidation rules, and persistence behavior.

| Layer | Owner | Core fields | Resolution | Cadence | Persistence |
|---|---|---|---|---|---|
| Annual climate normals | Procgen derived data | `temperature_normal`, `precipitation_normal`, source terrain identity, climate data/settings versions, effective climatological transport and aridity character | One climate cell per canonical greater-realm terrain cell | Rebuilt only when terrain source identity or climate-normal settings change | Regenerated from versioned inputs by default |
| Seasonal climate evaluation | World simulation/application climate layer | Temperature amplitude/phase, precipitation amplitude/phase, optional profile IDs and seasonal modifiers sampled from annual normals and deterministic calendar input | Implemented seasonal maps use one sample per greater-realm cell; later layers may cache or aggregate by region | Calendar-scale tick or on-demand deterministic evaluation, never render-frame driven | Derived/cacheable; profile/settings are persisted, samples usually are not |

- Annual normals describe what a place is generally like across long time spans. They are the only climate values biome generation may inspect.
- Seasonal evaluation describes predictable calendar-scale departures from annual normals, such as winter cooling or wet seasons. It consumes explicit deterministic time inputs such as normalized year fraction or calendar tick, a world/profile seed, stable coordinates, and seasonal profile versions.
- Task 077's circulation settings are climatological moisture-transport settings used to derive annual `precipitation_normal`. They are not persistent runtime wind, pressure gradients, storm tracks, or current airflow. A later compatibility task may rename or alias public settings, but current behavior and serialized climate identity must remain stable until such a task exists.
- Seasonal temperature is annual temperature plus a deterministic seasonal offset. This lets a warm region have cool seasons without losing its long-term climate identity.
- Seasonal precipitation is annual precipitation multiplied by a deterministic wet/dry seasonal tendency. It is not current rainfall and cannot rewrite annual precipitation normal or terrain-only drainage data.
- Biome assignment depends only on stable terrain and long-term climate normals. Seasonal offsets and future gameplay overlays must not regenerate or relabel `GreaterRealmBiomeMap`.
- Annual climate invalidates from terrain or climate-normal setting/version changes. Seasonal caches invalidate from annual-normal identity, seasonal profile/version changes, and deterministic calendar inputs.
- Terrain-only drainage, conditioned elevation, catchment area, and potential channel geometry remain stable procgen/hydrology data. Future runoff or discharge, if added for gameplay, may route over that topology but cannot feed back into generated terrain, climate normals, or current river debug geometry.

### Temperature Normals

- `GreaterRealmClimateSettings` defaults the north and south map edges to `+60` and `-60` degrees latitude. Each row linearly interpolates between those edges after both values are clamped to `-90..+90`; a one-row map samples their midpoint.
- The fixed latitude anchor is `1 - abs(latitude) / 90`, so the equator maps to `1` and either pole maps to `0` before other influences.
- Normalized land height above the fixed `0.5` waterline cools temperature with a default weight of `0.35`. Water has a defined temperature but does not use depth as elevation cooling.
- Eight-neighbor distance to any ocean or inland-water cell moderates temperature toward the fixed midpoint `0.5`. The default moderation weight is `0.20` with a smooth `16` map-unit influence distance.
- A three-octave, climate-domain noise field adds optional broad seed variation. Its default amplitude is `0.08` and base frequency is `2.5`; setting amplitude to `0` removes all seed influence from otherwise identical terrain.
- The composed result is clamped to `0..1` only. No minimum/maximum scan feeds back into generation.
- `GreaterRealmClimateGenerationCache` rebuilds each normal independently for its own setting changes and rebuilds both when explicitly invalidated by a terrain owner or when the source identity/fingerprint no longer matches. Climate generation accepts terrain as const and cannot rebuild terrain, hydrology, or river channels.

### Seasonal Temperature Evaluation

- `world::SeasonalTemperatureSettings` owns deterministic seasonal-temperature settings outside procgen. Defaults use the same `+60` north edge and `-60` south edge latitude context as greater-realm climate normals, a base normalized seasonal amplitude of `0.10`, added latitude amplitude of `0.16`, added elevation amplitude of `0.04`, and maritime damping of `0.35` over `16` map units.
- Northern and southern peak phases default to normalized year fractions `0.50` and `0.00`, giving opposite hemisphere seasons. Year fractions are explicit inputs, wrap to `0..1`, and are quantized for repeatable cache identity; evaluation never reads render frame time or engine wall-clock time.
- Optional profile-driven regional phase and amplitude variation consumes `profile_seed`, `profile_identity`, and map coordinates. Both regional variation strengths default to `0`, so the default seasonal wave is smooth and exactly profile-neutral.
- `SeasonalTemperatureMap` stores version `1`, source terrain identity, an annual-temperature fingerprint, a seasonal-settings fingerprint, normalized year fraction, and one `SeasonalTemperatureCell` per greater-realm cell. Each cell stores the source `annual_temperature_normal`, the signed seasonal offset, and the clamped composed `seasonal_temperature_normal`.
- The seasonal cache rebuilds when terrain identity, annual temperature normals, seasonal settings/profile identity, or normalized year fraction changes. It is a derived/cacheable world-simulation layer, not an extension of `GreaterRealmClimateGenerationCache`.
- Seasonal temperature evaluation may change experienced conditions, but it does not alter terrain, annual climate normals, precipitation normals, hydrology, debug image source data, or biome assignments. Biome generation continues to inspect only terrain and annual climate normals.

### Seasonal Precipitation Evaluation

- `world::SeasonalPrecipitationSettings` owns deterministic wet/dry seasonal settings outside procgen. Defaults use `+60` north edge and `-60` south edge latitude context, base multiplier amplitude `0.25`, latitude amplitude `0.15`, inland/elevation damping `0.10`, northern wet peak year fraction `0.00`, southern wet peak year fraction `0.50`, and multiplier bounds `0.25..1.75`.
- Seasonal precipitation consumes annual `precipitation_normal`, terrain coordinates, explicit normalized year fraction, and profile identity. It never reads frame time and does not use Task 077 climatological transport as runtime wind.
- `SeasonalPrecipitationMap` stores version `1`, source terrain identity, an annual-precipitation fingerprint, a seasonal-settings fingerprint, normalized year fraction, and one `SeasonalPrecipitationCell` per greater-realm cell. Each cell stores annual precipitation, seasonal multiplier, and clamped composed seasonal precipitation tendency.
- The seasonal precipitation cache rebuilds only when terrain identity, annual precipitation normals, seasonal settings/profile identity, or normalized year fraction changes. It does not dirty terrain, annual climate normals, potential river channels, debug source data, or biome assignment.
- Seasonal precipitation is a deterministic wet/dry tendency. It is not current rain, humidity, runoff, soil moisture, or river discharge.

### Precipitation Normals

- The latitude baseline uses opposing diagonal tropical and mid-latitude transport in each hemisphere. Hemisphere direction blends smoothly across `-8..+8` degrees latitude and tropical-to-mid-latitude direction across `25..40` degrees absolute latitude. Its default strength is `0.35`, retaining latitude structure without imposing the same circulation silhouette on every realm.
- A domain-separated seed wind character rotates the complete pattern through the full compass, shifts wind-band latitude by up to `8` degrees, varies each hemisphere by up to `20` degrees, and scales a broad regional direction field. `prevailing_wind_degrees` remains an authored rotation added after that character.
- Regional variation uses an eight-direction transport atlas at `45`-degree intervals. Every atlas pass performs full deterministic moisture transport, lift, and rain shadow; cells smoothly sample neighboring passes from a low-frequency direction field instead of adding precipitation noise. A weaker opposing component defaults to `0.20` so the normal is not equivalent to one permanent weather event.
- `wind_seed_variation` defaults to `1`. Setting it to `0` exactly restores the seed-neutral latitude-band baseline; intermediate values blend continuously between baseline and varied output. Regional strength defaults to `0.45` and frequency to `1.5` broad cycles across the realm.
- Ambient moisture defaults to `0.18`. Ocean cells replenish air moisture to at least `1.0`; inland water replenishes it to at least `0.65`, preserving their distinct stable source influence.
- Moisture retention defaults to `0.985` per map unit and background precipitation efficiency to `0.35`.
- Rising normalized land elevation condenses additional precipitation with lift strength `1.50`. Lift consumes transported moisture and creates a downwind shadow with strength `0.70` and per-map-unit retention `0.92`.
- A global precipitation scale defaults to `1.0`. Domain-separated seed character additionally varies effective precipitation scale from `0.70..1.75` and transport-loss severity from dry to humid realms; `precipitation_seed_variation` blends that character toward exact neutral values and `0` removes all seed influence. The final value is clamped to `0..1` without observed-range normalization.
- The transport pass reads only cell elevation and stable water classification. Drainage area, channel thresholds, rivers, current rainfall, humidity, soil moisture, runoff, and discharge cannot affect it.

### Application-Driven Biomes

- `GreaterRealmBiomeRuleSet` contains ordered rules with unique opaque `BiomeId` values, explicit integer priority, optional terrain form, water class, elevation, slope, coast-distance, temperature, and precipitation constraints, plus an optional fallback ID.
- All range boundaries are inclusive. Higher priority wins; equal-priority ties select the first declared rule. A valid unmatched cell receives the fallback ID or `INVALID_BIOME_ID` when no fallback exists. Assignment never depends on associative-container iteration order.
- Validation rejects reserved or duplicate rule IDs, unknown enum values, non-finite bounds, inverted ranges, normalized bounds outside `0..1`, and negative slope or coast-distance bounds.
- `GreaterRealmBiomeMap` stores only opaque IDs plus fingerprints for its terrain, climate, and complete ordered rule-set sources. Any source change rebuilds assignment without regenerating terrain or climate.
- Debug rendering accepts an application-owned ID-to-colour table. IDs absent from the table use a deterministic neutral fallback palette; biome names, art, resources, spawning, and gameplay behavior never enter procgen data.
- `TestApp` supplies a small in-memory inspection set covering ocean, inland water, alpine, polar, rainforest, desert, forest, tundra, and grassland IDs. Those definitions and colours are sandbox application data, not engine-standard biomes or an asset format. A representative-map regression check requires at least four visible land classes and prevents any one class from covering `70%` of land; a `256x192` seed sweep additionally requires both limited-desert and heavily desert realms.

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
13. Build priority drainage, treating ocean and inland-water cells as terminals, and condition depressions for downhill routing.
14. Accumulate contributing terrain area and export potential river channels.
15. Derive separate temperature and precipitation climate normals from finalized terrain, latitude, and transport settings.
16. Optionally evaluate application-supplied biome rules into a separate biome map.

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

`GreaterRealmClimateGenerationCache` is a separate derived-map cache. Changes to terrain fields, peaks, relief, or classification invalidate both normals; temperature-setting changes rebuild only temperature; precipitation, circulation, or seed-character setting changes rebuild only precipitation. `GreaterRealmBiomeGenerationCache` rebuilds assignment when terrain, either climate field, stored climate character, or ordered rule data changes. Debug base-view and overlay changes rebuild only the retained RGBA visualization and texture.

Seasonal climate evaluation is a separate cache, not an extension of `GreaterRealmClimateGenerationCache`. Seasonal cache invalidation may rebuild seasonal samples and gameplay-facing climate queries, but it does not dirty terrain, annual climate normals, or biome assignment.

Seasonal temperature and precipitation maps use version `2`. Their northern and southern phase waves remain opposite outside the tropics and blend with a smoothstep handoff across the configured equatorial transition half-width, which defaults to `15` degrees. This prevents adjacent cells around zero latitude from receiving abruptly opposite seasonal values. The transition setting participates in seasonal settings fingerprints without affecting annual climate or biome identity.

`ClimateWeatherSample` is the gameplay-facing query contract for this foundation. A query can expose stable annual normals, seasonal offsets/multipliers, and the stable biome ID. Missing seasonal data falls back to annual normals.

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
- Drainage uses conditioned terrain and terrain-only catchment area. Inland water can terminate incoming potential river channels, but hydrology does not synthesize lake outlets, through-lake channels, or downstream continuations. Generated rainfall, humidity, moisture, and current river discharge are deliberately excluded (tasks 027, 039, and 084).
- The 2.5D view derives a continuous regular-grid heightfield and does not adopt Mapgen4's irregular folded render mesh (tasks 032 and 033).

Task 049 records the alignment audit. Differences not listed above require an explicit decision or remediation before they can be treated as intentional.

## Debugging And Tests

- The build-configurable `GreaterRealmDebug` inspection module is enabled by the shipped Debug and Release presets. It counts terrain forms and coastal land independently, converts map data into an engine-neutral RGBA image, overlays exported rivers, and marks explicit peak cells.
- The default terrain view maps normalized land elevation through a continuous nonlinear lowland-to-summit colour ramp with fixed anchors at `0.50, 0.54, 0.59, 0.65, 0.75, 0.86, 1.00`. Closely spaced lowland and hill anchors emphasize the range occupied by most generated terrain while fixed rock and summit anchors preserve cross-map height meaning. A restrained terrain-form tint remains secondary; this is geography visualization only and does not assign biomes or consume mutable world-simulation fields.
- The debug image preserves relative water-depth shading, distinguishes ocean from inland water, and retains a one-cell dark coastline accent. A separate `Terrain forms` base view retains the categorical water and land-form palette.
- The unified application inspection selector exposes terrain forms, elevation, signed landmass, hill relief, mountain relief, mountain influence, slope, coast distance, catchment area, annual temperature, annual precipitation, biome assignment, seasonal temperature, and seasonal precipitation. Fixed climate palettes and application-supplied biome colours are shared by flat and `3D` previews; the panel reports annual climate means and observed ranges without using those statistics to alter generation.
- Coastlines, mountain peaks, rivers, and sampled drainage directions are independent overlays. Terrain with coastlines, peaks, and rivers enabled remains the default view.
- Changing a base view or overlay rebuilds only the RGBA image and preview texture from the retained map; it does not regenerate procedural data.
- The application can switch between the flat debug texture and a lit oblique `3D` heightfield. The heightfield reuses the active debug colours and overlays, and its elevation scale is presentation-only.
- `game::GreaterRealmClimateWeatherInspection` composes seasonal climate colours outside procgen, then reuses procgen's geographic overlays.
- The inspector owns an explicit normalized year-fraction slider that never advances from frame time. Year changes recompute seasonal samples only and do not invoke terrain, annual-climate, hydrology, or biome regeneration.
- `TextureManager` uploads the RGBA output without requiring procgen code to depend on SDL or GPU APIs. Same-sized debug images update the existing texture and preserve its `TextureID` without a global GPU-idle wait; only dimension changes allocate and swap a texture.
- The application-level `GreaterRealmDebugPanel` owns the debug UI, active settings, selected constraint tool, brush settings, and regeneration callbacks; `TestApp` owns preview placement, the editable constraint field, and composition. Controls expose island bias, seed variation with effective ruggedness summary, coastline detail, land relief controls, peak spacing/radius/jaggedness, ocean depth, potential-channel catchment threshold, a mutually exclusive Ocean/Shallow/Valley/Mountain paint-type row, brush size, and brush strength. Seed variation remains a normal debug tuning control; `0` is the exact neutral comparison mode and `1` is the default generator contract.
- Primary-button input over the visible preview maps directly to normalized constraint coordinates and paints continuously while dragged. Each paint sample carries the selected brush radius and strength into `TerrainConstraintField::paint`; these are stroke policy values and are not serialized into the constraint field. UI-consumed input and positions outside the preview cannot paint, and generated output is rebuilt at most once per frame while a stroke is active.
- The build-configurable `TerrainConstraintPainting` module owns preview-coordinate conversion and drag state without depending on SDL, UI widgets, rendering, or application classes; shipped Debug and Release presets include it with the greater-realm inspection UI.
- Island bias follows Mapgen4's `0..1` range and `0.5` default. It changes the signed landmass constraint, so it can affect both coastline topology and water elevation before the separate ocean-depth stage.
- Terrain noise and ocean depth use larger tuning steps and contrast-enhanced debug shading so changes are visible.
- Debug builds report per-stage generation timings plus end-to-end control-to-preview timing through the Debug-only `REALM_ENABLE_PROCGEN_PROFILING` definition; profiling code is compiled out of production builds.
- `REALM_OPTIMIZE_PROCGEN_DEBUG` defaults to `ON`, compiling the interactive procgen runtime with optimization in Debug builds while dedicated test targets retain their normal Debug checks. Disable it when stepping through procgen at instruction level is more important than interactive tuning speed.
- Automated tests cover output shape, deterministic seeds, map lookup, signed constraints, topology stability, ocean connectivity, inland-water classification, painted-lake river terminals, sea-level invariance, Mapgen4 coastline attenuation, terrain statistics, hill/mountain relief-stage separation, land-relief control ranges, stable fixed peak selection/spacing/distribution/dormancy/distance fields, one-stage authored-constraint composition, constraint interpolation/serialization, preview-coordinate mapping, paint interaction state, brush setting clamping/effect, drainage invariants, catchment accumulation, channel connectivity, staged-regeneration equivalence and timing paths, climate shape/range/determinism/latitude circulation/seed aridity/responses/invalidation, biome validation/precedence/identity/invalidation/distribution, and debug-image output.
- Test code is compiled only when `REALM_BUILD_TESTS=ON` and does not enter release builds.

## Not Yet Supported

- Lake retention, shared water-surface levels, automatic lake outlets, through-lake channels, river erosion, deltas, or watershed metadata. Enclosed water is classified as inland water and can terminate incoming potential river channels without implying those additional hydrological behaviors.
- Runtime runoff, soil moisture, snowpack, active river discharge, flooding, erosion, weather fronts, storm entities, long-duration drought events, gameplay weather rendering, particles, or audio.
- Product biome catalogues, biome asset formats, names, art, resource tables, spawn rules, or gameplay effects. The engine implementation intentionally stops at opaque application IDs and in-memory rules.
- Resources, settlements, factions, or object placement.
- Local tile generation or world-region streaming; queued Tasks 067-073 retain that future roadmap while current procgen work remains at greater-realm scale.
- Beach, cliff, rocky-shore, marsh, delta, or other detailed shoreline classification.
- A derived Delaunay/Voronoi render surface; task 032 rejected it as the canonical greater-realm representation.
- Mapgen4's irregular folded mesh around coasts, ridges, valleys, and rivers. The supported 2.5D path is a continuous triangulated regular-grid heightfield instead.
