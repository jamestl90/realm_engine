#pragma once

#include "../../include/procgen/Climate.hpp"
#include "../../include/procgen/GreaterRealmDebug.hpp"
#include "../../include/procgen/TerrainConstraintPainting.hpp"
#include "../../include/procgen/TerrainConstraints.hpp"
#include <cstdint>
#include <functional>
#include <memory>

namespace ui {
class Button;
class Slider;
class TextBlock;
class UIElement;
}

namespace game {

inline constexpr float GREATER_REALM_DEBUG_PANEL_WIDTH = 620.0f;

enum class GreaterRealmPresentationMode : std::uint8_t {
    Flat,
    Tilted3D
};

struct GreaterRealmPresentationSettings {
    GreaterRealmPresentationMode mode{GreaterRealmPresentationMode::Flat};
    float elevation_scale{100.0f};
};

class GreaterRealmDebugPanel {
public:
    using RegenerateCallback = std::function<void(bool)>;
    using ToolChangedCallback = std::function<void(procgen::TerrainConstraintTool)>;
    using BrushSettingsChangedCallback = std::function<void(procgen::TerrainConstraintBrushSettings)>;
    using ClearConstraintsCallback = std::function<void()>;
    using ViewChangedCallback = std::function<void()>;
    using PresentationChangedCallback = std::function<void()>;

    [[nodiscard]] std::unique_ptr<ui::UIElement> build(
        procgen::GreaterRealmGeneratorSettings& settings,
        procgen::GreaterRealmDebugOptions& debug_options,
        GreaterRealmPresentationSettings& presentation_settings,
        procgen::TerrainConstraintBrushSettings& brush_settings,
        const procgen::GreaterRealmMap& map,
        const procgen::TemperatureNormalSummary& temperature_summary,
        RegenerateCallback on_regenerate,
        ToolChangedCallback on_tool_changed,
        BrushSettingsChangedCallback on_brush_settings_changed,
        ClearConstraintsCallback on_clear_constraints,
        ViewChangedCallback on_view_changed,
        PresentationChangedCallback on_presentation_changed
    );

    void update(
        const procgen::GreaterRealmMap& map,
        const procgen::TemperatureNormalSummary& temperature_summary
    );

private:
    void regenerate(bool force_full = false);
    void notify_view_changed();
    void notify_presentation_changed();
    void update_overlay_buttons();
    void select_presentation_mode(GreaterRealmPresentationMode mode);
    void update_presentation_buttons();
    void select_constraint_tool(procgen::TerrainConstraintTool tool);
    void notify_brush_settings_changed();
    void update_constraint_tool_buttons();

    procgen::GreaterRealmGeneratorSettings* m_settings{nullptr};
    procgen::GreaterRealmDebugOptions* m_debug_options{nullptr};
    GreaterRealmPresentationSettings* m_presentation_settings{nullptr};
    procgen::TerrainConstraintBrushSettings* m_brush_settings{nullptr};
    RegenerateCallback m_on_regenerate;
    ToolChangedCallback m_on_tool_changed;
    BrushSettingsChangedCallback m_on_brush_settings_changed;
    ClearConstraintsCallback m_on_clear_constraints;
    ViewChangedCallback m_on_view_changed;
    PresentationChangedCallback m_on_presentation_changed;
    procgen::TerrainConstraintTool m_selected_tool{procgen::TerrainConstraintTool::Mountain};
    ui::TextBlock* m_seed_text{nullptr};
    ui::TextBlock* m_island_bias_text{nullptr};
    ui::TextBlock* m_seed_variation_text{nullptr};
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
    ui::TextBlock* m_elevation_scale_text{nullptr};
    ui::TextBlock* m_coverage_text{nullptr};
    ui::TextBlock* m_terrain_text{nullptr};
    ui::TextBlock* m_hydrology_text{nullptr};
    ui::TextBlock* m_temperature_text{nullptr};
    ui::TextBlock* m_brush_size_text{nullptr};
    ui::TextBlock* m_brush_strength_text{nullptr};
    ui::Slider* m_island_bias_slider{nullptr};
    ui::Slider* m_seed_variation_slider{nullptr};
    ui::Slider* m_coastline_noise_slider{nullptr};
    ui::Slider* m_base_elevation_slider{nullptr};
    ui::Slider* m_mountain_slider{nullptr};
    ui::Slider* m_peak_spacing_slider{nullptr};
    ui::Slider* m_peak_radius_slider{nullptr};
    ui::Slider* m_peak_jaggedness_slider{nullptr};
    ui::Slider* m_ridge_slider{nullptr};
    ui::Slider* m_valley_slider{nullptr};
    ui::Slider* m_noise_slider{nullptr};
    ui::Slider* m_ocean_depth_slider{nullptr};
    ui::Slider* m_channel_threshold_slider{nullptr};
    ui::Slider* m_elevation_scale_slider{nullptr};
    ui::Slider* m_brush_size_slider{nullptr};
    ui::Slider* m_brush_strength_slider{nullptr};
    ui::Button* m_coastline_button{nullptr};
    ui::Button* m_peaks_button{nullptr};
    ui::Button* m_rivers_button{nullptr};
    ui::Button* m_drainage_button{nullptr};
    ui::Button* m_flat_button{nullptr};
    ui::Button* m_tilted_3d_button{nullptr};
    ui::Button* m_ocean_tool_button{nullptr};
    ui::Button* m_shallow_tool_button{nullptr};
    ui::Button* m_valley_tool_button{nullptr};
    ui::Button* m_mountain_tool_button{nullptr};
};

} // namespace game
