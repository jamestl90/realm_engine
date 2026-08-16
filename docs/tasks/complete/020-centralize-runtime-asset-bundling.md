# Centralize Runtime Asset Bundling

Status: complete
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

## Implementation

- CMake copies an explicit allowlist of runtime shaders, fonts, font licensing, and SDL libraries from repository-owned sources into each executable directory.
- Editable shader sources remain under `shaders/`, while compiled runtime shader artifacts live under `assets/Shaders`.
- Runtime asset lookup resolves from the executable bundle rather than generated build files acting as source assets.

## Verification

- Fresh Debug and Release builds succeeded on 2026-08-16.
- Both bundles contained every required runtime library, font, license, shader, and reflection file, with no unexpected font variants.
- Debug and Release executables both passed direct-launch startup smoke checks from their respective output directories.
