# Rendering Feature Inventory

This document tracks rendering features currently present in the engine. It is a capability snapshot, not the full rendering architecture.

## Core System

- `GPUDevice`: creates and owns the SDL3 GPU device, currently requesting SPIR-V/DXIL/MSL support and forcing the Vulkan driver hint.
- `Renderer`: owns frame command submission, swapchain and depth-target acquisition, render-pass transitions, clear, and present.
- `PipelineManager`: creates, owns, caches, and releases SDL GPU graphics pipelines.
- `TerrainRenderer`: uploads and draws derived regular-grid heightfields through the terrain pipeline.
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

## Terrain Rendering

- `TerrainMesh` derives a continuous indexed heightfield from regular-grid elevation and RGBA display data without changing canonical procgen cells.
- Vertices contain planar position, elevation, elevation gradient, and colour; gradients produce lighting normals in the vertex shader.
- The terrain pipeline uses 32-bit indices, depth testing/writes, an oblique orthographic camera, adjustable elevation scale, and ambient plus directional lighting.
- The current greater-realm mesh is uploaded only when map geometry or debug colours change. Presentation mode and elevation-scale changes do not regenerate the map.
- A `256x192` greater realm produces 49,152 vertices and 292,230 indices in one indexed terrain draw.

## Frame And Camera

- `begin_frame()` acquires an SDL GPU command buffer and swapchain texture.
- `clear()` starts the requested clear pass and attaches a lazily allocated depth target only while 3D terrain is enabled.
- Terrain renders first with depth enabled. Sprites and UI then render through a separate colour-only pass so existing 2D composition remains unchanged.
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
- The active core pipelines are `PipelineType::Sprite` and `PipelineType::Terrain`.
- Pipeline configuration includes blend state, depth/stencil settings, primitive type, cull mode, front face, and scissor flag.
- Shader reflection data is loaded to populate SDL shader resource counts and validate the camera uniform block.
- Pipeline cache support exists for custom configurations.

## Current Notes

- Sprite interpolation receives `alpha`, but currently uses the current transform position until previous transform tracking exists.
- GPU instancing is not implemented; batching is CPU-built vertex/index data grouped by texture.
- Shape, text, and post-process pipeline enum values exist, but their core pipelines are not initialised.
- Custom pipeline asset loading exists in interfaces, but the engine currently constructs `AssetManager` without a pipeline manager.
- Texture loading support depends on SDL surface loading; broader image-format support should be verified before relying on PNG/JPG in production.
- The terrain camera is currently fixed and intended for debug inspection rather than player camera control.
- Existing world sprites do not yet share terrain projection or depth; task 047 tracks depth-aware 2.5D sprite integration.
- The greater-realm heightfield is currently one mesh. Future world streaming should partition derived meshes by region while retaining the canonical map representation.
- Collision remains 2D/canonical-grid data and is not derived from rendered triangles.
