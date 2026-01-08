#include "RogueFarmGame.hpp"
#include "../../include/core/Engine.hpp"
#include "../../include/rendering/Sprite.hpp"
#include <SDL3/SDL.h>

namespace game {

void RogueFarmGame::on_startup(core::Engine& engine) {
    SDL_Log("RogueFarmGame starting up...");
    
    SDL_Surface* surface = SDL_CreateSurface(24, 24, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        SDL_Log("Failed to create surface: %s", SDL_GetError());
        return;
    }

    SDL_Color red = {255, 0, 0, 255};
    Uint32 pixel = SDL_MapSurfaceRGBA(surface, red.r, red.g, red.b, red.a);
    if (!SDL_FillSurfaceRect(surface, nullptr, pixel)) {
        SDL_Log("Failed to fill surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return;
    }

    auto* texture_manager = engine.texture_manager();
    if (!texture_manager) {
        SDL_Log("Texture manager is null!");
        SDL_DestroySurface(surface);
        return;
    }

    m_test_texture = texture_manager->create_from_surface(surface);
    if (m_test_texture == rendering::INVALID_TEXTURE_ID) {
        SDL_Log("Failed to create texture from surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return;
    }

    SDL_DestroySurface(surface);

    m_test_entity = engine.world().create_entity();
    
    int window_width = 0;
    int window_height = 0;
    SDL_GetWindowSize(engine.window(), &window_width, &window_height);
    
    rendering::Transform transform;
    transform.x = static_cast<float>(window_width) / 2.0f;
    transform.y = static_cast<float>(window_height) / 2.0f;
    transform.z = 0.0f;
    engine.world().add_component(m_test_entity, transform);

    rendering::Sprite sprite;
    sprite.texture_id = m_test_texture;
    sprite.layer = 0;
    sprite.r = 255;
    sprite.g = 255;
    sprite.b = 255;
    sprite.a = 255;
    engine.world().add_component(m_test_entity, sprite);

    SDL_Log("RogueFarmGame startup complete");
}

void RogueFarmGame::on_update(core::Engine& engine, double dt) {
    (void)engine;
    (void)dt;
}

void RogueFarmGame::on_render(core::Engine& engine, double alpha) {
    (void)engine;
    (void)alpha;
}

void RogueFarmGame::on_shutdown(core::Engine& engine) {
    if (m_test_texture != rendering::INVALID_TEXTURE_ID) {
        engine.texture_manager()->destroy(m_test_texture);
        m_test_texture = rendering::INVALID_TEXTURE_ID;
    }

    if (m_test_entity.is_valid()) {
        engine.world().destroy_entity(m_test_entity);
    }

    SDL_Log("RogueFarmGame shutting down...");
}

} // namespace game
