#pragma once

#include "../../include/procgen/GreaterRealmDebug.hpp"
#include "../../include/procgen/TerrainConstraints.hpp"
#include <functional>
#include <memory>

namespace ui {
class Button;
class TextBlock;
class UIElement;
}

namespace game {

class GreaterRealmDebugPanel {
public:
    using RegenerateCallback = std::function<void()>;
    using PaintConstraintCallback = std::function<void(procgen::TerrainConstraintTool, float, float)>;
    using ClearConstraintsCallback = std::function<void()>;
    using ViewChangedCallback = std::function<void()>;

    [[nodiscard]] std::unique_ptr<ui::UIElement> build(
        procgen::GreaterRealmGeneratorSettings& settings,
        procgen::GreaterRealmDebugOptions& debug_options,
        const procgen::GreaterRealmMap& map,
        RegenerateCallback on_regenerate,
        PaintConstraintCallback on_paint_constraint,
        ClearConstraintsCallback on_clear_constraints,
        ViewChangedCallback on_view_changed
    );

    void update(const procgen::GreaterRealmMap& map);

private:
    void regenerate();
    void notify_view_changed();
    void update_overlay_buttons();

    procgen::GreaterRealmGeneratorSettings* m_settings{nullptr};
    procgen::GreaterRealmDebugOptions* m_debug_options{nullptr};
    RegenerateCallback m_on_regenerate;
    PaintConstraintCallback m_on_paint_constraint;
    ClearConstraintsCallback m_on_clear_constraints;
    ViewChangedCallback m_on_view_changed;
    float m_constraint_x{0.5f};
    float m_constraint_y{0.5f};
    ui::TextBlock* m_seed_text{nullptr};
    ui::TextBlock* m_sea_text{nullptr};
    ui::TextBlock* m_island_bias_text{nullptr};
    ui::TextBlock* m_coastline_noise_text{nullptr};
    ui::TextBlock* m_base_elevation_text{nullptr};
    ui::TextBlock* m_mountain_text{nullptr};
    ui::TextBlock* m_peak_spacing_text{nullptr};
    ui::TextBlock* m_peak_radius_text{nullptr};
    ui::TextBlock* m_peak_jaggedness_text{nullptr};
    ui::TextBlock* m_ridge_text{nullptr};
    ui::TextBlock* m_valley_text{nullptr};
    ui::TextBlock* m_noise_text{nullptr};
    ui::TextBlock* m_ocean_depth_text{nullptr};
    ui::TextBlock* m_channel_threshold_text{nullptr};
    ui::TextBlock* m_constraint_x_text{nullptr};
    ui::TextBlock* m_constraint_y_text{nullptr};
    ui::TextBlock* m_coverage_text{nullptr};
    ui::TextBlock* m_terrain_text{nullptr};
    ui::TextBlock* m_hydrology_text{nullptr};
    ui::Button* m_coastline_button{nullptr};
    ui::Button* m_peaks_button{nullptr};
    ui::Button* m_rivers_button{nullptr};
    ui::Button* m_drainage_button{nullptr};
};

} // namespace game
