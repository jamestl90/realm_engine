# Fit Procgen Preview to 1920x1080

Status: complete
Priority: medium
Area: Application / Procgen Debug Layout

## Goal

Use a 1920x1080 application window and fit the flat heightmap directly beside the debug panel without clipping or horizontal dead space.

## Context

The engine already uses a 1920x1080 logical coordinate space, but the native window opens at 2460x1440. The procgen preview is positioned at a hard-coded 65% horizontal center with a fixed 4x texture scale, leaving a visible gap after the 620-pixel debug panel.

## Acceptance Criteria

- Open the native application window at 1920x1080.
- Share one explicit debug-panel width between panel construction and preview layout.
- Place the flat preview's left edge directly against the panel's right edge.
- Uniformly scale the preview to use the available logical width without clipping or distorting its aspect ratio.
- Keep preview bounds and terrain painting aligned with the rendered map.
- Reapply the fitted layout when a regenerated texture changes map dimensions.
- Build and test the affected Debug and Release configurations.

## Dependencies

- Task 054 for the current two-column debug controls.
- Task 055 for painting through rendered preview bounds.

## Implementation Decisions

- Changed the native startup window from 2460x1440 to 1920x1080, matching the engine's existing logical resolution.
- Exported one `620`-pixel debug-panel width constant and reused it for panel measurement, flat preview placement, and the 3D viewport boundary.
- Replaced fixed 4x sprite scaling and 65% positioning with a uniform fit inside the remaining 1300x1080 logical area.
- Kept the source map aspect ratio intact. At the current 256x192 map size the preview occupies 1300x975 and is vertically centered with 52.5 pixels above and below.
- Kept painting aligned by continuing to derive interaction bounds from the fitted sprite transform and scale.
- Added a clamped terrain-renderer viewport offset so application layouts can position 3D terrain without hard-coded renderer knowledge of the debug panel.

## Verification

- Focused debug-panel, constraint-painting, and terrain-mesh tests passed.
- Full `ctest --test-dir out/build/debug-with-tests --output-on-failure` passed, 14/14.
- `cmake --build --preset debug-no-tests` passed.
- `cmake --build --preset release-no-tests` passed.
- Launched the native application and confirmed a 1920x1080 pixel window.
- Visually confirmed the flat preview starts at logical `x=620`, spans the full remaining 1300-pixel width, preserves its aspect ratio at 1300x975, and remains vertically centered.
