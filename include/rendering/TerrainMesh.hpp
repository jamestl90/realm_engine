#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace rendering {

#pragma pack(push, 1)
struct TerrainVertex {
    float x{0.0f};
    float y{0.0f};
    float elevation{0.0f};
    float gradient_x{0.0f};
    float gradient_y{0.0f};
    std::uint8_t r{255};
    std::uint8_t g{255};
    std::uint8_t b{255};
    std::uint8_t a{255};
};
#pragma pack(pop)

struct TerrainMesh {
    std::uint32_t width{0};
    std::uint32_t height{0};
    float extent_x{0.0f};
    float extent_y{0.0f};
    float minimum_elevation{0.0f};
    float maximum_elevation{0.0f};
    std::vector<TerrainVertex> vertices;
    std::vector<std::uint32_t> indices;

    [[nodiscard]] bool empty() const noexcept { return vertices.empty() || indices.empty(); }
    [[nodiscard]] std::size_t expected_vertex_count() const noexcept;
    [[nodiscard]] std::size_t expected_index_count() const noexcept;
    [[nodiscard]] bool has_expected_shape() const noexcept;
};

[[nodiscard]] TerrainMesh build_heightfield_mesh(
    std::uint32_t width,
    std::uint32_t height,
    float cell_size,
    std::span<const float> elevations,
    std::span<const std::uint8_t> rgba = {}
);

} // namespace rendering
