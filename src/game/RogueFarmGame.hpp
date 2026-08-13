#pragma once

#include "../../include/core/Game.hpp"
#include "../../include/ecs/Entity.hpp"
#include "../../include/rendering/Texture.hpp"

#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
#include "GreaterRealmDebugPanel.hpp"
#include "../../include/procgen/TerrainConstraints.hpp"
#endif

namespace game {

// Main game class for Rogue Farm
class RogueFarmGame : public core::Game {
public:
    RogueFarmGame() = default;
    ~RogueFarmGame() override = default;

    // Lifecycle hooks
    void on_startup(core::Engine& engine) override;
    void on_update(core::Engine& engine, double dt) override;
    void on_shutdown(core::Engine& engine) override;
    void on_resized(core::Engine& engine, int width, int height) override;

private:
#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    bool regenerate_procgen_debug_map(core::Engine& engine);
    bool refresh_procgen_debug_view(core::Engine& engine);
    bool replace_procgen_debug_texture(core::Engine& engine, const procgen::DebugImage& image);
#endif

    ecs::Entity m_test_entity;
    rendering::TextureID m_test_texture{rendering::INVALID_TEXTURE_ID};

#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    procgen::GreaterRealmGeneratorSettings m_procgen_settings{};
    procgen::TerrainConstraintField m_procgen_constraints{64, 64};
    procgen::GreaterRealmMap m_procgen_map;
    procgen::GreaterRealmDebugOptions m_procgen_debug_options;
    GreaterRealmDebugPanel m_procgen_debug_panel;
#endif
};

} // namespace game
