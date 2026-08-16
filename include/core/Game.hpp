#pragma once

#include <SDL3/SDL_events.h>

namespace core {

class Engine;

// Base class for host application logic
// Inherit from this and implement the virtual methods
class Game {
public:
    virtual ~Game() = default;

    // Called once after engine initialisation, before the main loop
    // Use this to load assets, register ECS systems, create initial entities
    virtual void on_startup(Engine& engine) = 0;

    // Called once per frame (variable timestep), after fixed updates
    // Use for input handling, camera updates, UI, etc.
    virtual void on_update(Engine& engine, double dt) { (void)engine; (void)dt; }

    // Called for every SDL event after retained UI has had the first chance to consume it.
    virtual void on_event(Engine& engine, const SDL_Event& event, bool ui_consumed) {
        (void)engine;
        (void)event;
        (void)ui_consumed;
    }

    // Called once per frame after ECS rendering and before retained UI rendering
    // Default implementation does nothing; engine handles ECS and retained UI rendering
    virtual void on_render(Engine& engine, double alpha) { (void)engine; (void)alpha; }

    // Called once when the engine is shutting down
    // Use for cleanup, saving state, etc.
    virtual void on_shutdown(Engine& engine) { (void)engine; }

    virtual void on_resized(Engine& engine, int width, int height) { (void)engine; (void)width; (void)height; }
};

} // namespace core
