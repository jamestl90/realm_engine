# Define Climate, Season, And Weather Layers

Status: complete
Priority: high
Area: Procgen / World Simulation Architecture

## Goal

Define a clean ownership and data-flow boundary between stable biome-driving climate normals, predictable seasonal climate, and changing runtime weather without altering the current greater-realm biome output.

## Context

Tasks 074-077 established stable temperature and precipitation normals for greater-realm biome generation. Their circulation is climatological moisture-transport machinery, not literal wind that remains fixed during play.

The intended world should support generally warm or cool regions while still allowing seasons, heatwaves, cold snaps, storms, drought, and other temporary conditions. Stable climate must continue to describe what a place is usually like; runtime weather must describe what is happening there now.

## Dependencies

- Task 066: climate and biome ownership boundary.
- Tasks 074-077: stable temperature, precipitation, biome, circulation, and aridity generation.
- Task 040: parked runtime weather and runoff proposal, whose eventual implementation should consume this contract.

## Acceptance Criteria

- Define three distinct layers: annual climate normals, seasonal climate evaluation, and transient runtime weather.
- Specify the fields owned by each layer, including annual temperature and precipitation, seasonal amplitude and phase, current temperature anomaly, pressure, wind, humidity, cloud, and active precipitation where appropriate.
- Define the composition of experienced temperature and precipitation from stable, seasonal, transient, and local influences.
- Keep biome assignment dependent only on stable long-term climate and terrain inputs; seasons and short-term weather must not regenerate or relabel biomes.
- Reframe procgen circulation as climatological moisture transport rather than persistent runtime wind, documenting whether public naming should change in a later compatibility task.
- Define ownership, versioning, invalidation, persistence, spatial resolution, and update cadence for every layer.
- Define deterministic seed and time inputs so seasonal and weather behavior can be reproduced without coupling it to frame rate.
- Identify boundaries with hydrology: active rainfall may feed future runoff and discharge, but must not rewrite terrain-only drainage topology or annual precipitation normals.
- Describe how warm regions can experience seasonal or transient cold and cool regions can experience heatwaves without destroying their baseline climate identity.
- Produce an ordered implementation breakdown for seasonal temperature, seasonal precipitation, runtime atmospheric state, weather evolution, and gameplay-facing queries.
- Preserve current greater-realm generation and biome output behavior exactly; this task introduces no weather simulation or biome retuning.
- Update the canonical architecture/procgen documentation and verify it remains consistent with Task 040 and the existing climate contracts.

## Out Of Scope

- Implementing a world calendar or seasons.
- Simulating pressure systems, fronts, clouds, wind, rainfall, storms, drought, or temperature anomalies.
- Runtime runoff, river discharge, flooding, erosion, or terrain mutation.
- Dynamic biome transitions or biome regeneration from weather.
- Local-region generation, rendering, particles, audio, or weather presentation.

## Notes

This is an architecture and task-breakdown change. A dedicated branch is not required unless implementation expands into runtime data structures or simulation code.

## Decisions

