#pragma once

#include "Time.hpp"
#include "Game.hpp"
#include "../ecs/World.hpp"
#include "../rendering/Renderer.hpp"
#include "../rendering/TerrainRenderer.hpp"
#include "../rendering/TextureManager.hpp"
#include "../rendering/FontManager.hpp"
#include "../rendering/UIRenderer.hpp"
#include "../assets/AssetManager.hpp"
#include "../ui/UIManager.hpp"
#include <memory>
#include <SDL3/SDL.h>

namespace core {

// Main engine class with fixed timestep loop
class Engine {
public:
    Engine();
    ~Engine();

    // Non-copyable, non-movable
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    // Initialize engine and subsystems
    [[nodiscard]] bool initialize(const char* title, int width, int height);

    // Shutdown engine
    void shutdown() noexcept;

    // Set the game instance (must be called before run())
    void set_game(std::unique_ptr<Game> game) noexcept;

    // Main game loop
    void run();

    // Request exit
    void quit() noexcept { running_ = false; }

    void resize_window(int width, int height);

    // Access subsystems
    [[nodiscard]] ecs::World& world() noexcept { return world_; }
    [[nodiscard]] const ecs::World& world() const noexcept { return world_; }
    [[nodiscard]] Time& time() noexcept { return time_; }
    [[nodiscard]] const Time& time() const noexcept { return time_; }
    [[nodiscard]] SDL_Window* window() noexcept { return window_; }
    [[nodiscard]] rendering::Renderer* renderer() noexcept { return renderer_.get(); }
    [[nodiscard]] const rendering::Renderer* renderer() const noexcept { return renderer_.get(); }
    [[nodiscard]] rendering::TerrainRenderer* terrain_renderer() noexcept { return terrain_renderer_.get(); }
    [[nodiscard]] const rendering::TerrainRenderer* terrain_renderer() const noexcept { return terrain_renderer_.get(); }
    
    [[nodiscard]] rendering::TextureManager* texture_manager() noexcept { return texture_manager_.get(); }
    [[nodiscard]] const rendering::TextureManager* texture_manager() const noexcept { return texture_manager_.get(); }
    
    [[nodiscard]] assets::AssetManager* asset_manager() noexcept { return asset_manager_.get(); }
    [[nodiscard]] const assets::AssetManager* asset_manager() const noexcept { return asset_manager_.get(); }

    [[nodiscard]] rendering::UIRenderer* ui_renderer() noexcept { return ui_renderer_.get(); }
    [[nodiscard]] const rendering::UIRenderer* ui_renderer() const noexcept { return ui_renderer_.get(); }

    [[nodiscard]] rendering::FontManager* font_manager() noexcept { return font_manager_.get(); }
    [[nodiscard]] const rendering::FontManager* font_manager() const noexcept { return font_manager_.get(); }

    [[nodiscard]] ui::UIManager& ui_manager() noexcept { return ui_manager_; }
    [[nodiscard]] const ui::UIManager& ui_manager() const noexcept { return ui_manager_; }

private:
    void process_events();
    void fixed_update(float dt);
    void update(double dt);
    void render(double alpha);
    void render_ui();

    SDL_Window* window_{nullptr};
    std::unique_ptr<rendering::GPUDevice> gpu_device_;
    
    // Core subsystems
    ecs::World world_;
    Time time_;
    
    // Rendering
    std::unique_ptr<rendering::Renderer> renderer_;
    std::unique_ptr<rendering::TerrainRenderer> terrain_renderer_;
    std::unique_ptr<rendering::TextureManager> texture_manager_;
    std::unique_ptr<rendering::FontManager> font_manager_;
    std::unique_ptr<rendering::UIRenderer> ui_renderer_;
    
    // Assets
    std::unique_ptr<assets::AssetManager> asset_manager_;

    // UI
    ui::UIManager ui_manager_;
    
    // Game instance
    std::unique_ptr<Game> game_;

    bool shutdown_ = false;
    
    bool running_{false};
    bool initialized_{false};
    
    // Fixed timestep accumulator
    double accumulator_{0.0};
    static constexpr double max_frame_time_{0.25}; // Cap at 250ms to prevent spiral of death

public:
    // Logical resolution constants
    static constexpr int LOGICAL_W = 1920;
    static constexpr int LOGICAL_H = 1080;

    // Handle window resize
    void on_resize(int pixel_w, int pixel_h);

    // Convert screen (pixel) coordinates to logical coordinates
    // Maps full window to logical coordinate space
    [[nodiscard]] bool screen_to_logical(float screen_x, float screen_y,
                                          float& logical_x, float& logical_y) const noexcept;

    // Convert logical coordinates to screen (pixel) coordinates
    void logical_to_screen(float logical_x, float logical_y,
                           float& screen_x, float& screen_y) const noexcept;

    // Get current window dimensions
    [[nodiscard]] int window_width() const noexcept { return window_width_; }
    [[nodiscard]] int window_height() const noexcept { return window_height_; }

private:
    int window_width_{LOGICAL_W};
    int window_height_{LOGICAL_H};
};

} // namespace core
