#include "../../include/rendering/Renderer.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include "../../include/rendering/TextureManager.hpp"
#include "../../include/rendering/Sprite.hpp"
#include "../../include/ecs/Component.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cassert>

namespace rendering {

Renderer::Renderer(GPUDevice* device)
    : device_(device) {
    assert(device_ && "GPU device cannot be null");
    command_buffer_ = SDL_AcquireGPUCommandBuffer(device_->handle());

    vertex_data_.reserve(MAX_SPRITES * 4);
    index_data_.reserve(MAX_SPRITES * 6);
    render_commands_.reserve(1000);
}

Renderer::~Renderer() = default;

Renderer::Renderer(Renderer&& other) noexcept
    : device_(other.device_)
    , vertex_data_(std::move(other.vertex_data_))
    , index_data_(std::move(other.index_data_))
    , render_commands_(std::move(other.render_commands_))
    , camera_x_(other.camera_x_)
    , camera_y_(other.camera_y_)
    , camera_zoom_(other.camera_zoom_) {
    other.device_ = nullptr;
}

Renderer& Renderer::operator=(Renderer&& other) noexcept {
    if (this != &other) {
        device_ = other.device_;
        vertex_data_ = std::move(other.vertex_data_);
        index_data_ = std::move(other.index_data_);
        render_commands_ = std::move(other.render_commands_);
        camera_x_ = other.camera_x_;
        camera_y_ = other.camera_y_;
        camera_zoom_ = other.camera_zoom_;

        other.device_ = nullptr;
    }
    return *this;
}

void Renderer::clear(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    if (!command_buffer_) return;

    SDL_GPUColorTargetInfo colorTarget = {};
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
        command_buffer_ = SDL_AcquireGPUCommandBuffer(device_->handle());
    }
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

        // Set color modulation
        SDL_FColor color = {
            cmd.sprite->r / 255.0f,
            cmd.sprite->g / 255.0f,
            cmd.sprite->b / 255.0f,
            cmd.sprite->a / 255.0f
        };
        SDL_SetGPUBlendConstants(current_pass_, color);

        // Calculate rotation center
        float center_x = dst_rect.x + dst_rect.w/2;
        float center_y = dst_rect.y + dst_rect.h/2;

        // Create blit info
        SDL_GPUBlitInfo blit_info = {};
        blit_info.source.texture = texture->gpu_texture;
        blit_info.source.x = src_rect.x;
        blit_info.source.y = src_rect.y;
        blit_info.source.w = src_rect.w;
        blit_info.source.h = src_rect.h;
        blit_info.destination.texture = nullptr; // Will be set to render target
        blit_info.destination.x = dst_rect.x;
        blit_info.destination.y = dst_rect.y;
        blit_info.destination.w = dst_rect.w;
        blit_info.destination.h = dst_rect.h;
        blit_info.flip_mode = static_cast<SDL_FlipMode>(cmd.sprite->flip);
        blit_info.filter = SDL_GPU_FILTER_LINEAR;
        blit_info.cycle = false;

        // Draw the texture
        SDL_BlitGPUTexture(command_buffer_, &blit_info);
    }
}

} // namespace rendering
