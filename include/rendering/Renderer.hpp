#pragma once

#include "Sprite.hpp"
#include "Texture.hpp"
#include "TextureManager.hpp"
#include "PipelineTypes.hpp"
#include "../ecs/World.hpp"
#include <SDL3/SDL.h>
#include <vector>

namespace rendering {

// Forward declarations
class GPUDevice;
class PipelineManager;

// Batch rendering with SDL3 GPU API
class Renderer {
public:
    explicit Renderer(GPUDevice* device);
    explicit Renderer(GPUDevice* device, SDL_Window* window);
    ~Renderer();

    // Non-copyable, movable
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept;
    Renderer& operator=(Renderer&&) noexcept;

    // Set window (required before begin_frame if using single-arg constructor)
    void set_window(SDL_Window* window) noexcept { window_ = window; }

    // Begin frame - acquires swapchain texture
    [[nodiscard]] bool begin_frame();

    // Render all sprites with interpolation
    void render(ecs::World& world, double alpha);

    // Clear screen
    void clear(std::uint8_t r = 0, std::uint8_t g = 0, std::uint8_t b = 0, std::uint8_t a = 255);

    // Present frame
    void present();

    // Camera control
    void set_camera_position(float x, float y) noexcept {
        camera_x_ = x;
        camera_y_ = y;
    }

    void set_camera_zoom(float zoom) noexcept {
        camera_zoom_ = zoom;
    }

    [[nodiscard]] float camera_x() const noexcept { return camera_x_; }
    [[nodiscard]] float camera_y() const noexcept { return camera_y_; }
    [[nodiscard]] float camera_zoom() const noexcept { return camera_zoom_; }

    // Access GPU resources for UI rendering
    [[nodiscard]] SDL_GPUCommandBuffer* command_buffer() const noexcept { return command_buffer_; }
    [[nodiscard]] SDL_GPUTexture* swapchain_texture() const noexcept { return swapchain_texture_; }
    [[nodiscard]] Uint32 swapchain_width() const noexcept { return swapchain_width_; }
    [[nodiscard]] Uint32 swapchain_height() const noexcept { return swapchain_height_; }

    // End current render pass (call before UI rendering)
    void end_render_pass() noexcept;

private:
    struct RenderCommand {
        const Sprite* sprite;
        const Transform* transform;
        float interpolated_x;
        float interpolated_y;
        std::uint8_t layer;
    };

    // Batch represents sprites sharing the same texture
    struct SpriteBatch {
        TextureID texture_id{INVALID_TEXTURE_ID};
        std::uint32_t start_index{0};
        std::uint32_t index_count{0};
    };

    void collect_render_commands(ecs::World& world, double alpha);
    void sort_render_commands();
    void execute_render_commands(const TextureManager& texture_manager);
    void build_batches(const TextureManager& texture_manager);
    void upload_vertex_data();
    void draw_batches(const TextureManager& texture_manager);
    bool create_gpu_resources();
    void release_gpu_resources();

    GPUDevice* device_{nullptr};
    SDL_Window* window_{nullptr};
    SDL_GPUCommandBuffer* command_buffer_{nullptr};

    // GPU resources
    SDL_GPUBuffer* vertex_buffer_{nullptr};
    SDL_GPUBuffer* index_buffer_{nullptr};
    SDL_GPUSampler* sampler_{nullptr};
    SDL_GPURenderPass* current_pass_{nullptr};
    SDL_GPUTexture* current_texture_{nullptr};
    SDL_GPUTexture* swapchain_texture_{nullptr};
    Uint32 swapchain_width_{0};
    Uint32 swapchain_height_{0};
    std::vector<SpriteVertex> vertex_data_;
    std::vector<std::uint16_t> index_data_;
    std::vector<SpriteBatch> batches_;
    bool gpu_resources_created_{false};

    static constexpr size_t MAX_SPRITES = 10000;
    static constexpr size_t VERTEX_BUFFER_SIZE = MAX_SPRITES * 4 * sizeof(SpriteVertex);
    static constexpr size_t INDEX_BUFFER_SIZE = MAX_SPRITES * 6 * sizeof(std::uint16_t);
    std::vector<RenderCommand> render_commands_;

    float camera_x_{0.0f};
    float camera_y_{0.0f};
    float camera_zoom_{1.0f};
};

} // namespace rendering
