# Rendering Feature Inventory

This document tracks rendering features currently present in the engine. It is a capability snapshot, not the full rendering architecture.

## Core System

- `GPUDevice`: creates and owns the SDL3 GPU device, currently requesting SPIR-V/DXIL/MSL support and forcing the Vulkan driver hint.
- `Renderer`: owns frame rendering for ECS sprites, including command buffer acquisition, swapchain texture acquisition, clear, sprite rendering, and present.
- `PipelineManager`: creates, owns, caches, and releases SDL GPU graphics pipelines.
- `TextureManager`: loads, creates, caches, looks up, and releases GPU textures.
- `FontManager`: loads TTF fonts through SDL_ttf and rasterises text into GPU textures.
- `UIRenderer`: renders UI after world sprites using the sprite pipeline. See [UI.md](UI.md) for UI-specific details.

## Sprite Rendering

- Sprites are rendered from ECS `Sprite` and `Transform` components.
- Supported sprite fields include texture ID, atlas region index, layer, horizontal/vertical flip, rotation, scale, and RGBA tint.
- Render commands are sorted by layer, then texture ID, then transform `z`.
- Sprites using the same texture are grouped into draw batches.
- Geometry is generated as indexed quads and uploaded each frame through SDL GPU transfer buffers.
- Rendering uses one shared sprite pipeline and indexed primitive draws.

## Frame And Camera

- `begin_frame()` acquires an SDL GPU command buffer and swapchain texture.
- `clear()` starts a render pass with the requested clear colour.
- `present()` ends any active render pass and submits the command buffer.
- Logical width/height are used to build an orthographic projection.
- Camera position and zoom are applied during sprite vertex generation.
- The renderer exposes the active command buffer and swapchain texture so the UI pass can render on top.

## Textures

- Textures can be loaded from file paths through SDL surface loading.
- Textures can be created directly from `SDL_Surface`.
- Surfaces are converted to `SDL_PIXELFORMAT_RGBA32` before upload when needed.
- Texture upload uses SDL GPU transfer buffers and copy passes.
- Texture IDs are cached by path.
- Atlas regions can be registered and looked up by texture ID and region name.
- A generated 1x1 white texture supports solid-colour rendering paths.

## Shaders And Pipelines

- Editable sprite shader sources and compilation scripts live in `shaders/`; compiled SPIR-V bytecode and reflection JSON live in `assets/Shaders` and are copied into each runtime bundle.
- The active core pipeline is `PipelineType::Sprite`.
- Pipeline configuration includes blend state, depth/stencil settings, primitive type, cull mode, front face, and scissor flag.
- Shader reflection data is loaded to populate SDL shader resource counts and validate the camera uniform block.
- Pipeline cache support exists for custom configurations.

## Current Notes

- Sprite interpolation receives `alpha`, but currently uses the current transform position until previous transform tracking exists.
- GPU instancing is not implemented; batching is CPU-built vertex/index data grouped by texture.
- Shape, text, and post-process pipeline enum values exist, but only the sprite core pipeline is initialised.
- Custom pipeline asset loading exists in interfaces, but the engine currently constructs `AssetManager` without a pipeline manager.
- Texture loading support depends on SDL surface loading; broader image-format support should be verified before relying on PNG/JPG in production.
