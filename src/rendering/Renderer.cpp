#include "../../include/rendering/Renderer.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include "../../include/rendering/TextureManager.hpp"
#include "../../include/rendering/PipelineManager.hpp"
#include "../../include/rendering/Sprite.hpp"
#include "../../include/rendering/UniformBuffers.hpp"
#include "../../include/ecs/Component.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>

namespace rendering {

Renderer::Renderer(GPUDevice* device)
    : device_(device)
    , window_(nullptr) {
    assert(device_ && "GPU device cannot be null");

    vertex_data_.reserve(MAX_SPRITES * 4);
    index_data_.reserve(MAX_SPRITES * 6);
    render_commands_.reserve(1000);
    batches_.reserve(100);
}

Renderer::Renderer(GPUDevice* device, SDL_Window* window)
    : device_(device)
    , window_(window) {
    assert(device_ && "GPU device cannot be null");
    assert(window_ && "Window cannot be null");

    vertex_data_.reserve(MAX_SPRITES * 4);
    index_data_.reserve(MAX_SPRITES * 6);
    render_commands_.reserve(1000);
    batches_.reserve(100);
}

Renderer::~Renderer() {
    release_gpu_resources();
}

Renderer::Renderer(Renderer&& other) noexcept
    : device_(other.device_)
    , window_(other.window_)
    , command_buffer_(other.command_buffer_)
    , vertex_buffer_(other.vertex_buffer_)
    , index_buffer_(other.index_buffer_)
    , sampler_(other.sampler_)
    , current_pass_(other.current_pass_)
    , swapchain_texture_(other.swapchain_texture_)
    , swapchain_width_(other.swapchain_width_)
    , swapchain_height_(other.swapchain_height_)
    , vertex_data_(std::move(other.vertex_data_))
    , index_data_(std::move(other.index_data_))
    , batches_(std::move(other.batches_))
    , gpu_resources_created_(other.gpu_resources_created_)
    , render_commands_(std::move(other.render_commands_))
    , camera_x_(other.camera_x_)
    , camera_y_(other.camera_y_)
    , camera_zoom_(other.camera_zoom_) {
    other.device_ = nullptr;
    other.window_ = nullptr;
    other.command_buffer_ = nullptr;
    other.vertex_buffer_ = nullptr;
    other.index_buffer_ = nullptr;
    other.sampler_ = nullptr;
    other.current_pass_ = nullptr;
    other.swapchain_texture_ = nullptr;
    other.gpu_resources_created_ = false;
}

Renderer& Renderer::operator=(Renderer&& other) noexcept {
    if (this != &other) {
        release_gpu_resources();

        device_ = other.device_;
        window_ = other.window_;
        command_buffer_ = other.command_buffer_;
        vertex_buffer_ = other.vertex_buffer_;
        index_buffer_ = other.index_buffer_;
        sampler_ = other.sampler_;
        current_pass_ = other.current_pass_;
        swapchain_texture_ = other.swapchain_texture_;
        swapchain_width_ = other.swapchain_width_;
        swapchain_height_ = other.swapchain_height_;
        vertex_data_ = std::move(other.vertex_data_);
        index_data_ = std::move(other.index_data_);
        batches_ = std::move(other.batches_);
        gpu_resources_created_ = other.gpu_resources_created_;
        render_commands_ = std::move(other.render_commands_);
        camera_x_ = other.camera_x_;
        camera_y_ = other.camera_y_;
        camera_zoom_ = other.camera_zoom_;

        other.device_ = nullptr;
        other.window_ = nullptr;
        other.command_buffer_ = nullptr;
        other.vertex_buffer_ = nullptr;
        other.index_buffer_ = nullptr;
        other.sampler_ = nullptr;
        other.current_pass_ = nullptr;
        other.swapchain_texture_ = nullptr;
        other.gpu_resources_created_ = false;
    }
    return *this;
}

bool Renderer::create_gpu_resources() {
    if (gpu_resources_created_) {
        return true;
    }

    SDL_GPUDevice* gpu = device_->handle();

    // Create vertex buffer
    SDL_GPUBufferCreateInfo vertex_buffer_info{};
    vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_buffer_info.size = static_cast<Uint32>(VERTEX_BUFFER_SIZE);

    vertex_buffer_ = SDL_CreateGPUBuffer(gpu, &vertex_buffer_info);
    if (!vertex_buffer_) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create vertex buffer: %s", SDL_GetError());
        return false;
    }

