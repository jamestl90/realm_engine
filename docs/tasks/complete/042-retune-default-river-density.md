# Retune Default River Density

Status: complete
Area: Procgen / Hydrology Tuning

## Goal

Substantially reduce excessive default river-channel density using measured threshold comparisons.

## Context

Raising the minimum catchment area from `60` to `80` did not reduce the visible channel network enough. The next default should be selected from representative fixed-seed channel counts rather than another small incremental adjustment.

## Acceptance Criteria

- Compare channel counts across representative seeds and catchment thresholds.
- Choose a substantially sparser default while retaining smaller/larger values in the debug controls.
- Verify the selected threshold reduces channels without changing terrain or drainage topology.
- Rebuild the focused hydrology test and standard tests-disabled Debug executable.

## Measurements

At 256x192 with seed `141421`:

| Minimum catchment area | Channel segments |
| ---: | ---: |
| 80 | 1035 |
| 160 | 657 |
| 240 | 509 |
| 320 | 395 |
| 400 | 320 |
| 500 | 263 |
| 640 | 201 |
| 800 | 152 |
| 1000 | 123 |

## Implementation

- Raised the default minimum catchment area from `80` to `800`.
- Changed the debug control to a `0..2000` range with `100`-unit steps.
- Removed temporary measurement output after selecting the default.

## Verification

- The freshly rebuilt focused hydrology test passes.
- A fixed-seed regression check confirms the new default is more than four times sparser than `80`, retains visible channels, and leaves terrain and drainage topology unchanged.
- The standard tests-disabled Debug executable builds successfully.
