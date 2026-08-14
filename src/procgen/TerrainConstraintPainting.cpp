#include "../../include/procgen/TerrainConstraintPainting.hpp"
#include <algorithm>
#include <cmath>

namespace procgen {

bool TerrainPreviewBounds::is_valid() const noexcept {
    return std::isfinite(x)
        && std::isfinite(y)
        && std::isfinite(width)
        && std::isfinite(height)
        && width > 0.0f
        && height > 0.0f;
}

bool TerrainPreviewBounds::contains(float pointer_x, float pointer_y) const noexcept {
    return is_valid()
        && std::isfinite(pointer_x)
        && std::isfinite(pointer_y)
        && pointer_x >= x
        && pointer_y >= y
        && pointer_x <= x + width
        && pointer_y <= y + height;
}

std::optional<TerrainConstraintPaintSample> terrain_constraint_paint_sample(
    const TerrainPreviewBounds& bounds,
    TerrainConstraintTool tool,
    float pointer_x,
    float pointer_y
) noexcept {
    if (!bounds.contains(pointer_x, pointer_y)) {
        return std::nullopt;
    }

    return TerrainConstraintPaintSample{
        tool,
        std::clamp((pointer_x - bounds.x) / bounds.width, 0.0f, 1.0f),
        std::clamp((pointer_y - bounds.y) / bounds.height, 0.0f, 1.0f)
    };
}

std::optional<TerrainConstraintPaintSample> TerrainConstraintPaintSession::pointer_down(
    const TerrainPreviewBounds& bounds,
    float pointer_x,
    float pointer_y,
    bool input_blocked
) noexcept {
    if (input_blocked || !bounds.contains(pointer_x, pointer_y)) {
        return std::nullopt;
    }

    m_painting = true;
    m_last_sample.reset();
    return sample_if_new(bounds, pointer_x, pointer_y);
}

std::optional<TerrainConstraintPaintSample> TerrainConstraintPaintSession::pointer_move(
    const TerrainPreviewBounds& bounds,
    float pointer_x,
    float pointer_y,
    bool input_blocked
) noexcept {
    if (!m_painting || input_blocked) {
        return std::nullopt;
    }
    return sample_if_new(bounds, pointer_x, pointer_y);
}

void TerrainConstraintPaintSession::pointer_up() noexcept {
    cancel();
}

void TerrainConstraintPaintSession::cancel() noexcept {
    m_painting = false;
    m_last_sample.reset();
}

std::optional<TerrainConstraintPaintSample> TerrainConstraintPaintSession::sample_if_new(
    const TerrainPreviewBounds& bounds,
    float pointer_x,
    float pointer_y
) noexcept {
    auto sample = terrain_constraint_paint_sample(bounds, m_tool, pointer_x, pointer_y);
    if (!sample) {
        return std::nullopt;
    }

    if (m_last_sample
        && m_last_sample->tool == sample->tool
        && m_last_sample->normalized_x == sample->normalized_x
        && m_last_sample->normalized_y == sample->normalized_y) {
        return std::nullopt;
    }

    m_last_sample = sample;
    return sample;
}

} // namespace procgen
