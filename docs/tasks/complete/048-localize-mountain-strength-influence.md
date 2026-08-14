# Localize Mountain Strength Influence

Status: complete
Area: Procedural Generation

## Goal

Make mountain strength affect localized mountain relief instead of shifting the normalized elevation of the entire landmass.

## Context

The current relief range includes `mountain_weight`. Increasing mountain strength therefore changes the normalization denominator for every land cell, including cells with no mountain influence. Default peak radius and spacing also create broad overlapping influence, making the control appear global rather than mountain-specific.

Mapgen4 computes a separate peak-distance mountain profile and blends it into local land elevation without adding a mountain-layer weight to a global normalization denominator. The greater-realm generator should follow that composition principle while retaining its canonical regular-grid representation.

## Acceptance Criteria

- Normalize relief with fixed mountain headroom independent from the current mountain-strength setting.
- Apply mountain strength locally through the exported mountain influence field.
- Increasing mountain strength must not lower land or alter cells with zero mountain influence.
- Preserve land/water topology and deterministic generation.
- Keep peak spacing, radius, and jaggedness responsibilities unchanged.
- Add regression tests for localized mountain-strength behavior.
- Update procgen documentation with the revised composition rule.

## Implementation

- Defined the default mountain strength once as `DEFAULT_MOUNTAIN_STRENGTH`.
- Replaced the adjustable mountain-strength term in the relief denominator with fixed headroom equal to that default.
- Retained mountain strength only in each cell's peak-distance influence contribution, so cells with zero influence no longer move when the setting changes.
- Preserved the established output at the default strength, including terrain classification and drainage behavior.

An initial, broader mountain-profile blend was evaluated but changed drainage enough to remove the tuned default river network on an existing hydrology fixture. This task therefore fixes the normalization coupling without combining it with a terrain retuning exercise.

## Testing

- Added a regression test comparing zero and maximum mountain strength.
- Verified land/water topology and water elevation remain unchanged.
- Verified the fixture contains both influenced and uninfluenced land, zero-influence land remains exactly unchanged, and stronger mountains never lower land.
- Verified influenced terrain gains measurable elevation.
- Built `debug-no-tests` and `release-no-tests` successfully.
- Built every test target and passed all 11 CTest tests, including greater-realm, mountain-peak, hydrology, and terrain-rendering coverage.