    // Create index buffer
    SDL_GPUBufferCreateInfo index_buffer_info{};
    index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_buffer_info.size = static_cast<Uint32>(INDEX_BUFFER_SIZE);

    index_buffer_ = SDL_CreateGPUBuffer(gpu, &index_buffer_info);
    if (!index_buffer_) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create index buffer: %s", SDL_GetError());
        SDL_ReleaseGPUBuffer(gpu, vertex_buffer_);
        vertex_buffer_ = nullptr;
        return false;
    }

    // Create sampler for texture sampling
    SDL_GPUSamplerCreateInfo sampler_info{};
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    sampler_ = SDL_CreateGPUSampler(gpu, &sampler_info);
    if (!sampler_) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create sampler: %s", SDL_GetError());
        SDL_ReleaseGPUBuffer(gpu, vertex_buffer_);
        SDL_ReleaseGPUBuffer(gpu, index_buffer_);
        vertex_buffer_ = nullptr;
        index_buffer_ = nullptr;
        return false;
    }

    gpu_resources_created_ = true;
    SDL_Log("Renderer GPU resources created successfully");
    return true;
}

void Renderer::release_gpu_resources() {
    if (!device_ || !device_->is_valid()) {
        return;
    }

    SDL_GPUDevice* gpu = device_->handle();

    if (sampler_) {
        SDL_ReleaseGPUSampler(gpu, sampler_);
        sampler_ = nullptr;
    }
    if (index_buffer_) {
        SDL_ReleaseGPUBuffer(gpu, index_buffer_);
        index_buffer_ = nullptr;
    }
    if (vertex_buffer_) {
        SDL_ReleaseGPUBuffer(gpu, vertex_buffer_);
        vertex_buffer_ = nullptr;
    }

    gpu_resources_created_ = false;
}

bool Renderer::begin_frame() {
    // Create GPU resources on first frame
    if (!gpu_resources_created_ && !create_gpu_resources()) {
        return false;
    }

    // Acquire command buffer for this frame
    command_buffer_ = SDL_AcquireGPUCommandBuffer(device_->handle());
    if (!command_buffer_) {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return false;
    }

    // Acquire swapchain texture
    if (!SDL_AcquireGPUSwapchainTexture(command_buffer_, window_, &swapchain_texture_, &swapchain_width_, &swapchain_height_)) {
        SDL_Log("Failed to acquire swapchain texture: %s", SDL_GetError());
        return false;
    }

    // Swapchain texture can be null if window is minimised or occluded
    if (!swapchain_texture_) {
        SDL_CancelGPUCommandBuffer(command_buffer_);
        command_buffer_ = nullptr;
        return true;
    }

    return true;
}

void Renderer::clear(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    if (!command_buffer_ || !swapchain_texture_) {
        return;
    }

    SDL_GPUColorTargetInfo color_target{};
    color_target.texture = swapchain_texture_;
    color_target.clear_color = {
        r / 255.0f,
        g / 255.0f,
        b / 255.0f,
        a / 255.0f
    };
    color_target.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op = SDL_GPU_STOREOP_STORE;

    current_pass_ = SDL_BeginGPURenderPass(command_buffer_, &color_target, 1, nullptr);
}

void Renderer::present() {
    if (current_pass_) {
        SDL_EndGPURenderPass(current_pass_);
        current_pass_ = nullptr;
    }
    if (command_buffer_) {
        SDL_SubmitGPUCommandBuffer(command_buffer_);
        command_buffer_ = nullptr;
    }
    swapchain_texture_ = nullptr;
}

void Renderer::end_render_pass() noexcept {
    if (current_pass_) {
        SDL_EndGPURenderPass(current_pass_);
        current_pass_ = nullptr;
    }
}

