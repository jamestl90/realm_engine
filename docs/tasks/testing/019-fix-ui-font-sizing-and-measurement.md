# Fix UI Font Sizing And Measurement

Status: testing
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
