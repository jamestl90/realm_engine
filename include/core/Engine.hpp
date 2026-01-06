#pragma once

#include "Time.hpp"
#include "../ecs/World.hpp"
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

private:
    void process_events();
    void fixed_update(float dt);
    void render(double alpha);

    SDL_Window* window_{nullptr};
    ecs::World world_;
    Time time_;
    
    bool running_{false};
    bool initialized_{false};
    
    // Fixed timestep accumulator
    double accumulator_{0.0};
    static constexpr double max_frame_time_{0.25}; // Cap at 250ms to prevent spiral of death
};

} // namespace core
