#pragma once

#include "../../include/core/Game.hpp"
#include "../../include/ecs/Entity.hpp"
#include "../../include/rendering/Texture.hpp"

#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
#include "../../include/procgen/GreaterRealm.hpp"
#endif

namespace ui {
class TextBlock;
}

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
#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
    bool regenerate_procgen_debug_map(core::Engine& engine);
    void create_procgen_debug_ui(core::Engine& engine, const procgen::GreaterRealmMap& map);
    void update_procgen_debug_text(const procgen::GreaterRealmMap& map);
#endif

    ecs::Entity m_test_entity;
    rendering::TextureID m_test_texture{rendering::INVALID_TEXTURE_ID};

#if defined(RFD_ENABLE_PROCGEN_DEBUG_VIEW)
    procgen::GreaterRealmGeneratorSettings m_procgen_settings{};
    ui::TextBlock* m_procgen_seed_text{nullptr};
    ui::TextBlock* m_procgen_sea_text{nullptr};
    ui::TextBlock* m_procgen_mountain_text{nullptr};
    ui::TextBlock* m_procgen_ridge_text{nullptr};
    ui::TextBlock* m_procgen_valley_text{nullptr};
    ui::TextBlock* m_procgen_noise_text{nullptr};
    ui::TextBlock* m_procgen_coverage_text{nullptr};
    ui::TextBlock* m_procgen_terrain_text{nullptr};
#endif
};

} // namespace game
