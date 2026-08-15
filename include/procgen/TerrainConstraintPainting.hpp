#pragma once

#include "TerrainConstraints.hpp"
#include <optional>

namespace procgen {

inline constexpr float DEFAULT_TERRAIN_CONSTRAINT_BRUSH_RADIUS = 0.12f;
inline constexpr float MIN_TERRAIN_CONSTRAINT_BRUSH_RADIUS = 0.02f;
inline constexpr float MAX_TERRAIN_CONSTRAINT_BRUSH_RADIUS = 0.25f;
inline constexpr float DEFAULT_TERRAIN_CONSTRAINT_BRUSH_STRENGTH = 1.0f;

struct TerrainPreviewBounds {
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};

    [[nodiscard]] bool is_valid() const noexcept;
    [[nodiscard]] bool contains(float pointer_x, float pointer_y) const noexcept;
};

struct TerrainConstraintBrushSettings {
    float normalized_radius{DEFAULT_TERRAIN_CONSTRAINT_BRUSH_RADIUS};
    float strength{DEFAULT_TERRAIN_CONSTRAINT_BRUSH_STRENGTH};
};

[[nodiscard]] TerrainConstraintBrushSettings clamp_terrain_constraint_brush_settings(
    TerrainConstraintBrushSettings settings
) noexcept;

struct TerrainConstraintPaintSample {
    TerrainConstraintTool tool{TerrainConstraintTool::Mountain};
    float normalized_x{0.0f};
    float normalized_y{0.0f};
    float normalized_radius{DEFAULT_TERRAIN_CONSTRAINT_BRUSH_RADIUS};
    float strength{DEFAULT_TERRAIN_CONSTRAINT_BRUSH_STRENGTH};
};

[[nodiscard]] std::optional<TerrainConstraintPaintSample> terrain_constraint_paint_sample(
    const TerrainPreviewBounds& bounds,
    TerrainConstraintTool tool,
    float pointer_x,
    float pointer_y,
    TerrainConstraintBrushSettings brush_settings = {}
) noexcept;

class TerrainConstraintPaintSession {
public:
    void select_tool(TerrainConstraintTool tool) noexcept { m_tool = tool; }
    [[nodiscard]] TerrainConstraintTool selected_tool() const noexcept { return m_tool; }
    void set_brush_settings(TerrainConstraintBrushSettings settings) noexcept;
    [[nodiscard]] TerrainConstraintBrushSettings brush_settings() const noexcept { return m_brush_settings; }
    [[nodiscard]] bool is_painting() const noexcept { return m_painting; }

    [[nodiscard]] std::optional<TerrainConstraintPaintSample> pointer_down(
        const TerrainPreviewBounds& bounds,
        float pointer_x,
        float pointer_y,
        bool input_blocked
    ) noexcept;

    [[nodiscard]] std::optional<TerrainConstraintPaintSample> pointer_move(
        const TerrainPreviewBounds& bounds,
        float pointer_x,
        float pointer_y,
        bool input_blocked
    ) noexcept;

    void pointer_up() noexcept;
    void cancel() noexcept;

private:
    [[nodiscard]] std::optional<TerrainConstraintPaintSample> sample_if_new(
        const TerrainPreviewBounds& bounds,
        float pointer_x,
        float pointer_y
    ) noexcept;

    TerrainConstraintTool m_tool{TerrainConstraintTool::Mountain};
    TerrainConstraintBrushSettings m_brush_settings{};
    bool m_painting{false};
    std::optional<TerrainConstraintPaintSample> m_last_sample;
};

} // namespace procgen
