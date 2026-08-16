#pragma once

#include "../../include/core/Game.hpp"
#include "../../include/ecs/Entity.hpp"
#include "../../include/rendering/Texture.hpp"

#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
#include "GreaterRealmDebugPanel.hpp"
#include "../../include/procgen/TerrainConstraintPainting.hpp"
#include "../../include/procgen/TerrainConstraints.hpp"
#endif

namespace game {

class TestApp : public core::Game {
public:
    TestApp() = default;
    ~TestApp() override = default;

    void on_startup(core::Engine& engine) override;
    void on_update(core::Engine& engine, double dt) override;
    void on_event(core::Engine& engine, const SDL_Event& event, bool ui_consumed) override;
    void on_shutdown(core::Engine& engine) override;
    void on_resized(core::Engine& engine, int width, int height) override;

private:
#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    bool regenerate_procgen_debug_map(core::Engine& engine, bool force_full = false);
    bool refresh_procgen_debug_view(core::Engine& engine);
    bool upload_procgen_debug_texture(core::Engine& engine, const procgen::DebugImage& image);
    bool refresh_procgen_terrain_mesh(core::Engine& engine, const procgen::DebugImage& image);
    void apply_procgen_presentation(core::Engine& engine) noexcept;
    [[nodiscard]] procgen::TerrainPreviewBounds procgen_preview_bounds(core::Engine& engine) const noexcept;
    void apply_procgen_paint_sample(const procgen::TerrainConstraintPaintSample& sample) noexcept;
#endif

    ecs::Entity m_test_entity;
    rendering::TextureID m_test_texture{rendering::INVALID_TEXTURE_ID};

#if defined(REALM_ENABLE_PROCGEN_DEBUG_VIEW)
    procgen::GreaterRealmGeneratorSettings m_procgen_settings{};
    procgen::TerrainConstraintField m_procgen_constraints{64, 64};
    procgen::GreaterRealmGenerationCache m_procgen_generation_cache;
    procgen::GreaterRealmMap m_procgen_map;
    procgen::GreaterRealmDebugOptions m_procgen_debug_options;
    GreaterRealmPresentationSettings m_procgen_presentation;
    procgen::TerrainConstraintBrushSettings m_procgen_brush_settings;
    procgen::TerrainConstraintPaintSession m_procgen_paint_session;
    GreaterRealmDebugPanel m_procgen_debug_panel;
    bool m_procgen_paint_dirty{false};
#endif
};

} // namespace game
