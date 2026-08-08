#include "../../include/procgen/GreaterRealm.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace procgen {

namespace {

constexpr float INF_DISTANCE = 1.0e20f;

[[nodiscard]] float clamp01(float value) noexcept {
    return std::clamp(value, 0.0f, 1.0f);
}

[[nodiscard]] float smoothstep(float edge0, float edge1, float value) noexcept {
    if (edge0 == edge1) {
        return value < edge0 ? 0.0f : 1.0f;
    }

    const float t = clamp01((value - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

[[nodiscard]] float lerp(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}

[[nodiscard]] std::uint64_t hash_coords(Seed seed, std::int32_t x, std::int32_t y, std::uint64_t salt) noexcept {
    std::uint64_t value = seed ^ salt;
    value ^= static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) + 0x9e3779b97f4a7c15ull + (value << 6) + (value >> 2);
    value ^= static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)) + 0xbf58476d1ce4e5b9ull + (value << 6) + (value >> 2);
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ull;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebull;
    value ^= value >> 31;
    return value;
}

[[nodiscard]] float random01(Seed seed, std::int32_t x, std::int32_t y, std::uint64_t salt) noexcept {
    const std::uint64_t value = hash_coords(seed, x, y, salt);
    return static_cast<float>((value >> 40) & 0xffffffu) / static_cast<float>(0xffffffu);
}

[[nodiscard]] float value_noise(Seed seed, float x, float y, float frequency, std::uint64_t salt) noexcept {
    const float sample_x = x * frequency;
    const float sample_y = y * frequency;
    const auto x0 = static_cast<std::int32_t>(std::floor(sample_x));
    const auto y0 = static_cast<std::int32_t>(std::floor(sample_y));
    const float tx = sample_x - static_cast<float>(x0);
    const float ty = sample_y - static_cast<float>(y0);
    const float sx = smoothstep(0.0f, 1.0f, tx);
    const float sy = smoothstep(0.0f, 1.0f, ty);

    const float v00 = random01(seed, x0, y0, salt);
    const float v10 = random01(seed, x0 + 1, y0, salt);
    const float v01 = random01(seed, x0, y0 + 1, salt);
    const float v11 = random01(seed, x0 + 1, y0 + 1, salt);

    const float vx0 = lerp(v00, v10, sx);
    const float vx1 = lerp(v01, v11, sx);
    return lerp(vx0, vx1, sy);
}

[[nodiscard]] float fbm(Seed seed, float x, float y, float frequency, std::uint64_t salt, std::uint32_t octaves = 4) noexcept {
    float total = 0.0f;
    float amplitude = 1.0f;
    float amplitude_sum = 0.0f;

    for (std::uint32_t octave = 0; octave < octaves; ++octave) {
        total += value_noise(seed, x, y, frequency, salt + octave * 1013ull) * amplitude;
        amplitude_sum += amplitude;
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }

    return amplitude_sum > 0.0f ? total / amplitude_sum : 0.0f;
}

[[nodiscard]] float ridged_noise(Seed seed, float x, float y, float frequency, std::uint64_t salt, std::uint32_t octaves = 4) noexcept {
    const float value = fbm(seed, x, y, frequency, salt, octaves);
    return 1.0f - std::abs(value * 2.0f - 1.0f);
}

[[nodiscard]] float normalized_x(std::uint32_t x, std::uint32_t width) noexcept {
    return width > 1 ? static_cast<float>(x) / static_cast<float>(width - 1) : 0.0f;
}

[[nodiscard]] float normalized_y(std::uint32_t y, std::uint32_t height) noexcept {
    return height > 1 ? static_cast<float>(y) / static_cast<float>(height - 1) : 0.0f;
}

void compute_coast_distance(GreaterRealmMap& map) {
    std::vector<float> distances(map.cells.size(), INF_DISTANCE);

    for (std::uint32_t y = 0; y < map.height; ++y) {
        for (std::uint32_t x = 0; x < map.width; ++x) {
            const auto index = map.index(x, y);
            const bool is_water = map.cells[index].is_water;
            bool touches_opposite = false;

            for (std::int32_t oy = -1; oy <= 1 && !touches_opposite; ++oy) {
                for (std::int32_t ox = -1; ox <= 1; ++ox) {
                    if (ox == 0 && oy == 0) {
                        continue;
                    }

                    const auto* neighbor = map.cell(static_cast<std::int32_t>(x) + ox, static_cast<std::int32_t>(y) + oy);
                    if (neighbor && neighbor->is_water != is_water) {
                        touches_opposite = true;
                        break;
                    }
                }
            }

            if (touches_opposite) {
                distances[index] = 0.0f;
            }
        }
    }

    const auto relax = [&](std::uint32_t x, std::uint32_t y, std::int32_t ox, std::int32_t oy, float cost) {
        auto* cell = map.cell(static_cast<std::int32_t>(x) + ox, static_cast<std::int32_t>(y) + oy);
        if (!cell) {
            return;
        }

        const auto current_index = map.index(x, y);
        const auto neighbor_index = map.index(static_cast<std::uint32_t>(cell->x), static_cast<std::uint32_t>(cell->y));
        distances[current_index] = std::min(distances[current_index], distances[neighbor_index] + cost);
    };

    for (std::uint32_t y = 0; y < map.height; ++y) {
        for (std::uint32_t x = 0; x < map.width; ++x) {
            relax(x, y, -1, 0, 1.0f);
            relax(x, y, 0, -1, 1.0f);
            relax(x, y, -1, -1, 1.41421356f);
            relax(x, y, 1, -1, 1.41421356f);
        }
    }

    for (std::int32_t y = static_cast<std::int32_t>(map.height) - 1; y >= 0; --y) {
        for (std::int32_t x = static_cast<std::int32_t>(map.width) - 1; x >= 0; --x) {
            const auto ux = static_cast<std::uint32_t>(x);
            const auto uy = static_cast<std::uint32_t>(y);
            relax(ux, uy, 1, 0, 1.0f);
            relax(ux, uy, 0, 1, 1.0f);
            relax(ux, uy, 1, 1, 1.41421356f);
            relax(ux, uy, -1, 1, 1.41421356f);
        }
    }

    for (std::size_t i = 0; i < map.cells.size(); ++i) {
        map.cells[i].distance_to_coast = distances[i] == INF_DISTANCE ? 0.0f : distances[i];
    }
}

void compute_slopes(GreaterRealmMap& map) {
    const float inv_cell_size = map.cell_size > 0.0f ? 1.0f / map.cell_size : 1.0f;

    for (std::uint32_t y = 0; y < map.height; ++y) {
        for (std::uint32_t x = 0; x < map.width; ++x) {
            auto& cell = map.cells[map.index(x, y)];
            const auto* left = map.cell(static_cast<std::int32_t>(x) - 1, static_cast<std::int32_t>(y));
            const auto* right = map.cell(static_cast<std::int32_t>(x) + 1, static_cast<std::int32_t>(y));
            const auto* up = map.cell(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y) - 1);
            const auto* down = map.cell(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y) + 1);

            const float dx = ((right ? right->elevation : cell.elevation) - (left ? left->elevation : cell.elevation)) * 0.5f;
            const float dy = ((down ? down->elevation : cell.elevation) - (up ? up->elevation : cell.elevation)) * 0.5f;
            cell.slope = std::sqrt(dx * dx + dy * dy) * inv_cell_size;
        }
    }
}

