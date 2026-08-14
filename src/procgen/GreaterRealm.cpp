#include "../../include/procgen/GreaterRealm.hpp"
#include "../../include/procgen/Hydrology.hpp"
#include "../../include/procgen/MountainPeaks.hpp"
#include "../../include/procgen/TerrainConstraints.hpp"
#include <algorithm>
#include <array>
#include <cmath>

#if defined(REALM_ENABLE_PROCGEN_PROFILING)
#include <chrono>
#include <cstdio>
#endif

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

void classify_oceans(GreaterRealmMap& map) {
    std::vector<std::size_t> open;
    open.reserve(static_cast<std::size_t>(map.width) * 2 + static_cast<std::size_t>(map.height) * 2);

    const auto enqueue = [&](std::uint32_t x, std::uint32_t y) {
        auto& cell = map.cells[map.index(x, y)];
        if (!cell.is_water || cell.is_ocean) {
            return;
        }

        cell.is_ocean = true;
        open.push_back(map.index(x, y));
    };

    for (std::uint32_t x = 0; x < map.width; ++x) {
        enqueue(x, 0);
        enqueue(x, map.height - 1);
    }
    for (std::uint32_t y = 0; y < map.height; ++y) {
        enqueue(0, y);
        enqueue(map.width - 1, y);
    }

    constexpr std::array<std::array<std::int32_t, 2>, 4> neighbors{{
        {{-1, 0}},
        {{1, 0}},
        {{0, -1}},
        {{0, 1}}
    }};

    while (!open.empty()) {
        const std::size_t index = open.back();
        open.pop_back();

        const auto& cell = map.cells[index];
        for (const auto& offset : neighbors) {
            const std::int32_t neighbor_x = cell.x + offset[0];
            const std::int32_t neighbor_y = cell.y + offset[1];
            if (!map.contains(neighbor_x, neighbor_y)) {
                continue;
            }

            enqueue(static_cast<std::uint32_t>(neighbor_x), static_cast<std::uint32_t>(neighbor_y));
        }
    }
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
        auto& cell = map.cells[i];
        cell.is_coastal = !cell.is_water && distances[i] == 0.0f;
        cell.distance_to_coast = distances[i] == INF_DISTANCE ? 0.0f : distances[i];
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

        if (cell.elevation >= settings.mountain_threshold) {
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

static GreaterRealmMap generate_greater_realm_impl(
    const GreaterRealmGeneratorSettings& settings,
    const TerrainConstraintField* constraints
) {
#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    using ProfileClock = std::chrono::steady_clock;
    const auto profile_started_at = ProfileClock::now();
    auto profile_last_mark = profile_started_at;
    const auto profile_stage = [&profile_last_mark]() {
        const auto now = ProfileClock::now();
        const double elapsed_ms = std::chrono::duration<double, std::milli>(now - profile_last_mark).count();
        profile_last_mark = now;
        return elapsed_ms;
    };
#endif

    GreaterRealmMap map;
    map.seed = settings.seed;
    map.width = std::max<std::uint32_t>(settings.width, 1);
    map.height = std::max<std::uint32_t>(settings.height, 1);
    map.cell_size = settings.cell_size > 0.0f ? settings.cell_size : 1.0f;
    map.cells.resize(map.expected_cell_count());

    struct TerrainLayers {
        float base_elevation{0.0f};
        float ridge{0.0f};
        float valley{0.0f};
        float terrain_noise{0.0f};
        float ocean_noise{0.0f};
        float constraint_elevation{0.0f};
        float constraint_influence{0.0f};
    };

    std::vector<TerrainLayers> layers(map.cells.size());

    const float sea_level = std::clamp(settings.sea_level, 0.01f, 0.99f);
    const float sea_level_offset = (0.5f - sea_level) * 2.0f;
    const float island_bias = std::clamp(settings.island_bias, 0.0f, 1.0f);

    for (std::uint32_t y = 0; y < map.height; ++y) {
        for (std::uint32_t x = 0; x < map.width; ++x) {
            const auto index = map.index(x, y);
            const float u = normalized_x(x, map.width);
            const float v = normalized_y(y, map.height);
            const float centered_x = u * 2.0f - 1.0f;
            const float centered_y = v * 2.0f - 1.0f;
            const float square_distance = std::max(std::abs(centered_x), std::abs(centered_y));
            const float land_noise = fbm(
                settings.seed,
                centered_x,
                centered_y,
                1.0f,
                11ull,
                5
            ) * 2.0f - 1.0f;
            const float island_constraint = 0.75f - 2.0f * square_distance * square_distance;
            const float automatic_constraint = std::clamp(
                0.5f * (land_noise + island_constraint * island_bias)
                + sea_level_offset,
                -1.0f,
                1.0f
            );
            const TerrainConstraintSample authored = constraints
                ? constraints->sample(u, v)
                : TerrainConstraintSample{};
            const float authored_influence = clamp01(authored.influence);
            const float broad_constraint = lerp(
                automatic_constraint,
                std::clamp(authored.elevation, -1.0f, 1.0f),
                authored_influence
            );

            const float coastline_noise = fbm(
                settings.seed,
                u,
                v,
                settings.coastline_noise_frequency,
                59ull,
                4
            ) * 2.0f - 1.0f;
            const float coastline_proximity = 1.0f - smoothstep(
                0.0f,
                0.30f,
                std::abs(broad_constraint)
            );
            const float landmass_elevation = std::clamp(
                broad_constraint
                    + coastline_noise * std::max(settings.coastline_noise_weight, 0.0f) * coastline_proximity,
                -1.0f,
                1.0f
            );

            const float base_elevation = fbm(settings.seed, u, v, settings.base_elevation_frequency, 101ull, 5);
            const float mountain_mask = smoothstep(0.05f, 0.85f, landmass_elevation);
            const float ridge_influence = std::pow(ridged_noise(settings.seed, u, v, settings.ridge_frequency, 307ull, 3), 3.0f) * mountain_mask;
            const float valley_influence = std::pow(ridged_noise(settings.seed, u, v, settings.valley_frequency, 401ull, 4), 2.0f) * smoothstep(0.02f, 0.75f, landmass_elevation);
            const float terrain_noise = fbm(settings.seed, u, v, settings.terrain_noise_frequency, 503ull, 3) - 0.5f;
            const float ocean_noise = fbm(settings.seed, u, v, settings.ocean_noise_frequency, 601ull, 3) * 2.0f - 1.0f;

            layers[index] = TerrainLayers{
                base_elevation,
                ridge_influence,
                valley_influence,
                terrain_noise,
                ocean_noise,
                authored.elevation,
                authored_influence
            };

            auto& cell = map.cells[index];
            cell.x = static_cast<std::int32_t>(x);
            cell.y = static_cast<std::int32_t>(y);
            cell.landmass_elevation = landmass_elevation;
            cell.is_water = landmass_elevation <= 0.0f;
        }
    }

#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const double terrain_fields_ms = profile_stage();
#endif

    generate_mountain_peak_field(map, settings);

#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const double mountain_peaks_ms = profile_stage();
#endif

    const float base_weight = std::max(settings.base_elevation_weight, 0.0f);
    const float mountain_weight = std::max(settings.mountain_weight, 0.0f);
    const float ridge_weight = std::max(settings.ridge_weight, 0.0f);
    const float valley_weight = std::max(settings.valley_weight, 0.0f);
    const float terrain_noise_weight = std::max(settings.terrain_noise_weight, 0.0f);
    const float relief_min = -valley_weight;
    const float relief_max = base_weight + DEFAULT_MOUNTAIN_STRENGTH + ridge_weight;
    const float relief_range = std::max(relief_max - relief_min, 0.0001f);

    for (std::size_t i = 0; i < map.cells.size(); ++i) {
        auto& cell = map.cells[i];
        const auto& layer = layers[i];

        if (cell.is_water) {
            const float depth_variation = 1.0f + layer.ocean_noise * 0.20f;
            const float depth = clamp01(
                -cell.landmass_elevation
                * std::max(settings.ocean_depth_weight, 0.0f)
                * std::max(depth_variation, 0.0f)
            );
            cell.elevation = sea_level * (1.0f - depth);
            continue;
        }

        const float raw_relief =
            layer.base_elevation * base_weight
            + cell.mountain_influence * mountain_weight
            + layer.ridge * ridge_weight
            - layer.valley * valley_weight;
        const float normalized_relief = clamp01((raw_relief - relief_min) / relief_range);
        float relief = clamp01(normalized_relief + layer.terrain_noise * terrain_noise_weight);
        if (layer.constraint_influence > 0.0f) {
            const float constrained_relief = clamp01(layer.constraint_elevation);
            relief = lerp(relief, constrained_relief, layer.constraint_influence);
        }
        const float inland_influence = smoothstep(0.0f, 0.45f, cell.landmass_elevation);
        const float coastal_rise = 0.01f + 0.14f * clamp01(cell.landmass_elevation / 0.45f);
        const float land_height = clamp01(lerp(coastal_rise, relief, inland_influence));
        cell.elevation = sea_level + (1.0f - sea_level) * land_height;
    }

#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const double relief_ms = profile_stage();
#endif

    classify_oceans(map);
    compute_coast_distance(map);
    compute_slopes(map);
    classify_cells(map, settings);

#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const double classification_ms = profile_stage();
#endif

    build_greater_realm_drainage(map);

#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const double drainage_ms = profile_stage();
#endif

    build_greater_realm_river_channels(map, settings);

#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const double rivers_ms = profile_stage();
    const double total_ms = std::chrono::duration<double, std::milli>(
        ProfileClock::now() - profile_started_at
    ).count();
    std::fprintf(
        stderr,
        "DEBUG: Procgen stages: fields=%.2fms peaks=%.2fms relief=%.2fms classify=%.2fms drainage=%.2fms channels=%.2fms total=%.2fms\n",
        terrain_fields_ms,
        mountain_peaks_ms,
        relief_ms,
        classification_ms,
        drainage_ms,
        rivers_ms,
        total_ms
    );
#endif

    return map;
}

GreaterRealmMap generate_greater_realm(const GreaterRealmGeneratorSettings& settings) {
    return generate_greater_realm_impl(settings, nullptr);
}

GreaterRealmMap generate_greater_realm(
    const GreaterRealmGeneratorSettings& settings,
    const TerrainConstraintField& constraints
) {
    return generate_greater_realm_impl(settings, &constraints);
}

} // namespace procgen
