# Add Mountain Peak Distance Fields

Status: todo
Area: Procgen / Elevation

## Goal

Generate explicit mountain peaks and use distance fields to shape coherent mountain masses.

## Context

Mapgen4 preselects peak locations and propagates a jagged distance field from them. The current generator uses ridged noise masks, which create high terrain but not explicit peaks with controlled spacing and falloff.

## Acceptance Criteria

- Select deterministic peak locations with configurable density or spacing.
- Compute a deterministic, optionally jagged distance field from peaks.
- Blend peak elevation with the existing land constraint and low-amplitude hill relief.
- Export enough peak metadata for later hydrology and debug inspection.
- Add tests for peak determinism, spacing, distance monotonicity, and parameter response.
- Add compile-gated debug visualization and update procgen documentation.

