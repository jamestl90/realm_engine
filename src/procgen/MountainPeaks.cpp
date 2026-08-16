#include "../../include/procgen/MountainPeaks.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <vector>

namespace procgen {
namespace {

struct PeakCandidate {
    std::uint32_t index{INVALID_CELL_INDEX};
    float priority{0.0f};
};

struct DistanceNode {
    std::uint32_t index{INVALID_CELL_INDEX};
    float distance{0.0f};
};

struct DistanceNodeGreater {
    bool operator()(const DistanceNode& left, const DistanceNode& right) const noexcept {
        return left.distance == right.distance
            ? left.index > right.index
            : left.distance > right.distance;
    }
};

constexpr std::array<std::array<std::int32_t, 2>, 8> NEIGHBORS{{
    {{-1, -1}}, {{0, -1}}, {{1, -1}}, {{-1, 0}},
    {{1, 0}}, {{-1, 1}}, {{0, 1}}, {{1, 1}}
}};

std::uint64_t hash_value(Seed seed, std::uint32_t a, std::uint32_t b, std::uint64_t salt) noexcept {
    std::uint64_t value = seed ^ salt;
    value ^= static_cast<std::uint64_t>(a) + 0x9e3779b97f4a7c15ull + (value << 6) + (value >> 2);
    value ^= static_cast<std::uint64_t>(b) + 0xbf58476d1ce4e5b9ull + (value << 6) + (value >> 2);
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ull;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebull;
    value ^= value >> 31;
    return value;
}

float random01(Seed seed, std::uint32_t a, std::uint32_t b, std::uint64_t salt) noexcept {
    return static_cast<float>((hash_value(seed, a, b, salt) >> 40) & 0xffffffu)
        / static_cast<float>(0xffffffu);
}

float squared_distance(const GreaterRealmCell& a, const GreaterRealmCell& b) noexcept {
    const float dx = static_cast<float>(a.x - b.x);
    const float dy = static_cast<float>(a.y - b.y);
    return dx * dx + dy * dy;
}

std::vector<PeakCandidate> build_candidates(const GreaterRealmMap& map) {
    std::vector<PeakCandidate> candidates;
    candidates.reserve(map.cells.size());

    for (std::uint32_t y = 0; y < map.height; ++y) {
        for (std::uint32_t x = 0; x < map.width; ++x) {
            const float centered_x = map.width > 1
                ? static_cast<float>(x) / static_cast<float>(map.width - 1) * 2.0f - 1.0f
                : 0.0f;
            const float centered_y = map.height > 1
                ? static_cast<float>(y) / static_cast<float>(map.height - 1) * 2.0f - 1.0f
                : 0.0f;
            const float square_distance = std::max(std::abs(centered_x), std::abs(centered_y));
            const float boundary_weight = std::clamp(1.0f - square_distance, 0.0f, 1.0f) * 0.35f;
            candidates.push_back({
                static_cast<std::uint32_t>(map.index(x, y)),
                random01(map.seed, x, y, 0x4d4f554e5441494eull) + boundary_weight
            });
        }
    }

    std::stable_sort(candidates.begin(), candidates.end(), [](const PeakCandidate& left, const PeakCandidate& right) {
        return left.priority == right.priority
            ? left.index < right.index
            : left.priority > right.priority;
    });
    return candidates;
}

} // namespace

void generate_mountain_peak_field(
    GreaterRealmMap& map,
    const GreaterRealmGeneratorSettings& settings
) {
    map.mountain_peaks.clear();
    if (!map.has_expected_cell_count() || map.cells.empty()) {
        return;
    }

    const auto& character = map.terrain_character;
    const float spacing = std::max(settings.mountain_peak_spacing * character.peak_spacing_scale, 2.0f);
    const float spacing_squared = spacing * spacing;
    const float radius = std::max(settings.mountain_peak_radius * character.peak_radius_scale, 0.001f);
    const float jaggedness = std::clamp(settings.mountain_peak_jaggedness, 0.0f, 1.0f);

    for (auto& cell : map.cells) {
        cell.is_mountain_peak = false;
        cell.mountain_distance = std::numeric_limits<float>::infinity();
        cell.mountain_influence = 0.0f;
    }

    std::vector<PeakCandidate> selected_sites;
    std::vector<PeakCandidate> relief_sources;
    const auto candidates = build_candidates(map);
    for (const auto& candidate : candidates) {
        const auto& candidate_cell = map.cells[candidate.index];
        bool separated = true;
        for (const auto& selected : selected_sites) {
            if (squared_distance(candidate_cell, map.cells[selected.index]) < spacing_squared) {
                separated = false;
                break;
            }
        }

        if (!separated) {
            continue;
        }

        selected_sites.push_back(candidate);
        if (candidate_cell.relief_constraint > 0.0f) {
            relief_sources.push_back(candidate);
        }
        if (candidate_cell.relief_constraint > 0.0f && !candidate_cell.is_water) {
            map.mountain_peaks.push_back({
                candidate.index,
                candidate_cell.x,
                candidate_cell.y,
                candidate.priority
            });
        }
    }

    std::priority_queue<DistanceNode, std::vector<DistanceNode>, DistanceNodeGreater> open;
    for (const auto& source : relief_sources) {
        auto& cell = map.cells[source.index];
        cell.mountain_distance = 0.0f;
        open.push({source.index, 0.0f});
        cell.is_mountain_peak = !cell.is_water;
    }

    while (!open.empty()) {
        const DistanceNode current = open.top();
        open.pop();
        if (current.distance != map.cells[current.index].mountain_distance) {
            continue;
        }

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
            const std::uint32_t edge_a = std::min(current.index, neighbor_index);
            const std::uint32_t edge_b = std::max(current.index, neighbor_index);
            const float base_cost = offset[0] != 0 && offset[1] != 0 ? 1.41421356f : 1.0f;
            const float jitter = (random01(map.seed, edge_a, edge_b, 0x4a41474745444d54ull) * 2.0f - 1.0f) * jaggedness;
            const float next_distance = current.distance + base_cost * std::max(1.0f + jitter, 0.1f);
            auto& neighbor = map.cells[neighbor_index];
            if (next_distance < neighbor.mountain_distance) {
                neighbor.mountain_distance = next_distance;
                open.push({neighbor_index, next_distance});
            }
        }
    }

    for (auto& cell : map.cells) {
        cell.mountain_influence = std::clamp(1.0f - cell.mountain_distance / radius, 0.0f, 1.0f);
    }
}

} // namespace procgen
