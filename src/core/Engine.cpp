#include "core/Engine.hpp"
#include <SDL3/SDL.h>

namespace core {

Engine::Engine() = default;

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize(const char* title, int width, int height) {
    if (initialized_) {
        return true;
    }

    // Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }

    // Create window
    window_ = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
    if (!window_) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    // Initialize time system
    time_.reset();

    initialized_ = true;
    SDL_Log("Engine initialized successfully");
    return true;
}

void Engine::shutdown() noexcept {
    if (!initialized_) {
        return;
    }

    // Clear ECS world
    world_.clear();

    // Destroy window
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    // Shutdown SDL
    SDL_Quit();

    initialized_ = false;
    SDL_Log("Engine shutdown complete");
}

void Engine::run() {
    if (!initialized_) {
        SDL_Log("Engine not initialized. Call initialize() first.");
        return;
    }

    running_ = true;
    accumulator_ = 0.0;
    time_.reset();

    SDL_Log("Entering main loop...");

    // Main game loop with fixed timestep
    while (running_) {
        time_.tick();

        const double frame_time = time_.unscaled_delta_time();
        const double clamped_frame_time = frame_time < max_frame_time_ ? frame_time : max_frame_time_;

        accumulator_ += clamped_frame_time;

        // Process input events
        process_events();

        // Fixed timestep updates
        const double fixed_dt = time_.fixed_delta_time();
        while (accumulator_ >= fixed_dt) {
            if (!time_.is_paused()) {
                fixed_update(static_cast<float>(fixed_dt));
            }
            accumulator_ -= fixed_dt;
        }

        // Calculate interpolation alpha
        const double alpha = accumulator_ / fixed_dt;

        // Render with interpolation
        render(alpha);
    }
}

void Engine::process_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                quit();
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.scancode == SDL_SCANCODE_ESCAPE) {
                    quit();
                }
                break;
        }
    }
}

void Engine::fixed_update(float dt) {
    // Update all ECS systems with fixed timestep
    world_.update(dt);
}

void Engine::render(double alpha) {
    // Interpolation factor for smooth rendering between fixed updates
    // alpha ranges from 0.0 to 1.0
    (void)alpha; // Suppress unused parameter warning for now

    // Rendering will be implemented when we add the Renderer class
    // For now, we just need to keep the window responsive
    
    // Small delay to prevent CPU spinning
    SDL_Delay(1);
}

} // namespace core
