# Implement JSON Asset Manifest

Status: todo
Area: Assets

## Goal

Replace or supplement the simple line-oriented manifest loader with the documented JSON manifest format.

## Context

`preload_manifest()` currently expects simple `type:path` lines, while `docs/ARCHITECTURE.md` describes a JSON manifest.

## Acceptance Criteria

- Define the manifest JSON schema the engine will support now.
- Implement manifest parsing with RapidJSON.
- Support preload entries for implemented asset types.
- Report missing or unsupported asset entries clearly.
- Update `docs/ASSETS.md` and `docs/ARCHITECTURE.md` if the schema changes.

## Notes

Keep this scoped to preload/validation. Build-time asset packing can stay future work.
