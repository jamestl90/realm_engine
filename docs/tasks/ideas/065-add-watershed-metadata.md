# Add Watershed Metadata

Status: idea
Priority: deferred
Area: Procgen / Hydrology

## Goal

Derive deterministic watershed identity and summary metadata from the existing greater-realm drainage topology.

## Context

Every greater-realm cell already has an acyclic downslope path and the map stores a complete drainage order, but callers must repeatedly traverse those links to answer basic geographic questions such as which outlet receives a cell, which cells share a catchment, and how large a basin is.

Watersheds are stable generated geography. They can support later region planning, biome rules, placement suitability, and runtime runoff without introducing weather state into procgen.

## Dependencies

- Task 027: deterministic priority drainage topology.
- Task 039: terrain-only catchment accumulation and potential channels.
- Task 064: explicit ocean and inland-water classification for outlet semantics.

## Acceptance Criteria

- Export a deterministic watershed identifier and terminal outlet reference for each applicable cell.
- Export compact watershed summaries including outlet, contributing area, cell count, and map-space bounds.
- Ensure every downstream chain retains one watershed identity and reaches its recorded outlet.
- Define behavior for ocean cells, inland water, maps without ocean, and boundary fallback outlets.
- Derive metadata without changing visual elevation, conditioned drainage elevation, downslope links, catchment area, or potential river channels.
- Add a compile-gated watershed debug view that does not affect generated output.
- Add tests for determinism, complete assignment, downstream consistency, summary totals, outlet validity, and representative edge cases.
- Integrate watershed generation into dependency-aware regeneration and document the new pipeline stage.
- Run focused procgen tests, full CTest, and Debug/Release builds.

## Notes

Watershed identity is natural-geography metadata, not a streaming chunk or persistence-region identifier.

No branch is required unless implementation reveals a broader drainage data redesign.

This idea is deliberately parked. Basin-aware simulation is interesting, but it is not a near-term requirement, and natural-disaster-style gameplay is not a current project priority. Reassess it if regions, biome rules, settlement suitability, or another non-disaster feature develops a concrete need for watershed identity.
