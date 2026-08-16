# Use Glyph Atlases For Dynamic UI Text

Status: complete
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

## Implementation

- `UIRenderer` renders SDL_ttf GPU atlas draw sequences into the existing ordered UI command stream.
- Font size, colour, UTF-8 text handling, and measured placement are preserved without a whole-string GPU texture cache.
- Procgen interaction regeneration is coalesced once per update, and engine shutdown destroys `UIRenderer` and its GPU text engine before font and GPU ownership.

## Verification

- All 14 CTest targets passed on 2026-08-16, including UI measurement and procgen-panel interaction coverage.
- Fresh Debug and Release runtime builds succeeded and both executables passed direct-launch startup smoke checks.
- Current runtime inspection confirmed dynamic procgen labels render clearly through the glyph-atlas path without visible layout regressions.