void Renderer::render(ecs::World& world, double alpha) {
    render_commands_.clear();
    vertex_data_.clear();
    index_data_.clear();
    batches_.clear();

    collect_render_commands(world, alpha);

    if (render_commands_.empty()) {
        return;
    }

    sort_render_commands();

    const auto* texture_manager = world.get_resource<TextureManager>();
    if (!texture_manager) {
        return;
    }

    build_batches(*texture_manager);

    // End render pass before copy pass (SDL3 GPU requires passes to not overlap)
    bool had_render_pass = (current_pass_ != nullptr);
    if (current_pass_) {
        SDL_EndGPURenderPass(current_pass_);
        current_pass_ = nullptr;
    }

    upload_vertex_data();

    // Restart render pass for drawing (with LOAD to preserve clear color)
    if (had_render_pass && swapchain_texture_) {
        SDL_GPUColorTargetInfo color_target{};
        color_target.texture = swapchain_texture_;
        color_target.load_op = SDL_GPU_LOADOP_LOAD;
        color_target.store_op = SDL_GPU_STOREOP_STORE;
        current_pass_ = SDL_BeginGPURenderPass(command_buffer_, &color_target, 1, nullptr);
    }

    draw_batches(*texture_manager);
}

void Renderer::collect_render_commands(ecs::World& world, double alpha) {
    (void)alpha;

    world.each<Sprite, Transform>([this](ecs::Entity, const Sprite& sprite, const Transform& transform) {
        if (sprite.texture_id == INVALID_TEXTURE_ID) {
            return;
        }

        RenderCommand cmd;
        cmd.sprite = &sprite;
        cmd.transform = &transform;

        // Interpolate position using alpha for smooth rendering between fixed updates
        // For now we use current position; when previous position tracking is added,
        // this would be: lerp(prev_x, curr_x, alpha)
        cmd.interpolated_x = transform.x;
        cmd.interpolated_y = transform.y;

        // TODO: When previous transform is tracked, implement proper interpolation:
        // cmd.interpolated_x = prev_transform->x + (transform->x - prev_transform->x) * static_cast<float>(alpha);
        // cmd.interpolated_y = prev_transform->y + (transform->y - prev_transform->y) * static_cast<float>(alpha);

        cmd.layer = sprite.layer;
        render_commands_.push_back(cmd);
    });
}

void Renderer::sort_render_commands() {
    // Sort by layer first, then by texture (for batching), then by depth
    std::sort(render_commands_.begin(), render_commands_.end(),
        [](const RenderCommand& a, const RenderCommand& b) {
            if (a.sprite->layer != b.sprite->layer) {
                return a.sprite->layer < b.sprite->layer;
            }
            if (a.sprite->texture_id != b.sprite->texture_id) {
                return a.sprite->texture_id < b.sprite->texture_id;
            }
            return a.transform->z < b.transform->z;
        });
}