void classify_cells(GreaterRealmMap& map, const GreaterRealmGeneratorSettings& settings) {
    for (auto& cell : map.cells) {
        if (cell.is_water) {
            cell.terrain_form = TerrainForm::Ocean;
            continue;
        }

        if (cell.distance_to_coast <= settings.coast_distance) {
            cell.terrain_form = TerrainForm::Coast;
        } else if (cell.elevation >= settings.mountain_threshold) {
            cell.terrain_form = TerrainForm::Mountains;
        } else if (cell.elevation >= settings.highland_threshold) {
            cell.terrain_form = TerrainForm::Highlands;
        } else if (cell.elevation >= settings.hill_threshold) {
            cell.terrain_form = TerrainForm::Hills;
        } else {
            cell.terrain_form = TerrainForm::Plains;
        }
    }
}

} // namespace

const char* to_string(TerrainForm form) noexcept {
    switch (form) {
        case TerrainForm::Ocean:
            return "ocean";
        case TerrainForm::Coast:
            return "coast";
        case TerrainForm::Plains:
            return "plains";
        case TerrainForm::Hills:
            return "hills";
        case TerrainForm::Highlands:
            return "highlands";
        case TerrainForm::Mountains:
            return "mountains";
    }

    return "unknown";
}

bool is_water(TerrainForm form) noexcept {
    return form == TerrainForm::Ocean;
}

