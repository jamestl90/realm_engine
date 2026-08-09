# Use Glyph Atlases For Dynamic UI Text

Status: testing
Area: UI / Rendering

## Goal

Keep frequently changing UI labels from creating and retaining a separate GPU texture for every unique string.

## Acceptance Criteria

- Render UI text from reusable SDL_ttf GPU glyph atlases.
- Preserve per-control font size, colour, UTF-8 shaping, and measured layout.
- Batch glyph geometry alongside solid and textured UI commands without changing draw order.
- Remove the unbounded whole-string texture cache.
- Generate each procgen map only once per control interaction and reuse it for summary labels.
- Destroy the GPU text engine before its fonts and GPU device.
- Verify dynamic text visually and build/test Debug and Release configurations.
