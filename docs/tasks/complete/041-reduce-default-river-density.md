# Reduce Default River Density

Status: complete
Area: Procgen / Hydrology Tuning

## Goal

Reduce the number of potential river channels shown by default while preserving user control over channel density.

## Context

The default minimum catchment area of `60` produces slightly too many visible river channels. Raising the threshold removes smaller upstream channels without changing terrain or drainage topology.

## Acceptance Criteria

- Raise the default minimum river catchment area conservatively.
- Preserve the existing `10`-unit UI adjustment step and `0..500` range.
- Do not change terrain generation or drainage topology.
- Rebuild the focused hydrology tests and the tests-disabled Debug game.

## Implementation

- Raised `river_min_drainage_area` from `60` to `80`.
- Kept the UI adjustment step at `10` and its range at `0..500`.

## Verification

- The freshly rebuilt focused hydrology test passes.
- A fixed-seed regression check confirms `80` exports strictly fewer channels than `60` while terrain and drainage topology remain identical.
- A tests-disabled Debug game build succeeds in `out/build/debug-river-density-verify`.
- The standard Debug executable could not be overwritten because it was running; its source objects compiled successfully before the linker encountered the Windows file lock.

## Current State

Task 042 later superseded the `80` default after full-size preview measurements showed it remained too dense.
