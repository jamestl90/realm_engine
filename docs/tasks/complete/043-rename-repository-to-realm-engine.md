# Rename Repository To Realm Engine

Status: complete
Area: Project Identity / Build / Repository

## Goal

Replace the legacy `rfd_game` identity with `realm_engine` across the repository, build outputs, documentation, and GitHub repository.

## Acceptance Criteria

- Rename the CMake project, main target, and executable to `realm_engine`.
- Rename internal `rfd_*` build targets and `RFD_*` build definitions to `realm_*` and `REALM_*` equivalents.
- Update active documentation, scripts, presets, and tests to use the new identity.
- Rename the GitHub repository from `rfd_game` to `realm_engine` and update the local `origin` URL.
- Ensure no active source or build configuration references the old identity.
- Run all test suites and build tests-disabled Debug and Release executables under the new name.
- Record the local folder rename as a final workspace-level step if it cannot safely occur while the repository is open.

## Implementation

- Renamed the CMake project, primary target, and executable to `realm_engine`.
- Renamed internal build targets from `rfd_*` to `realm_*` and build definitions from `RFD_*` to `REALM_*`.
- Updated presets, source gates, tests, asset documentation, procgen documentation, and historical task references.
- Renamed the GitHub repository to `jamestl90/realm_engine` and updated the local `origin` URL.
- Removed stale generated `rfd_*` build artifacts from prior verification directories.

## Verification

- Tests-disabled Debug and Release builds produce `realm_engine.exe` successfully.
- All eight freshly configured and rebuilt test suites pass under `realm_*` targets.
- Active source and build configuration contain no legacy project identifiers.
- `git ls-remote origin HEAD` resolves the renamed GitHub repository successfully.

## Local Folder

The open workspace remains at `D:\Repos\2D_Game_1`. Renaming it to `D:\Repos\realm_engine` is optional and should be done only after closing this workspace, then reopening Codex from the new path.
