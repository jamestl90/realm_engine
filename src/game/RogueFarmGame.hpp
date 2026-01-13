#pragma once

#include "../../include/core/Game.hpp"
#include "../../include/ecs/Entity.hpp"
#include "../../include/rendering/Texture.hpp"

namespace game {

// Main game class for Rogue Farm
class RogueFarmGame : public core::Game {
public:
    RogueFarmGame() = default;
    ~RogueFarmGame() override = default;

    // Lifecycle hooks
    void on_startup(core::Engine& engine) override;
    void on_update(core::Engine& engine, double dt) override;
    void on_render(core::Engine& engine, double alpha) override;
    void on_shutdown(core::Engine& engine) override;
    void on_resized(core::Engine& engine, int width, int height) override;

private:
    ecs::Entity m_test_entity;
    rendering::TextureID m_test_texture{rendering::INVALID_TEXTURE_ID};
};

} // namespace game