void Renderer::build_batches(const TextureManager& texture_manager) {
    if (render_commands_.empty()) {
        return;
    }

    // Build orthographic projection matrix (2D, origin at top-left)
    const float width = static_cast<float>(swapchain_width_);
    const float height = static_cast<float>(swapchain_height_);

    TextureID current_texture_id = INVALID_TEXTURE_ID;
    std::uint32_t batch_start_index = 0;

    for (const auto& cmd : render_commands_) {
        const auto* texture = texture_manager.get(cmd.sprite->texture_id);
        if (!texture || !texture->gpu_texture) {
            continue;
        }

        // Start new batch if texture changes
        if (cmd.sprite->texture_id != current_texture_id) {
            // Finish previous batch
            if (current_texture_id != INVALID_TEXTURE_ID && index_data_.size() > batch_start_index) {
                SpriteBatch batch;
                batch.texture_id = current_texture_id;
                batch.start_index = batch_start_index;
                batch.index_count = static_cast<std::uint32_t>(index_data_.size()) - batch_start_index;
                batches_.push_back(batch);
            }
            current_texture_id = cmd.sprite->texture_id;
            batch_start_index = static_cast<std::uint32_t>(index_data_.size());
        }

        // Get texture region or use full texture
        float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
        float sprite_width = static_cast<float>(texture->width);
        float sprite_height = static_cast<float>(texture->height);

        const auto* region = texture_manager.get_region(cmd.sprite->texture_id, std::to_string(cmd.sprite->region_index));
        if (region) {
            u0 = region->u0;
            v0 = region->v0;
            u1 = region->u1;
            v1 = region->v1;
            sprite_width = static_cast<float>(region->width);
            sprite_height = static_cast<float>(region->height);
        }

        // Apply flip
        if ((cmd.sprite->flip & SpriteFlip::Horizontal) == SpriteFlip::Horizontal) {
            std::swap(u0, u1);
        }
        if ((cmd.sprite->flip & SpriteFlip::Vertical) == SpriteFlip::Vertical) {
            std::swap(v0, v1);
        }

        // Calculate world-space quad corners (centered on position)
        const float half_w = (sprite_width * cmd.sprite->scale_x) * 0.5f;
        const float half_h = (sprite_height * cmd.sprite->scale_y) * 0.5f;

        // Apply camera transform
        const float screen_x = (cmd.interpolated_x - camera_x_) * camera_zoom_;
        const float screen_y = (cmd.interpolated_y - camera_y_) * camera_zoom_;
        const float scaled_half_w = half_w * camera_zoom_;
        const float scaled_half_h = half_h * camera_zoom_;

        // Handle rotation
        float cos_r = 1.0f, sin_r = 0.0f;
        if (cmd.sprite->rotation != 0.0f) {
            cos_r = std::cos(cmd.sprite->rotation);
            sin_r = std::sin(cmd.sprite->rotation);
        }

        // Quad corners relative to center
        const float corners[4][2] = {
            {-scaled_half_w, -scaled_half_h}, // top-left
            { scaled_half_w, -scaled_half_h}, // top-right
            { scaled_half_w,  scaled_half_h}, // bottom-right
            {-scaled_half_w,  scaled_half_h}  // bottom-left
        };

        // UV coordinates for each corner
        const float uvs[4][2] = {
            {u0, v0}, // top-left
            {u1, v0}, // top-right
            {u1, v1}, // bottom-right
            {u0, v1}  // bottom-left
        };

        const std::uint16_t base_vertex = static_cast<std::uint16_t>(vertex_data_.size());

        // Generate 4 vertices
        for (int i = 0; i < 4; ++i) {
            SpriteVertex vertex;

            // Apply rotation and translate to screen position
            const float rx = corners[i][0] * cos_r - corners[i][1] * sin_r;
            const float ry = corners[i][0] * sin_r + corners[i][1] * cos_r;
            vertex.x = screen_x + rx;
            vertex.y = screen_y + ry;

            vertex.u = uvs[i][0];
            vertex.v = uvs[i][1];
            vertex.r = cmd.sprite->r;
            vertex.g = cmd.sprite->g;
            vertex.b = cmd.sprite->b;
            vertex.a = cmd.sprite->a;

            vertex_data_.push_back(vertex);
        }

        // Generate 6 indices for 2 triangles (clockwise winding)
        index_data_.push_back(base_vertex + 0);
        index_data_.push_back(base_vertex + 1);
        index_data_.push_back(base_vertex + 2);
        index_data_.push_back(base_vertex + 0);
        index_data_.push_back(base_vertex + 2);
        index_data_.push_back(base_vertex + 3);
    }

    // Finish last batch
    if (current_texture_id != INVALID_TEXTURE_ID && index_data_.size() > batch_start_index) {
        SpriteBatch batch;
        batch.texture_id = current_texture_id;
        batch.start_index = batch_start_index;
        batch.index_count = static_cast<std::uint32_t>(index_data_.size()) - batch_start_index;
        batches_.push_back(batch);
    }
}

void Renderer::upload_vertex_data() {
    if (vertex_data_.empty() || index_data_.empty() || !command_buffer_) {
        return;
    }

    SDL_GPUDevice* gpu = device_->handle();

    const Uint32 vertex_size = static_cast<Uint32>(vertex_data_.size() * sizeof(SpriteVertex));
    const Uint32 index_size = static_cast<Uint32>(index_data_.size() * sizeof(std::uint16_t));

    // Create transfer buffer for vertex data
    SDL_GPUTransferBufferCreateInfo transfer_info{};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = vertex_size + index_size;

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &transfer_info);
    if (!transfer_buffer) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create transfer buffer: %s", SDL_GetError());
        return;
    }

    // Map and copy data
    void* mapped = SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false);
    if (!mapped) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to map transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
        return;
    }

    std::memcpy(mapped, vertex_data_.data(), vertex_size);
    std::memcpy(static_cast<std::uint8_t*>(mapped) + vertex_size, index_data_.data(), index_size);
    SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);

    // Upload to GPU buffers via copy pass
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer_);
    if (!copy_pass) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to begin copy pass: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
        return;
    }

    // Upload vertex buffer
    SDL_GPUTransferBufferLocation src_vertex{};
    src_vertex.transfer_buffer = transfer_buffer;
    src_vertex.offset = 0;

    SDL_GPUBufferRegion dst_vertex{};
    dst_vertex.buffer = vertex_buffer_;
    dst_vertex.offset = 0;
    dst_vertex.size = vertex_size;

    SDL_UploadToGPUBuffer(copy_pass, &src_vertex, &dst_vertex, false);

    // Upload index buffer
    SDL_GPUTransferBufferLocation src_index{};
    src_index.transfer_buffer = transfer_buffer;
    src_index.offset = vertex_size;

    SDL_GPUBufferRegion dst_index{};
    dst_index.buffer = index_buffer_;
    dst_index.offset = 0;
    dst_index.size = index_size;

    SDL_UploadToGPUBuffer(copy_pass, &src_index, &dst_index, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
}

