# Preserve River Connections To Painted Lakes

Status: todo
Priority: high
Area: Procgen / Hydrology / Terrain Editing

## Goal

When an authored ocean or shallow-water stroke creates an enclosed inland water body that contacts an existing river, preserve the unaffected river approach and terminate it cleanly at the lake instead of regenerating a replacement route around the shoreline.

## Context

Authored terrain painting invalidates terrain fields and rebuilds downstream classification, drainage, and potential river channels. Inland water is classified separately from ocean, but the priority-drainage topology currently seeds ocean cells as outlets and does not give painted inland water an explicit river-terminal contract.

Visual review shows an established river diverting around a newly painted lake. For the editing workflow, the expected result is that the river continues along its unaffected upstream path until it meets the lake. This task treats the lake as a terminal water body; automatic lake outlets or through-flow are separate future hydrology behavior.

## Dependencies

- Task 027: deterministic priority drainage and outlet topology.
- Task 036: dependency-aware procgen regeneration.
- Task 055: direct authored-constraint painting controls.
- Task 064: ocean and inland-water classification.

## Acceptance Criteria

- Add a deterministic regression fixture that generates a visible river, paints an enclosed ocean or shallow-water constraint into contact with it, and reproduces the current shoreline diversion.
- Treat contacted inland water as a valid terminal for incoming potential river channels without reclassifying it as ocean.
- Preserve river segments outside the painted terrain influence exactly up to the first affected approach segment when their terrain inputs remain unchanged.
- Export or render a final connection from the upstream river approach to the first contacted inland-water cell or shoreline location.
- Do not export replacement river segments that skirt the lake perimeter solely to avoid entering the new water body.
- Clip or omit river segments inside the lake after the first contact; this task does not synthesize an outlet, through-lake channel, or downstream continuation.
- Preserve deterministic, adjacent, acyclic drainage data and valid channel indices after inland water becomes a terminal.
- Keep ocean/coastal termination behavior unchanged.
- Keep river-threshold and width-only regeneration behavior unchanged.
- Ensure repeated full generation from the same terrain settings and authored constraints produces the same river-to-lake connection without relying on transient renderer state.
- Ensure staged regeneration after painting matches the supported clean-generation result, or explicitly version and document any retained edit-history data if exact path preservation requires it.
- Verify both flat and `3D` debug overlays display the incoming river meeting the lake without a visible gap or shoreline detour.
- Update `docs/PROCGEN.md` with inland-water river-terminal semantics and the explicit absence of automatic lake outlets.
- Run focused hydrology, painting, staged-regeneration, and debug-view tests, followed by full CTest and Debug/Release builds.

## Investigation Notes

- First determine whether seeding inland-water cells as drainage terminals is sufficient to preserve the expected approach.
- Prefer a hydrological terminal rule over retaining a stale renderer-only river overlay.
- If exact upstream preservation cannot be derived from the final terrain and constraints alone, document the required stable-channel or edit-history ownership before adding mutable state to procgen.
- Reopened after implementation attempt: seeding inland-water cells as drainage terminals is insufficient by itself. Painting shallows changes terrain classification/conditioning enough to alter the upstream drainage graph, so the original river path can disappear before it reaches the new lake terminal.
- Next attempt should explicitly choose an ownership model for preserved river geometry, such as a stable pre-edit channel overlay clipped by edited water, a drainage-lock constraint for river corridors, or a separate lake-paint layer that does not perturb terrain conditioning outside the intended water mask.

## Out Of Scope

- Shared lake-surface elevation or lake filling.
- Automatic lake outlets, multiple inflows, through-lake routing, or watershed metadata.
- Runtime rainfall, runoff, discharge, flooding, erosion, or seasonal water levels.
- General-purpose manual river drawing or river path locking beyond the painted-lake interaction.

## Notes

The task should remain narrowly focused on river-to-lake contact. Do not solve the visual symptom by preventing required terrain or hydrology invalidation globally.
