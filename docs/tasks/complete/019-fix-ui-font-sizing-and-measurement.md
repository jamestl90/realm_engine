# Fix UI Font Sizing And Measurement

Status: complete
Area: UI / Rendering

## Goal

Make UI layout and rendering use the same font face, point size, and SDL_ttf text metrics.

## Acceptance Criteria

- Render UI text at each control's requested font size.
- Cache and reuse size variants of the active font face.
- Measure `TextBlock`, `Button`, `TextBox`, and `ComboBox` text using SDL_ttf metrics.
- Position button labels, text selections, cursors, aligned text, and combo-box content using measured dimensions.
- Retain character-based estimates only as a fallback when a font cannot be measured.
- Add automated layout tests covering renderer-provided text metrics.
- Verify debug and release builds.

## Implementation

- `FontManager` caches font variants by face and point size.
- UI measurement resolves the same font variant used by rendering and uses SDL_ttf metrics, retaining character estimates only as a fallback.
- Text layout for the covered controls uses measured dimensions.

## Verification

- `ui_text_measurement` passed as part of the 14-target CTest suite on 2026-08-16.
- Fresh Debug and Release runtime builds succeeded.
- Current runtime inspection confirmed readable, aligned procgen controls and dynamic summary text at their requested sizes.
