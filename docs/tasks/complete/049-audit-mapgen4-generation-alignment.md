# Audit Mapgen4 Generation Alignment

Status: complete
Priority: high
Area: Procedural Generation / Architecture

## Goal

Audit the greater-realm generation pipeline against Mapgen4 and identify accidental discrepancies, semantic mismatches, weak approximations, and defects that prevent the engine from achieving the intended Mapgen4-style terrain behavior.

## Context

The engine deliberately takes inspiration from Mapgen4's layered generation model without copying every implementation decision. Several differences are intentional and already documented, including retaining a canonical regular grid, separating runtime weather from static terrain generation, and treating 2.5D geometry as derived render data. Those decisions are not defects and must not be reopened by this audit unless their recorded assumptions are factually incorrect.

The audit should instead find differences that have no deliberate design justification, such as globally coupled controls, incorrect parameter semantics, missing masks or falloffs, inappropriate normalization, wrong stage ordering, misleading debug output, or tests that prove only data changes rather than intended terrain behavior.

## Dependencies

- Current greater-realm, mountain, constraint, drainage, river, and debug-view implementations.
- Recorded decisions in tasks 023, 026, 027, 030, 031, 032, 039, and 048.
- `docs/PROCGEN.md` as the current engine capability and deviation record.
- Mapgen4's authoritative source code and Red Blob Games' accompanying technical notes.

## Audit Scope

- Landmass constraints, sea-level handling, island bias, coastline noise, and ocean depth.
- Base elevation, hills, mountain peak selection, distance propagation, jaggedness, radius, strength, ridges, valleys, and terrain noise.
- Authored constraint values, interpolation, masks, blend behavior, and pipeline ordering.
- Elevation normalization, clamping, coastal transitions, terrain classification, and slope calculation.
- Drainage conditioning, catchment accumulation, potential-channel extraction, and dependencies between terrain stages.
- Parameter names, ranges, defaults, UI behavior, and whether each control matches its documented responsibility.
- Debug views and tests where they may hide, exaggerate, or fail to detect generation defects.

## Classification Rules

Classify every observed difference as one of:

- `intentional deviation`: supported by an existing recorded engine decision.
- `planned capability gap`: valid future work that is already tracked or outside the current generation milestone.
- `unjustified discrepancy`: behavior differs from Mapgen4 without a documented reason and may undermine the intended result.
- `confirmed defect`: source analysis or focused evidence demonstrates incorrect or misleading behavior.

Do not label a difference as a defect merely because the engine uses a regular grid or has not yet implemented a later feature such as biomes, runtime weather, resources, settlements, or world streaming.

## Acceptance Criteria

- Build a stage-by-stage comparison matrix linking Mapgen4 source behavior to the corresponding engine implementation.
- Record every intentional deviation and the task or document that authorizes it.
- Exercise representative fixed seeds and parameter extremes to test control locality, independence, monotonicity, topology stability, and visual significance.
- Identify normalization, masking, blending, ordering, and parameter-contract discrepancies with concrete code evidence.
- Review existing tests for false confidence, vacuous assertions, and missing behavioral guarantees.
- Rank unjustified discrepancies and confirmed defects by severity and player-visible impact.
- Create focused remediation tasks for confirmed problems; do not combine unrelated fixes into the audit task.
- Update `docs/PROCGEN.md` with a concise, canonical statement of intentional Mapgen4 deviations so future audits do not reopen settled decisions.
- Make no generation-behavior changes as part of the audit itself, apart from non-production characterization tooling or tests needed to gather evidence.

## Authoritative References