void Renderer::draw_batches(const TextureManager& texture_manager) {
    if (batches_.empty() || !current_pass_) {
        return;
    }

    PipelineManager* pipeline_mgr = device_->pipeline_manager();
    if (!pipeline_mgr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "No pipeline manager available");
        return;
    }

    SDL_GPUGraphicsPipeline* sprite_pipeline = pipeline_mgr->get_core_pipeline(PipelineType::Sprite);
    if (!sprite_pipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Sprite pipeline not available");
        return;
    }

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(current_pass_, sprite_pipeline);

    // Bind vertex buffer
    SDL_GPUBufferBinding vertex_binding{};
    vertex_binding.buffer = vertex_buffer_;
    vertex_binding.offset = 0;
    SDL_BindGPUVertexBuffers(current_pass_, 0, &vertex_binding, 1);

    // Bind index buffer
    SDL_GPUBufferBinding index_binding{};
    index_binding.buffer = index_buffer_;
    index_binding.offset = 0;
    SDL_BindGPUIndexBuffer(current_pass_, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Create and upload camera uniform (orthographic projection)
    CameraData camera_data{};

    // Build orthographic projection matrix (column-major for GPU)
    // Maps LOGICAL coordinates to NDC [-1, 1]
    // Logical coordinates are mapped to the full swapchain (native resolution)
    const float width = static_cast<float>(logical_width_);
    const float height = static_cast<float>(logical_height_);
    const float left = 0.0f;
    const float right = width;
    const float top = 0.0f;
    const float bottom = height;
    const float near_plane = -1.0f;
    const float far_plane = 1.0f;

    // Orthographic projection matrix (column-major)
    std::memset(camera_data.projection, 0, sizeof(camera_data.projection));
    camera_data.projection[0] = 2.0f / (right - left);           // [0][0]
    camera_data.projection[5] = 2.0f / (top - bottom);           // [1][1]
    camera_data.projection[10] = -2.0f / (far_plane - near_plane); // [2][2]
    camera_data.projection[12] = -(right + left) / (right - left); // [3][0]
    camera_data.projection[13] = -(top + bottom) / (top - bottom); // [3][1]
    camera_data.projection[14] = -(far_plane + near_plane) / (far_plane - near_plane); // [3][2]
    camera_data.projection[15] = 1.0f;                           // [3][3]

    // Push camera uniform to vertex shader
    SDL_PushGPUVertexUniformData(command_buffer_, 0, &camera_data, sizeof(CameraData));

    // Draw each batch
    for (const auto& batch : batches_) {
        const auto* texture = texture_manager.get(batch.texture_id);
        if (!texture || !texture->gpu_texture) {
            continue;
        }

        // Bind texture and sampler
        SDL_GPUTextureSamplerBinding texture_sampler_binding{};
        texture_sampler_binding.texture = texture->gpu_texture;
        texture_sampler_binding.sampler = sampler_;
        SDL_BindGPUFragmentSamplers(current_pass_, 0, &texture_sampler_binding, 1);

        // Draw indexed primitives
        SDL_DrawGPUIndexedPrimitives(current_pass_, batch.index_count, 1, batch.start_index, 0, 0);
    }
}

void Renderer::execute_render_commands(const TextureManager& texture_manager) {
    // This method is now handled by build_batches, upload_vertex_data, and draw_batches
    (void)texture_manager;
}

} // namespace rendering
