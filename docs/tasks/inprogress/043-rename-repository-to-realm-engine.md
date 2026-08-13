# Rename Repository To Realm Engine

Status: in progress
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
