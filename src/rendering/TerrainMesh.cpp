#include "../../include/rendering/TerrainMesh.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace rendering {
namespace {

std::size_t cell_count(std::uint32_t width, std::uint32_t height) noexcept {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

} // namespace

std::size_t TerrainMesh::expected_vertex_count() const noexcept {
    return cell_count(width, height);
}

std::size_t TerrainMesh::expected_index_count() const noexcept {
    if (width < 2 || height < 2) {
        return 0;
    }
    return static_cast<std::size_t>(width - 1)
        * static_cast<std::size_t>(height - 1)
        * 6;
}

bool TerrainMesh::has_expected_shape() const noexcept {
    return vertices.size() == expected_vertex_count()
        && indices.size() == expected_index_count();
}

TerrainMesh build_heightfield_mesh(
    std::uint32_t width,
    std::uint32_t height,
    float cell_size,
    std::span<const float> elevations,
    std::span<const std::uint8_t> rgba
) {
    TerrainMesh mesh;
    const std::size_t expected_cells = cell_count(width, height);
    if (width < 2
        || height < 2
        || !std::isfinite(cell_size)
        || cell_size <= 0.0f
        || elevations.size() != expected_cells
        || (!rgba.empty() && rgba.size() != expected_cells * 4)) {
        return mesh;
    }

    for (float elevation : elevations) {
        if (!std::isfinite(elevation)) {
            return {};
        }
    }

    mesh.width = width;
    mesh.height = height;
    mesh.extent_x = static_cast<float>(width - 1) * cell_size;
    mesh.extent_y = static_cast<float>(height - 1) * cell_size;
    mesh.minimum_elevation = std::numeric_limits<float>::max();
    mesh.maximum_elevation = std::numeric_limits<float>::lowest();
    mesh.vertices.reserve(expected_cells);
    mesh.indices.reserve(mesh.expected_index_count());

    const float origin_x = mesh.extent_x * 0.5f;
    const float origin_y = mesh.extent_y * 0.5f;
    const auto elevation_at = [&](std::uint32_t x, std::uint32_t y) {
        return elevations[static_cast<std::size_t>(y) * width + x];
    };

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::uint32_t left_x = x > 0 ? x - 1 : x;
            const std::uint32_t right_x = x + 1 < width ? x + 1 : x;
            const std::uint32_t top_y = y > 0 ? y - 1 : y;
            const std::uint32_t bottom_y = y + 1 < height ? y + 1 : y;
            const float x_distance = static_cast<float>(right_x - left_x) * cell_size;
            const float y_distance = static_cast<float>(bottom_y - top_y) * cell_size;
            const float elevation = elevation_at(x, y);

            TerrainVertex vertex;
            vertex.x = static_cast<float>(x) * cell_size - origin_x;
            vertex.y = static_cast<float>(y) * cell_size - origin_y;
            vertex.elevation = elevation;
            vertex.gradient_x = (elevation_at(right_x, y) - elevation_at(left_x, y)) / x_distance;
            vertex.gradient_y = (elevation_at(x, bottom_y) - elevation_at(x, top_y)) / y_distance;

            if (!rgba.empty()) {
                const std::size_t colour_offset = (static_cast<std::size_t>(y) * width + x) * 4;
                vertex.r = rgba[colour_offset];
                vertex.g = rgba[colour_offset + 1];
                vertex.b = rgba[colour_offset + 2];
                vertex.a = rgba[colour_offset + 3];
            }

            mesh.minimum_elevation = std::min(mesh.minimum_elevation, elevation);
            mesh.maximum_elevation = std::max(mesh.maximum_elevation, elevation);
            mesh.vertices.push_back(vertex);
        }
    }

    for (std::uint32_t y = 0; y + 1 < height; ++y) {
        for (std::uint32_t x = 0; x + 1 < width; ++x) {
            const std::uint32_t top_left = y * width + x;
            const std::uint32_t top_right = top_left + 1;
            const std::uint32_t bottom_left = top_left + width;
            const std::uint32_t bottom_right = bottom_left + 1;

            mesh.indices.push_back(top_left);
            mesh.indices.push_back(top_right);
            mesh.indices.push_back(bottom_right);
            mesh.indices.push_back(top_left);
            mesh.indices.push_back(bottom_right);
            mesh.indices.push_back(bottom_left);
        }
    }

    return mesh;
}

} // namespace rendering