- [Mapgen4 repository](https://github.com/redblobgames/mapgen4) for project intent and module ownership.
- [Mapgen4 `painting.ts`](https://github.com/redblobgames/mapgen4/blob/main/painting.ts) for automatic signed constraints, island bias, and paint values.
- [Mapgen4 `map.ts`](https://github.com/redblobgames/mapgen4/blob/main/map.ts) for coastline noise, hill/mountain composition, ocean depth, drainage, moisture, and river flow.
- [Mapgen4 `generate-points.ts`](https://github.com/redblobgames/mapgen4/blob/main/generate-points.ts) for Poisson mountain-site selection before terrain generation.
- [Mapgen4 `mapgen4.ts`](https://github.com/redblobgames/mapgen4/blob/main/mapgen4.ts) for public parameter names, defaults, and ranges.
- [Mapgen4 `geometry.ts`](https://github.com/redblobgames/mapgen4/blob/main/geometry.ts) for derived ridge/valley folds and river-aware render geometry.

## Stage Comparison

| Stage | Mapgen4 behavior | Current engine behavior | Classification |
|---|---|---|---|
| Automatic signed constraint | Five-octave signed fBm plus `island * (0.75 - 2 * squareDistance^2)`, then a positive-land mountain hint | Matches the island formula and range; adds an engine sea-level offset; omits the mountain hint | Sea level is an intentional deviation (tasks 023 and 026); mountain-hint omission is covered by task 050 |
| Authored constraint | Painting modifies the one dense signed constraint field used by elevation | A sparse serializable overlay blends into the automatic field | Intentional tooling adaptation (task 030) |
| Coastline detail | Adds high-frequency noise scaled by `1 - e^4` | Adds fBm through a `smoothstep` proximity mask that reaches zero at `abs(e) = 0.30` | Unjustified discrepancy; task 053 |
| Peak-site selection | Poisson sites are selected before terrain and pushed away from boundaries | One land candidate per spacing bucket is ranked by noise plus current signed land elevation, then greedily spaced | Confirmed locality defect and unjustified sampling discrepancy; task 052 |
| Mountain distance | Approximate randomized breadth-first distance on triangle adjacency | Deterministic jittered Dijkstra distance on eight-neighbor grid adjacency | Intentional grid adaptation (tasks 031 and 032) |
| Land relief | Blends low hills toward peak-distance mountains with local weight `max(e, 0)^2` | Globally normalizes an additive base, mountain, ridge, and valley stack, then adds terrain noise | Confirmed task-031 implementation mismatch; task 050 |
| Authored final relief | Signed constraint participates once, as the input to the land-relief blend | Authored value first changes signed topology, then directly replaces final relief by influence a second time | Confirmed semantic defect; task 051 |
| Ocean depth | Multiplies negative signed elevation by depth plus ocean noise | Maps signed water depth into normalized `0..seaLevel` elevation with independent ocean noise | Intentional normalized-output adaptation (tasks 016, 023, and 026) |
| Classification and coast metadata | Primarily continuous elevation and biome/render data | Exports water/ocean, terrain forms, coastline metadata, coast distance, and slope | Intentional engine extension (tasks 016 and 025) |
| Drainage | Priority traversal starts in deep ocean on triangle adjacency; generated rainfall/moisture drives flow and can flatten uphill trunks | Eight-neighbor priority flood exports separate conditioned drainage elevation; terrain area drives potential channels | Intentional adaptation (tasks 027 and 039) |
| Terrain geometry | Irregular dual mesh folds around ridges, valleys, coasts, and rivers | Canonical grid plus disposable continuous regular-grid heightfield | Intentional architecture decision (tasks 032 and 033) |

## Findings

### High: Land Relief Does Not Implement The Intended Local Blend

`src/procgen/GreaterRealm.cpp` normalizes `base + mountain + ridge - valley` against global parameter headroom. This is structurally different from Mapgen4's low-hill/mountain-target blend and from task 031's acceptance criterion to blend peak elevation with the land constraint and low-amplitude hills. Task 048 fixed one global mountain-strength coupling without changing this broader composition. The current behavior is a confirmed defect because the recorded implementation does not satisfy the earlier task contract.

Remediation: task 050.

### High: Local Painting Can Relocate Distant Peaks

`src/procgen/MountainPeaks.cpp` selects candidates from current non-water cells and includes signed land elevation in candidate priority. Any edit that changes topology or candidate ranking can therefore rebuild the global peak field. A test-only probe using seed `314159`, size `96x72`, and one mountain brush at `(0.5, 0.5)` with radius `0.16` kept four total peaks but removed two identities and introduced two others. It changed 1,027 final elevations outside normalized radius `0.30` from the brush center.

This violates the local-edit responsibility expected by task 030 and differs from Mapgen4's fixed pre-terrain peak sites. The existing test checks only one distant corner, so it does not detect the effect.

Remediation: task 052.

### High: Authored Constraints Are Composed Twice

`src/procgen/GreaterRealm.cpp` first blends authored elevation into `broad_constraint`, which controls coastline and topology. During final land assembly it then blends the same signed authored value directly into normalized relief. Mapgen4 uses the signed value once and derives local relief from it. The second interpolation gives paint tools two unrelated responsibilities and bypasses the normal relief composition.

Remediation: task 051 after task 050 defines the corrected common path.

### Medium: Coastline Attenuation Has No Recorded Justification

Both implementations localize high-frequency detail toward the coastline, but their masks differ materially. No existing task records why the engine's `abs(e) < 0.30` support is preferable to Mapgen4's polynomial attenuation. This is an unjustified discrepancy, not yet a confirmed visual defect.

Remediation: task 053 will compare both before choosing.

### Medium: Parameter Tests Can Pass The Wrong Behavior

The suite strongly covers deterministic output, valid topology, peak spacing, acyclic drainage, channel connectivity, serialization, and debug output. However, several relief controls are accepted when total elevation difference merely exceeds `1.0`. Those tests do not establish where the change occurred, whether it was monotonic, whether it was visible relative to map size, or whether another stage changed unexpectedly. Constraint locality checks only the edited center and one corner.

Remediation: the owning contracts are folded into tasks 050 through 053 rather than maintained as a separate cross-cutting test ticket.

## Characterization Results

The temporary audit probe was compiled only with `REALM_TEST_BUILD` and removed after collecting results. It made no generation changes.

- Seed `314159`, `96x72`: peak spacing `14` produced 11 sites; spacing `48` produced one site. The count control is monotonic for the probe map.
- Seed `314159`, `96x72`: ocean depth `0` to `2` lowered 2,942 water cells and changed zero land elevations. Ocean depth is independent from land relief and topology for the probe map.
- Seed `314159`, `96x72`: the center mountain brush moved the signed center constraint from `0.170673` to `0.865052` and final elevation from `0.598382` to `0.947975`, while also causing the nonlocal peak changes described above.
- Existing fixed-seed tests confirm island-bias clamping and topology response, sea-level monotonic land-area response, mountain-strength locality after task 048, peak-radius influence growth, jagged-distance response, and terrain-noise/ocean-depth topology independence.

## Test Audit

- Keep: exact determinism, signed topology, ocean connectivity, sea-level response, mountain-strength locality, peak spacing and descending distance paths, constraint serialization, priority-drainage invariants, catchment accumulation, and channel threshold independence.
- Strengthen: base, ridge, valley, terrain-noise, peak radius, jaggedness, and coastline controls need spatial and significance assertions across several seeds.
- Add with remediation: fixed peak identity under painting, bounded distant response, one-stage authored semantics, hill-versus-mountain separation, coastline attenuation, and parameter ownership. These belong in tasks 050 through 053 alongside the behavior they protect.
- Avoid: snapshotting the current complete elevation field. Tasks 050 through 053 will deliberately correct accidental output and should be guarded by behavioral contracts instead of preserving exact flawed maps.

## Intentional Deviations Confirmed

- Canonical regular-grid storage and direct local-region handoff: task 032.
- Eight-neighbor grid adaptations for mountain distance and drainage: tasks 031, 032, and 027.
- Independent sea-level offset and normalized final elevation/water-depth representation: tasks 016, 023, and 026.
- Explicit terrain-form, coastline, coast-distance, and slope metadata: tasks 016 and 025.
- Terrain-only catchment and potential channels, with rainfall and runoff deferred to runtime simulation: task 039.
- Derived continuous regular-grid 2.5D heightfield instead of Mapgen4's irregular folded mesh: tasks 032 and 033.

These are now summarized canonically in `docs/PROCGEN.md` so future comparisons do not reopen them without new evidence.

## Remediation Order

1. Task 050: correct foundational land-relief composition.
2. Task 052: stabilize peak sites independently from mutable terrain.
3. Task 051: route authored constraints once through the corrected composition.
4. Task 053: evaluate and resolve coastline attenuation.

## Implementation

- Audited all greater-realm, constraint, peak, drainage, river, debug, UI-setting, and procgen test paths against Mapgen4's authoritative source.
- Classified settled architecture decisions separately from accidental generation differences.
- Added tasks 050 through 053 for focused remediation, with each task owning the behavioral contracts for the generation stage it changes.
- Added the intentional-deviation boundary to `docs/PROCGEN.md`.
- Made no production or generation-behavior changes.

## Verification

- A compile-gated temporary probe characterized fixed-seed parameter extremes and local-edit side effects, then was removed from the source and CMake graph.
- All 11 existing CTest targets pass from `out/build/debug-with-tests`.
- `git diff --check` passes.
- The final worktree contains documentation and task records only; no generation, application, test, or build-system source changed.
