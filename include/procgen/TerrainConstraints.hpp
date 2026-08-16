#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace procgen {

enum class TerrainConstraintTool : std::uint8_t {
    Ocean,
    ShallowWater,
    Valley,
    Mountain
};

struct TerrainConstraintSample {
    float elevation{0.0f};
    float influence{0.0f};
};

class TerrainConstraintField {
public:
    TerrainConstraintField() = default;
    TerrainConstraintField(std::uint32_t width, std::uint32_t height);

    [[nodiscard]] std::uint32_t width() const noexcept { return m_width; }
    [[nodiscard]] std::uint32_t height() const noexcept { return m_height; }
    [[nodiscard]] bool empty() const noexcept { return m_values.empty(); }
    [[nodiscard]] bool has_expected_sample_count() const noexcept;

    void resize(std::uint32_t width, std::uint32_t height);
    void clear() noexcept;
    void paint(
        TerrainConstraintTool tool,
        float normalized_x,
        float normalized_y,
        float normalized_radius,
        float strength = 1.0f
    );

    [[nodiscard]] TerrainConstraintSample sample(float normalized_x, float normalized_y) const noexcept;
    [[nodiscard]] TerrainConstraintSample sample_at(std::uint32_t x, std::uint32_t y) const noexcept;

private:
    friend std::vector<std::uint8_t> serialize_terrain_constraints(const TerrainConstraintField& field);
    friend std::optional<TerrainConstraintField> deserialize_terrain_constraints(std::span<const std::uint8_t> bytes);

    [[nodiscard]] std::size_t index(std::uint32_t x, std::uint32_t y) const noexcept;

    std::uint32_t m_width{0};
    std::uint32_t m_height{0};
    std::vector<float> m_values;
    std::vector<float> m_influences;
};

[[nodiscard]] float terrain_constraint_value(TerrainConstraintTool tool) noexcept;
[[nodiscard]] std::vector<std::uint8_t> serialize_terrain_constraints(const TerrainConstraintField& field);
[[nodiscard]] std::optional<TerrainConstraintField> deserialize_terrain_constraints(std::span<const std::uint8_t> bytes);

} // namespace procgen
