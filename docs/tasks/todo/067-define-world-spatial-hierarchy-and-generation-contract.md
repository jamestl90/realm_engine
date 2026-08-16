# Define World Spatial Hierarchy And Generation Contract

Status: todo
Priority: high
Area: World / Procgen Architecture

## Goal

Define how coarse greater-realm geography maps to streamable world regions, storage chunks, and playable top-down local tiles.

## Context

`GreaterRealmMap` is a regular grid of coarse geographic samples. Treating each cell as a gameplay region without an explicit scale and ownership decision would couple local generation, streaming, persistence, and rendering to debug-map resolution.

The game is top-down 2D. The derived 3D terrain view is for map and world inspection and must not own gameplay coordinates.

## Dependencies

- Task 015: world-management implementation review and breakdown.

## Acceptance Criteria

- Define the terms and responsibilities for greater-realm cell, world region, storage chunk, local tile, and world-space position.
- Decide whether region and chunk are distinct levels or whether one level is sufficient for the first implementation.
- Define coordinate origins, axis directions, bounds, neighbor rules, and conversions between hierarchy levels.
- Define how finite greater-realm bounds interact with engine APIs that may use signed region coordinates.
- Define how a local generation request samples coarse elevation, terrain form, coastline, inland water, drainage, and river metadata.
- Define world identity, generator-version identity, deterministic seed derivation, and authored-constraint ownership.
- Define which data is generated, persisted, streamed, runtime-only, renderer-only, or application-owned.
- Record how later climate tendencies and biome rules attach without making application biome labels part of core procgen.
- Update `docs/ARCHITECTURE.md` and `docs/PROCGEN.md` with the approved contract.
- Revise Tasks 068-073 if the approved hierarchy changes their assumptions.

## Notes

This is a documentation and architecture decision task. It must not implement region storage or local terrain generation.

No branch is required because this task changes documentation and backlog contracts only.

## Scheduling Note

This is committed future work, scheduled after the current grand-scale greater-realm generation phase.
