#include "../../include/procgen/Hydrology.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <queue>
#include <vector>

namespace procgen {
namespace {

struct DrainageNode {
    float elevation{0.0f};
    std::uint32_t index{INVALID_CELL_INDEX};
};

struct DrainageNodeGreater {
    bool operator()(const DrainageNode& left, const DrainageNode& right) const noexcept {
        return left.elevation == right.elevation
            ? left.index > right.index
            : left.elevation > right.elevation;
    }
};

constexpr std::array<std::array<std::int32_t, 2>, 8> NEIGHBORS{{
    {{-1, -1}}, {{0, -1}}, {{1, -1}}, {{-1, 0}},
    {{1, 0}}, {{-1, 1}}, {{0, 1}}, {{1, 1}}
}};

} // namespace

void build_greater_realm_drainage(GreaterRealmMap& map) {
    map.drainage_order.clear();
    map.rivers.clear();
    if (!map.has_expected_cell_count() || map.cells.empty()) {
        return;
    }

    std::vector<bool> visited(map.cells.size(), false);
    std::priority_queue<DrainageNode, std::vector<DrainageNode>, DrainageNodeGreater> open;

    const auto seed_outlet = [&](std::uint32_t index) {
        if (visited[index]) {
            return;
        }
        auto& cell = map.cells[index];
        visited[index] = true;
        cell.downslope_index = INVALID_CELL_INDEX;
        cell.is_drainage_outlet = true;
        cell.drainage_elevation = cell.elevation;
        open.push({cell.drainage_elevation, index});
    };

    for (std::uint32_t index = 0; index < map.cells.size(); ++index) {
        auto& cell = map.cells[index];
        cell.downslope_index = INVALID_CELL_INDEX;
        cell.is_drainage_outlet = false;
        cell.drainage_elevation = cell.elevation;
        cell.flow = 0.0f;
        if (cell.is_ocean) {
            seed_outlet(index);
        }
    }

    if (open.empty()) {
        for (std::uint32_t x = 0; x < map.width; ++x) {
            seed_outlet(static_cast<std::uint32_t>(map.index(x, 0)));
            seed_outlet(static_cast<std::uint32_t>(map.index(x, map.height - 1)));
        }
        for (std::uint32_t y = 0; y < map.height; ++y) {
            seed_outlet(static_cast<std::uint32_t>(map.index(0, y)));
            seed_outlet(static_cast<std::uint32_t>(map.index(map.width - 1, y)));
        }
    }

    map.drainage_order.reserve(map.cells.size());
    while (!open.empty()) {
        const DrainageNode current = open.top();
        open.pop();
        map.drainage_order.push_back(current.index);
        const auto& current_cell = map.cells[current.index];

        for (const auto& offset : NEIGHBORS) {
            const std::int32_t neighbor_x = current_cell.x + offset[0];
            const std::int32_t neighbor_y = current_cell.y + offset[1];
            if (!map.contains(neighbor_x, neighbor_y)) {
                continue;
            }

            const auto neighbor_index = static_cast<std::uint32_t>(map.index(
                static_cast<std::uint32_t>(neighbor_x),
                static_cast<std::uint32_t>(neighbor_y)
            ));
            if (visited[neighbor_index]) {
                continue;
            }

            auto& neighbor = map.cells[neighbor_index];
            visited[neighbor_index] = true;
            neighbor.downslope_index = current.index;
            neighbor.is_drainage_outlet = false;
            neighbor.drainage_elevation = std::max(neighbor.elevation, current.elevation);
            open.push({neighbor.drainage_elevation, neighbor_index});
        }
    }
}

void accumulate_greater_realm_rivers(
    GreaterRealmMap& map,
    const GreaterRealmGeneratorSettings& settings
) {
    map.rivers.clear();
    if (!map.has_expected_cell_count() || map.drainage_order.size() != map.cells.size()) {
        return;
    }

    const float flow_scale = std::max(settings.river_flow_scale, 0.0f);
    const float minimum_flow = std::max(settings.river_min_flow, 0.0f);
    const float width_scale = std::max(settings.river_width_scale, 0.0f);

    for (auto& cell : map.cells) {
        cell.flow = cell.is_water ? 0.0f : std::max(cell.moisture, 0.01f) * flow_scale;
    }

    for (auto iterator = map.drainage_order.rbegin(); iterator != map.drainage_order.rend(); ++iterator) {
        auto& cell = map.cells[*iterator];
        if (cell.downslope_index != INVALID_CELL_INDEX) {
            map.cells[cell.downslope_index].flow += cell.flow;
        }
    }

    for (std::uint32_t source_index = 0; source_index < map.cells.size(); ++source_index) {
        const auto& source = map.cells[source_index];
        if (source.is_water
            || source.downslope_index == INVALID_CELL_INDEX
            || source.flow < minimum_flow) {
            continue;
        }

        const auto& destination = map.cells[source.downslope_index];
        if (destination.drainage_elevation > source.drainage_elevation) {
            continue;
        }

        map.rivers.push_back({
            source_index,
            source.downslope_index,
            source.flow,
            1.0f + std::sqrt(std::max(source.flow - minimum_flow, 0.0f)) * width_scale
        });
    }
}

} // namespace procgen
