# Wire AssetManager Runtime Dependencies

Status: todo
Area: Assets

## Goal

Wire `AssetManager` to the runtime systems it is designed to coordinate.

## Context

Engine startup currently constructs `AssetManager` with a texture manager but without an audio system or pipeline manager. That limits audio and custom pipeline asset paths.

## Acceptance Criteria

- Pass `PipelineManager` into `AssetManager` during engine initialization.
- Decide whether an `AudioSystem` instance should be created now or deferred.
- Ensure pipeline asset APIs can reach the runtime pipeline manager.
- Update `docs/ASSETS.md`.

## Notes

Do not implement a full audio engine in this task unless it naturally falls out of minimal wiring.