- Annual climate normals remain the only climate layer owned by greater-realm procgen today. `GreaterRealmClimateMap` continues to store one cell per canonical terrain cell with fixed-scale `temperature_normal` and `precipitation_normal` values, source terrain identity, climate data version, and climate-generation settings/character identity. No current-weather, season, humidity, soil-moisture, runoff, or discharge fields are added to terrain, climate, or biome output.
- Task 077's wind and aridity settings describe climatological moisture transport used to produce `precipitation_normal`. They are not persistent runtime wind, pressure, storm motion, or per-frame airflow. Public names that mention wind can remain for compatibility until a focused naming task can add aliases or migration notes, but documentation must call them climatological transport settings.
- Biome assignment remains a stable derived map based only on terrain, annual temperature normal, annual precipitation normal, and application-owned biome rules. Seasonal offsets and runtime weather must never regenerate, relabel, or mutate `GreaterRealmBiomeMap`; applications that want temporary visual or gameplay overlays must represent them outside biome identity.
- Seasonal climate evaluation is a future world-simulation/application-facing layer that consumes stable normals plus deterministic calendar inputs. It may define per-cell or per-region seasonal temperature amplitude, temperature phase, precipitation amplitude, precipitation phase, snow/rain tendency thresholds, and optional authored profile IDs. It does not own annual normals and does not change generated terrain, hydrology, climate normals, or biome assignments.
- Transient runtime weather is future world-simulation state. It owns current temperature anomaly, pressure, runtime wind vector, humidity, cloud cover, active precipitation intensity/type, storm/drought state, and any later weather-front or event metadata. It may be persisted with active/streamed regions and advanced on a fixed simulation cadence, but it must not rewrite annual climate normals, terrain-only drainage topology, potential channel geometry, or biome IDs.
- Experienced temperature is composed by converting the stable annual `temperature_normal` through application/world calibration, then adding seasonal offset, transient weather anomaly, and local runtime modifiers such as elevation exposure, shelter, water proximity, or urban/gameplay effects where those systems exist. Warm regions can therefore experience cold seasons or cold snaps while retaining their warm long-term climate identity.
- Experienced precipitation is composed from stable `precipitation_normal` as a long-term probability/intensity tendency, seasonal wet/dry modulation, transient active precipitation, and local runtime modifiers. Active rainfall can feed future runoff, soil moisture, snowpack, flooding, and discharge, but those values remain runtime hydrology/weather state and cannot feed back into annual precipitation normals.
- Spatial resolution is layer-specific. Annual climate normals stay one-to-one with greater-realm cells. Seasonal evaluation may initially sample at the same resolution for determinism and debugging, then cache or aggregate by region if profiling requires it. Runtime weather should use explicit weather cells or region-scoped state, typically coarser than terrain cells, with interpolation/query helpers for gameplay.
- Update cadence is layer-specific. Annual normals update only when source terrain or climate settings change. Seasonal climate evaluates from deterministic world time at calendar-scale ticks or on demand and must not be tied to render frames. Runtime weather advances on a fixed simulation time step or explicit event schedule independent from frame rate.
- Versioning and invalidation stay explicit. Annual climate uses existing climate-map data and algorithm versions plus source fingerprints. Seasonal evaluation must include a seasonal-algorithm/profile version and invalidate cached seasonal samples when normals, profile settings, or deterministic calendar inputs change. Runtime weather persistence must include a weather-algorithm version, world/weather seed identity, region identity, simulation timestamp, and schema version; incompatible saved state is discarded or migrated without regenerating procgen layers.
- Persistence is not uniform. Annual climate and biome arrays are currently regenerated from versioned inputs by default. Seasonal samples are derived/cacheable and usually need not be saved. Runtime weather and runoff/discharge, once implemented, are mutable simulation state and should be persisted for active or modified streamed regions according to the future region-persistence contract.
- Determinism requires explicit time and seed inputs. Seasonal evaluation consumes world seed or authored seasonal profile seed, normalized year fraction or absolute calendar tick, and stable map coordinates. Runtime weather consumes a weather seed/domain, fixed simulation tick index or absolute simulation timestamp, region/weather-cell identity, and prior persisted state when continuity is required.
- Hydrology boundaries remain unchanged. Terrain-only drainage and catchment topology describe where water could flow. Active rainfall, snowmelt, runoff, soil moisture, and river discharge may later use that topology, but must not alter conditioned elevation, catchment area, potential channel identity, or current greater-realm river debug output.

## Breakdown

- Task 079 adds deterministic seasonal temperature evaluation over annual temperature normals.
- Task 080 adds deterministic seasonal precipitation evaluation over annual precipitation normals.
- Task 081 defines runtime atmospheric state, persistence identity, and query resolution without evolving weather yet.
- Task 082 implements deterministic weather evolution from explicit seeds, time steps, seasonal tendencies, and atmospheric state.
- Task 083 adds gameplay-facing climate/weather query helpers so consumers ask for stable climate, seasonal tendency, experienced conditions, or active precipitation without reaching through ownership boundaries.

Task 040 remains the parked runoff/discharge idea. It now depends on this layer contract and the follow-up weather tasks, but its runoff, soil moisture, river discharge, flooding, and erosion-adjacent scope is not implemented or deleted by Task 078.

## Verification

- Inspected `git status --short` and `git status --short --untracked-files=all` before editing; Git reported only the existing permission warning for `C:\Users\James/.config/git/ignore` and no worktree changes.
- Read the requested architecture, procgen, task workflow, Task 078, Task 066, Tasks 074-077, and Task 040 documents. `docs/PROJECT_BRIEF.md` was requested but does not exist in the repository; `rg --files docs` found no project brief under `docs`.
- Updated `docs/ARCHITECTURE.md` and `docs/PROCGEN.md` with the annual-normal, seasonal-evaluation, and transient-weather ownership boundary, including fields, versioning, invalidation, persistence, deterministic seed/time inputs, spatial resolution, update cadence, biome immutability, hydrology boundaries, and Task 077 climatological-transport wording.
- Updated parked Task 040 to depend on this contract and the follow-up weather foundation while preserving its deferred runoff/discharge scope.
- Created concrete todo follow-up tasks 079-083 for seasonal temperature, seasonal precipitation, runtime atmospheric state, deterministic weather evolution, and gameplay-facing climate/weather queries.
- Confirmed no production types, simulation code, terrain generation behavior, climate-normal algorithms, precipitation behavior, or biome generation behavior were changed.
- `git diff --check` passed; Git reported only the repository's existing LF-to-CRLF warnings for touched Markdown files.
- No build or runtime tests were required because this task changes architecture documentation and backlog records only.

## Commit Message

`docs(procgen): define climate season weather layers`
