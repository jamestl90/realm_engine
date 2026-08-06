# Add Render Debug Draw Layer

Status: todo
Area: Rendering

## Goal

Add a simple debug drawing path for engine development.

## Context

Upcoming collision, layout, camera, and ECS work will be easier to validate with visual overlays.

## Acceptance Criteria

- Support basic lines and rectangles.
- Allow drawing in logical/world coordinates.
- Render debug geometry after sprites or in a clearly defined pass.
- Keep it easy to enable/disable.
- Document capabilities in `docs/RENDERING.md`.

## Notes

Start minimal. Collision bounds and grid overlays are the main early use cases.
