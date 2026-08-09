# Centralize Runtime Asset Bundling

Status: testing
Area: Assets / Build

## Goal

Use the repository `assets/` directory as the single source of truth and automatically create self-contained Debug and Release runtime bundles.

## Acceptance Criteria

- Store compiled runtime shader artifacts under `assets/Shaders`.
- Keep editable shader sources and compiler scripts under `shaders/`.
- Bundle required shaders, selected fonts, font licenses, and SDL DLLs beside the executable after each build.
- Exclude test tile sheets and unused font variants from runtime outputs.
- Resolve runtime asset paths relative to the executable directory.
- Launch Debug and Release executables directly from their output folders.
