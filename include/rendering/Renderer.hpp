#pragma once

#include "Sprite.hpp"
#include "Texture.hpp"
#include "../ecs/World.hpp"
#include <SDL3/SDL.h>
#include <vector>

namespace rendering {

// Batch rendering with SDL3 GPU API
class Renderer {
public:
    explicit Renderer(SDL_Renderer* renderer);
    ~Renderer();

    // Non-copyable, movable
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept;
    Renderer& operator=(Renderer&&) noexcept;

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

private:
    struct RenderCommand {
        const Sprite* sprite;
        const Transform* transform;
        float interpolated_x;
        float interpolated_y;
        std::uint8_t layer;
    };

    void collect_render_commands(ecs::World& world, double alpha);
    void sort_render_commands();
    void execute_render_commands(const TextureManager& texture_manager);

    SDL_Renderer* renderer_{nullptr};
    std::vector<RenderCommand> render_commands_;
    
    float camera_x_{0.0f};
    float camera_y_{0.0f};
    float camera_zoom_{1.0f};
};

} // namespace rendering
