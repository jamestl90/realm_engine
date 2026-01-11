#include "../../include/rendering/Renderer.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include "../../include/rendering/TextureManager.hpp"
#include "../../include/rendering/Sprite.hpp"
#include "../../include/ecs/Component.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cassert>
#include <iostream>

namespace rendering {

Renderer::Renderer(GPUDevice* device)
    : device_(device)
    , window_(nullptr) {
    assert(device_ && "GPU device cannot be null");

    vertex_data_.reserve(MAX_SPRITES * 4);
    index_data_.reserve(MAX_SPRITES * 6);
    render_commands_.reserve(1000);
}

Renderer::Renderer(GPUDevice* device, SDL_Window* window)
    : device_(device)
    , window_(window) {
    assert(device_ && "GPU device cannot be null");
    assert(window_ && "Window cannot be null");

    vertex_data_.reserve(MAX_SPRITES * 4);
    index_data_.reserve(MAX_SPRITES * 6);
    render_commands_.reserve(1000);
}

Renderer::~Renderer() = default;

Renderer::Renderer(Renderer&& other) noexcept
    : device_(other.device_)
    , window_(other.window_)
    , command_buffer_(other.command_buffer_)
    , swapchain_texture_(other.swapchain_texture_)
    , swapchain_width_(other.swapchain_width_)
    , swapchain_height_(other.swapchain_height_)
    , vertex_data_(std::move(other.vertex_data_))
    , index_data_(std::move(other.index_data_))
    , render_commands_(std::move(other.render_commands_))
    , camera_x_(other.camera_x_)
    , camera_y_(other.camera_y_)
    , camera_zoom_(other.camera_zoom_) {
    other.device_ = nullptr;
    other.window_ = nullptr;
    other.command_buffer_ = nullptr;
    other.swapchain_texture_ = nullptr;
}

Renderer& Renderer::operator=(Renderer&& other) noexcept {
    if (this != &other) {
        device_ = other.device_;
        window_ = other.window_;
        command_buffer_ = other.command_buffer_;
        swapchain_texture_ = other.swapchain_texture_;
        swapchain_width_ = other.swapchain_width_;
        swapchain_height_ = other.swapchain_height_;
        vertex_data_ = std::move(other.vertex_data_);
        index_data_ = std::move(other.index_data_);
        render_commands_ = std::move(other.render_commands_);
        camera_x_ = other.camera_x_;
        camera_y_ = other.camera_y_;
        camera_zoom_ = other.camera_zoom_;

        other.device_ = nullptr;
        other.window_ = nullptr;
        other.command_buffer_ = nullptr;
        other.swapchain_texture_ = nullptr;
    }
    return *this;
}

bool Renderer::begin_frame() {
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
        // Cancel the command buffer since we can't render
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

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = swapchain_texture_;
    colorTarget.clear_color = {
        r / 255.0f,
        g / 255.0f, 
        b / 255.0f,
        a / 255.0f
    };
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    current_pass_ = SDL_BeginGPURenderPass(command_buffer_, &colorTarget, 1, nullptr);
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
    // Reset swapchain texture for next frame
    swapchain_texture_ = nullptr;
}

void Renderer::render(ecs::World& world, double alpha) {
    render_commands_.clear();
    collect_render_commands(world, alpha);
    sort_render_commands();
    const auto* texture_manager = world.get_resource<TextureManager>();
    if (!texture_manager) {
        return;
    }
    execute_render_commands(*texture_manager);

    // TODO: UI rendering - Render UI elements after world sprites, in screen-space coordinates.
    // UI should be rendered last to appear on top of all game content.
}

void Renderer::collect_render_commands(ecs::World& world, double alpha) {
    (void)alpha;

    auto* sprite_array = world.get_component_array<Sprite>();
    auto* transform_array = world.get_component_array<Transform>();
    if (!sprite_array || !transform_array) {
        return;
    }

    const auto& entities = sprite_array->entity_data();
    const auto entity_count = entities.size();

    // TODO: Culling - Perform frustum/viewport culling here before adding to render_commands_.
    // Skip entities whose bounding boxes are entirely outside the camera view to reduce draw calls.

    for (std::size_t i = 0; i < entity_count; ++i) {
        const auto entity_id = entities[i];
        const auto* sprite = sprite_array->get(entity_id);
        const auto* transform = transform_array->get(entity_id);

        if (!sprite || !transform || sprite->texture_id == INVALID_TEXTURE_ID) {
            continue;
        }

        RenderCommand cmd;
        cmd.sprite = sprite;
        cmd.transform = transform;
        cmd.interpolated_x = transform->x;
        cmd.interpolated_y = transform->y;
        cmd.layer = sprite->layer;
        render_commands_.push_back(cmd);
    }
}

void Renderer::sort_render_commands() {
    std::sort(render_commands_.begin(), render_commands_.end(),
        [](const RenderCommand& a, const RenderCommand& b) {
            if (a.sprite->layer != b.sprite->layer) {
                return a.sprite->layer < b.sprite->layer;
            }
            return a.transform->z < b.transform->z;
        });
}

void Renderer::execute_render_commands(const TextureManager& texture_manager) {
    if (!current_pass_) {
        return;
    }

    SDL_FRect dst_rect;
    SDL_FRect src_rect;
    
    for (const auto& cmd : render_commands_) {
        const auto* texture = texture_manager.get(cmd.sprite->texture_id);
        if (!texture || !texture->gpu_texture) {
            continue;
        }

        dst_rect.x = (cmd.interpolated_x - camera_x_) * camera_zoom_;
        dst_rect.y = (cmd.interpolated_y - camera_y_) * camera_zoom_;
        
        const auto* region = texture_manager.get_region(cmd.sprite->texture_id, std::to_string(cmd.sprite->region_index));
        if (region) {
            src_rect.x = region->u0 * static_cast<float>(texture->width);
            src_rect.y = region->v0 * static_cast<float>(texture->height);
            src_rect.w = (region->u1 - region->u0) * static_cast<float>(texture->width);
            src_rect.h = (region->v1 - region->v0) * static_cast<float>(texture->height);
            dst_rect.w = static_cast<float>(region->width) * camera_zoom_ * cmd.sprite->scale_x;
            dst_rect.h = static_cast<float>(region->height) * camera_zoom_ * cmd.sprite->scale_y;
        } else {
            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.w = static_cast<float>(texture->width);
            src_rect.h = static_cast<float>(texture->height);
            dst_rect.w = static_cast<float>(texture->width) * camera_zoom_ * cmd.sprite->scale_x;
            dst_rect.h = static_cast<float>(texture->height) * camera_zoom_ * cmd.sprite->scale_y;
        }

        dst_rect.x -= dst_rect.w * 0.5f;
        dst_rect.y -= dst_rect.h * 0.5f;

        // End current render pass before blitting
        if (current_pass_) {
            SDL_EndGPURenderPass(current_pass_);
            current_pass_ = nullptr;
        }

        // Create blit info
        SDL_GPUBlitInfo blit_info = {};
        blit_info.source.texture = texture->gpu_texture;
        blit_info.source.mip_level = 0;
        blit_info.source.layer_or_depth_plane = 0;
        blit_info.source.x = static_cast<Uint32>(src_rect.x);
        blit_info.source.y = static_cast<Uint32>(src_rect.y);
        blit_info.source.w = static_cast<Uint32>(src_rect.w);
        blit_info.source.h = static_cast<Uint32>(src_rect.h);
        blit_info.destination.texture = swapchain_texture_;
        blit_info.destination.mip_level = 0;
        blit_info.destination.layer_or_depth_plane = 0;
        blit_info.destination.x = static_cast<Uint32>(dst_rect.x);
        blit_info.destination.y = static_cast<Uint32>(dst_rect.y);
        blit_info.destination.w = static_cast<Uint32>(dst_rect.w);
        blit_info.destination.h = static_cast<Uint32>(dst_rect.h);
        blit_info.load_op = SDL_GPU_LOADOP_LOAD;
        blit_info.clear_color = {0.0f, 0.0f, 0.0f, 0.0f};
        blit_info.flip_mode = static_cast<SDL_FlipMode>(cmd.sprite->flip);
        blit_info.filter = SDL_GPU_FILTER_LINEAR;
        blit_info.cycle = false;

        // Blit the texture
        SDL_BlitGPUTexture(command_buffer_, &blit_info);
    }
}

} // namespace rendering