std::size_t GreaterRealmMap::expected_cell_count() const noexcept {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

bool GreaterRealmMap::has_expected_cell_count() const noexcept {
    return cells.size() == expected_cell_count();
}

bool GreaterRealmMap::contains(std::int32_t x, std::int32_t y) const noexcept {
    return x >= 0
        && y >= 0
        && static_cast<std::uint32_t>(x) < width
        && static_cast<std::uint32_t>(y) < height;
}

std::size_t GreaterRealmMap::index(std::uint32_t x, std::uint32_t y) const noexcept {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
}

GreaterRealmCell* GreaterRealmMap::cell(std::int32_t x, std::int32_t y) noexcept {
    if (!contains(x, y) || !has_expected_cell_count()) {
        return nullptr;
    }

    return &cells[index(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y))];
}

const GreaterRealmCell* GreaterRealmMap::cell(std::int32_t x, std::int32_t y) const noexcept {
    if (!contains(x, y) || !has_expected_cell_count()) {
        return nullptr;
    }

    return &cells[index(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y))];
}

GreaterRealmMap generate_greater_realm(const GreaterRealmGeneratorSettings& settings) {
    GreaterRealmMap map;
    map.seed = settings.seed;
    map.width = std::max<std::uint32_t>(settings.width, 1);
    map.height = std::max<std::uint32_t>(settings.height, 1);
    map.cell_size = settings.cell_size > 0.0f ? settings.cell_size : 1.0f;
    map.cells.resize(map.expected_cell_count());

    std::vector<float> raw_elevations(map.cells.size(), 0.0f);
    float min_elevation = std::numeric_limits<float>::max();
    float max_elevation = std::numeric_limits<float>::lowest();

    const float aspect = map.height > 0 ? static_cast<float>(map.width) / static_cast<float>(map.height) : 1.0f;

    for (std::uint32_t y = 0; y < map.height; ++y) {
        for (std::uint32_t x = 0; x < map.width; ++x) {
            const auto index = map.index(x, y);
            const float u = normalized_x(x, map.width);
            const float v = normalized_y(y, map.height);
            const float centered_x = (u * 2.0f - 1.0f) * aspect;
            const float centered_y = v * 2.0f - 1.0f;
            const float radius = std::sqrt(centered_x * centered_x + centered_y * centered_y);

            const float edge_falloff = 1.0f - smoothstep(0.58f, 1.08f, radius);
            const float land_noise = fbm(settings.seed, u, v, settings.land_shape_frequency, 11ull, 5);
            const float land_shape = clamp01(edge_falloff * 0.9f + (land_noise - 0.5f) * 0.75f + 0.18f);

            const float base_elevation = fbm(settings.seed, u, v, settings.base_elevation_frequency, 101ull, 5);
            const float mountain_mask = smoothstep(0.35f, 0.95f, land_shape);
            const float mountain_influence = std::pow(ridged_noise(settings.seed, u, v, settings.mountain_frequency, 211ull, 4), 2.0f) * mountain_mask;
            const float ridge_influence = std::pow(ridged_noise(settings.seed, u, v, settings.ridge_frequency, 307ull, 3), 3.0f) * mountain_mask;
            const float valley_influence = std::pow(ridged_noise(settings.seed, u, v, settings.valley_frequency, 401ull, 4), 2.0f) * smoothstep(0.2f, 0.85f, land_shape);
            const float terrain_noise = fbm(settings.seed, u, v, settings.terrain_noise_frequency, 503ull, 3) - 0.5f;

            const float raw_elevation =
                land_shape * settings.land_shape_weight
                + base_elevation * settings.base_elevation_weight
                + mountain_influence * settings.mountain_weight
                + ridge_influence * settings.ridge_weight
                - valley_influence * settings.valley_weight
                + terrain_noise * settings.terrain_noise_weight;

            raw_elevations[index] = raw_elevation;
            min_elevation = std::min(min_elevation, raw_elevation);
            max_elevation = std::max(max_elevation, raw_elevation);

            auto& cell = map.cells[index];
            cell.x = static_cast<std::int32_t>(x);
            cell.y = static_cast<std::int32_t>(y);
        }
    }

    const float elevation_range = max_elevation - min_elevation;
    for (std::size_t i = 0; i < map.cells.size(); ++i) {
        auto& cell = map.cells[i];
        cell.elevation = elevation_range > 0.0f ? clamp01((raw_elevations[i] - min_elevation) / elevation_range) : 0.0f;
        cell.is_water = cell.elevation <= settings.sea_level;
        cell.is_ocean = cell.is_water;
    }

    compute_coast_distance(map);
    compute_slopes(map);
    classify_cells(map, settings);

    return map;
}

} // namespace procgen
