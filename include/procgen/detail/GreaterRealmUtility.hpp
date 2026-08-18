#pragma once

#include "GenerationUtility.hpp"
#include "../GreaterRealm.hpp"
#include <functional>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

namespace procgen::detail {

[[nodiscard]] inline float normalized_land_height(
    const GreaterRealmCell& cell
) noexcept {
    if (cell.is_water) {
        return 0.0f;
    }
    return clamp01(
        (cell.elevation - NORMALIZED_WATERLINE) / (1.0f - NORMALIZED_WATERLINE)
    );
}

[[nodiscard]] inline std::vector<float> distance_to_water(
    const GreaterRealmMap& terrain
) {
    using QueueEntry = std::pair<float, std::uint32_t>;

    std::vector<float> distances(
        terrain.cells.size(),
        std::numeric_limits<float>::infinity()
    );
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>> open;
    for (std::uint32_t index = 0; index < terrain.cells.size(); ++index) {
        if (terrain.cells[index].is_water) {
            distances[index] = 0.0f;
            open.emplace(0.0f, index);
        }
    }

    const float cell_size = terrain.cell_size > 0.0f ? terrain.cell_size : 1.0f;
    while (!open.empty()) {
        const auto [distance, index] = open.top();
        open.pop();
        if (distance > distances[index]) {
            continue;
        }

        const auto x = static_cast<std::int32_t>(index % terrain.width);
        const auto y = static_cast<std::int32_t>(index / terrain.width);
        for (const auto& offset : EIGHT_WAY_NEIGHBORS) {
            const std::int32_t neighbor_x = x + offset[0];
            const std::int32_t neighbor_y = y + offset[1];
            if (!terrain.contains(neighbor_x, neighbor_y)) {
                continue;
            }
            const auto neighbor_index = static_cast<std::uint32_t>(terrain.index(
                static_cast<std::uint32_t>(neighbor_x),
                static_cast<std::uint32_t>(neighbor_y)
            ));
            const bool diagonal = offset[0] != 0 && offset[1] != 0;
            const float candidate = distance + cell_size * (diagonal ? SQRT_TWO : 1.0f);
            if (candidate < distances[neighbor_index]) {
                distances[neighbor_index] = candidate;
                open.emplace(candidate, neighbor_index);
            }
        }
    }
    return distances;
}

} // namespace procgen::detail
