#pragma once

#include "Time.hpp"
#include "Game.hpp"
#include "../ecs/World.hpp"
#include "../rendering/Renderer.hpp"
#include "../rendering/TextureManager.hpp"
#include "../assets/AssetManager.hpp"
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

    // Access subsystems
    [[nodiscard]] ecs::World& world() noexcept { return world_; }
    [[nodiscard]] const ecs::World& world() const noexcept { return world_; }
    [[nodiscard]] Time& time() noexcept { return time_; }
    [[nodiscard]] const Time& time() const noexcept { return time_; }
    [[nodiscard]] SDL_Window* window() noexcept { return window_; }
    [[nodiscard]] rendering::Renderer* renderer() noexcept { return renderer_.get(); }
    [[nodiscard]] const rendering::Renderer* renderer() const noexcept { return renderer_.get(); }
    
    [[nodiscard]] rendering::TextureManager* texture_manager() noexcept { return texture_manager_.get(); }
    [[nodiscard]] const rendering::TextureManager* texture_manager() const noexcept { return texture_manager_.get(); }
    
    [[nodiscard]] assets::AssetManager* asset_manager() noexcept { return asset_manager_.get(); }
    [[nodiscard]] const assets::AssetManager* asset_manager() const noexcept { return asset_manager_.get(); }

private:
    void process_events();
    void fixed_update(float dt);
    void update(double dt);
    void render(double alpha);

    SDL_Window* window_{nullptr};
    std::unique_ptr<rendering::GPUDevice> gpu_device_;
    
    // Core subsystems
    ecs::World world_;
    Time time_;
    
    // Rendering
    std::unique_ptr<rendering::Renderer> renderer_;
    std::unique_ptr<rendering::TextureManager> texture_manager_;
    
    // Assets
    std::unique_ptr<assets::AssetManager> asset_manager_;
    
    // Game instance
    std::unique_ptr<Game> game_;

    bool shutdown_ = false;
    
    bool running_{false};
    bool initialized_{false};
    
    // Fixed timestep accumulator
    double accumulator_{0.0};
    static constexpr double max_frame_time_{0.25}; // Cap at 250ms to prevent spiral of death
};

} // namespace core
