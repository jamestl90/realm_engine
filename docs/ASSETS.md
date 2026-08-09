# Asset Feature Inventory

This document tracks asset-system features currently present in the engine. It is a capability snapshot, not the full asset architecture.

## Core Types

- `AssetHandle<T>`: type-safe handle with 32-bit ID and 32-bit generation.
- `AssetState`: `Unloaded`, `Loading`, `Loaded`, and `Failed`.
- Handle aliases exist for texture, audio, font, data, animation, tileset, and pipeline assets.
- Asset cache entries track asset data, generation, ref count, state, path, last modified time, and pinned state.

## Asset Metadata Types

- `TextureAsset`: texture ID, dimensions, filter/wrap settings, source path, and named regions.
- `AudioAsset`: decoded buffer pointer, length, sample rate, channels, duration, default volume, streaming flag, and source path.
- `FontAsset`: planned glyph atlas/metrics data.
- `AnimationAsset`: planned frame/event data.
- `DataAsset`: raw binary data with source path.
- `TilesetAsset`: grid settings, tile properties, terrain definitions, animated tile mapping, and tile region helpers.
- `PipelineAsset`: base pipeline type, pipeline config, runtime pipeline handle, shader paths, and source path.

## Loading And Lookup

- `AssetManager` owns per-type caches for textures, audio, fonts, animations, data, tilesets, and pipelines.
- Paths are normalised and resolved relative to a configurable base path, defaulting to `assets/`.
- Repeated loads return cached handles and increment ref counts when already loaded.
- Texture loading creates GPU textures through `TextureManager`.
- Audio loading uses `SDL_LoadWAV`.
- Data loading stores raw file bytes.
- Pipeline loading from config can create cached graphics pipelines when a `PipelineManager` is available.
- Text and binary file helpers are available.

## Lifetime And Reloading

- `unload_*()` methods decrement ref counts and unload when the count reaches zero and the asset is not pinned.
- `pin()`, `unpin()`, `add_ref()`, and `release_ref()` exist for all handle types.
- `collect_unused()` immediately removes unreferenced, unpinned assets; the grace-period argument is currently unused.
- Hot reload can be toggled at runtime.
- `poll_hot_reload()` currently checks loaded textures, data assets, and pipelines.
- Reload callbacks can be registered.
- `force_reload()` exists for texture, data, and pipeline handles.
- `clear()` releases all cached assets.

## Batch And Manifest Loading

- `load_batch()` synchronously loads assets based on file extension and reports aggregate success through a callback.
- `preload_manifest()` supports a simple line-oriented `type:path` format.
- Manifest JSON parsing described in the architecture is not currently implemented.

## Source And Build Bundling

- `assets/` is the authoritative source directory for runtime content.
- CMake copies an explicit allowlist of runtime shaders, fonts, and font license files beside `rfd_game` after each Debug or Release build.
- Generated build copies are disposable; assets should never be maintained directly under `out/build/`.
- Test tile sheets and unused font variants are not included in runtime output bundles.
- Compiled sprite shader artifacts live in `assets/Shaders`; editable HLSL sources and shader compilation scripts remain in `shaders/`.

## Current Notes

- The asset system is synchronous; `LoadPriority` exists for future async behavior.
- `load_font_internal()` is currently a placeholder and returns failure.
- `load_animation_internal()` is currently a placeholder and returns failure.
- `load_tileset_internal()` is currently a placeholder and returns failure.
- Pipeline definition loading reads a file but currently creates a default sprite pipeline; JSON parsing is not implemented.
- Engine startup currently constructs `AssetManager` without an audio system or pipeline manager, so audio playback integration and custom pipeline asset loading are not wired through the default engine path.
- Runtime font rendering used by UI currently goes through `rendering::FontManager`, not `AssetManager::load_font()`.
- Asset packing into an indexed blob is not implemented; current builds bundle selected assets as loose files.
