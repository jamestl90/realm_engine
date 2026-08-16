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
constexpr float MIN_LAND_RELIEF = 0.01f;
constexpr float HILL_RELIEF_SCALE = 0.20f;
constexpr float MOUNTAIN_RELIEF_SCALE = 0.74f;
constexpr float RIDGE_RELIEF_SCALE = 0.14f;
constexpr float VALLEY_RELIEF_SCALE = 0.12f;
constexpr float TERRAIN_NOISE_RELIEF_SCALE = 0.10f;
constexpr float COASTLINE_DETAIL_SUPPORT = 0.20f;

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
            cell.terrain_form = cell.is_ocean
                ? TerrainForm::Ocean
                : TerrainForm::InlandWater;
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
        case TerrainForm::InlandWater:
            return "inland water";
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
    return form == TerrainForm::Ocean || form == TerrainForm::InlandWater;
}

GreaterRealmTerrainCharacter derive_greater_realm_terrain_character(
    const GreaterRealmGeneratorSettings& settings
) noexcept {
    const float variation = clamp01(settings.seed_terrain_variation);
    if (variation == 0.0f) {
        return {};
    }

    const float sample = random01(
        settings.seed,
        0,
        0,
        0x5445525241494e43ull
    );
    const float centered = sample * 2.0f - 1.0f;
    const float shaped = 0.5f + 0.5f * std::copysign(
        std::pow(std::abs(centered), 0.70f),
        centered
    );
    const float ruggedness = lerp(0.5f, shaped, variation);

    GreaterRealmTerrainCharacter character;
    character.ruggedness = ruggedness;
    character.base_relief_scale = std::pow(2.0f, (ruggedness - 0.5f) * 2.0f);
    character.mountain_relief_scale = std::pow(4.0f, (ruggedness - 0.5f) * 2.0f);
    character.mountain_coverage_scale = std::pow(3.0f, (ruggedness - 0.5f) * 2.0f);
    character.detail_scale = std::pow(1.75f, (ruggedness - 0.5f) * 2.0f);
    character.peak_spacing_scale = 1.0f - (ruggedness - 0.5f) * 0.90f;
    character.peak_radius_scale = 1.0f + (ruggedness - 0.5f) * 1.20f;
    return character;
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

namespace {

#if defined(REALM_ENABLE_PROCGEN_PROFILING)
using RegenerationClock = std::chrono::steady_clock;
#endif
using TerrainLayers = GreaterRealmGenerationCache::TerrainLayers;

[[nodiscard]] GreaterRealmDirtyStage expand_dirty_stages(
    GreaterRealmDirtyStage stages
) noexcept {
    if (has_dirty_stage(stages, GreaterRealmDirtyStage::TerrainFields)) {
        stages |= GreaterRealmDirtyStage::MountainPeaks
            | GreaterRealmDirtyStage::Relief
            | GreaterRealmDirtyStage::Classification
            | GreaterRealmDirtyStage::Drainage
            | GreaterRealmDirtyStage::RiverChannels;
    }
    if (has_dirty_stage(stages, GreaterRealmDirtyStage::MountainPeaks)) {
        stages |= GreaterRealmDirtyStage::Relief
            | GreaterRealmDirtyStage::Classification
            | GreaterRealmDirtyStage::Drainage
            | GreaterRealmDirtyStage::RiverChannels;
    }
    if (has_dirty_stage(stages, GreaterRealmDirtyStage::Relief)) {
        stages |= GreaterRealmDirtyStage::Classification
            | GreaterRealmDirtyStage::Drainage
            | GreaterRealmDirtyStage::RiverChannels;
    }
    if (has_dirty_stage(stages, GreaterRealmDirtyStage::Drainage)) {
        stages |= GreaterRealmDirtyStage::RiverChannels;
    }
    constexpr GreaterRealmDirtyStage generated_map_stages =
        GreaterRealmDirtyStage::TerrainFields
        | GreaterRealmDirtyStage::MountainPeaks
        | GreaterRealmDirtyStage::Relief
        | GreaterRealmDirtyStage::Classification
        | GreaterRealmDirtyStage::Drainage
        | GreaterRealmDirtyStage::RiverChannels;
    if (has_dirty_stage(stages, generated_map_stages)) {
        stages |= GreaterRealmDirtyStage::DebugImage;
    }
    if (has_dirty_stage(stages, GreaterRealmDirtyStage::DebugImage)) {
        stages |= GreaterRealmDirtyStage::TextureUpload;
    }
    return stages;
}

[[nodiscard]] GreaterRealmDirtyStage settings_dirty_stage(
    const GreaterRealmGeneratorSettings& previous,
    const GreaterRealmGeneratorSettings& current
) noexcept {
    GreaterRealmDirtyStage stages = GreaterRealmDirtyStage::None;

    if (previous.seed != current.seed
        || previous.width != current.width
        || previous.height != current.height
        || previous.cell_size != current.cell_size
        || previous.sea_level != current.sea_level
        || previous.base_elevation_frequency != current.base_elevation_frequency
        || previous.ridge_frequency != current.ridge_frequency
        || previous.valley_frequency != current.valley_frequency
        || previous.terrain_noise_frequency != current.terrain_noise_frequency
        || previous.ocean_noise_frequency != current.ocean_noise_frequency
        || previous.island_bias != current.island_bias
        || previous.coastline_noise_weight != current.coastline_noise_weight) {
        stages |= GreaterRealmDirtyStage::TerrainFields;
    }

    if (previous.seed_terrain_variation != current.seed_terrain_variation
        || previous.mountain_peak_spacing != current.mountain_peak_spacing
        || previous.mountain_peak_radius != current.mountain_peak_radius
        || previous.mountain_peak_jaggedness != current.mountain_peak_jaggedness) {
        stages |= GreaterRealmDirtyStage::MountainPeaks;
    }

    if (previous.base_elevation_weight != current.base_elevation_weight
        || previous.mountain_weight != current.mountain_weight
        || previous.ridge_weight != current.ridge_weight
        || previous.valley_weight != current.valley_weight
        || previous.terrain_noise_weight != current.terrain_noise_weight
        || previous.ocean_depth_weight != current.ocean_depth_weight) {
        stages |= GreaterRealmDirtyStage::Relief;
    }

    if (previous.mountain_threshold != current.mountain_threshold
        || previous.highland_threshold != current.highland_threshold
        || previous.hill_threshold != current.hill_threshold) {
        stages |= GreaterRealmDirtyStage::Classification;
    }

    if (previous.river_min_drainage_area != current.river_min_drainage_area
        || previous.river_width_scale != current.river_width_scale) {
        stages |= GreaterRealmDirtyStage::RiverChannels;
    }

    return stages;
}

void build_terrain_fields(
    GreaterRealmMap& map,
    std::vector<TerrainLayers>& layers,
    const GreaterRealmGeneratorSettings& settings,
    const TerrainConstraintField* constraints
) {
    map.seed = settings.seed;
    map.width = std::max<std::uint32_t>(settings.width, 1);
    map.height = std::max<std::uint32_t>(settings.height, 1);
    map.cell_size = settings.cell_size > 0.0f ? settings.cell_size : 1.0f;
    map.cells.assign(map.expected_cell_count(), GreaterRealmCell{});
    layers.resize(map.cells.size());

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
            float automatic_constraint = std::clamp(
                0.5f * (land_noise + island_constraint * island_bias),
                -1.0f,
                1.0f
            );
            if (automatic_constraint > 0.0f) {
                const float mountain_hint_noise =
                    (value_noise(settings.seed, centered_x + 30.0f, centered_y + 50.0f, 1.0f, 701ull) * 2.0f - 1.0f) * 0.5f
                    + (value_noise(settings.seed, centered_x * 2.0f + 33.0f, centered_y * 2.0f + 55.0f, 1.0f, 709ull) * 2.0f - 1.0f) * 0.5f;
                const float mountain_hint = std::min(1.0f, automatic_constraint * 5.0f)
                    * (1.0f - std::abs(mountain_hint_noise) / 0.5f);
                if (mountain_hint > 0.0f) {
                    automatic_constraint = std::max(
                        automatic_constraint,
                        std::min(automatic_constraint * 3.0f, mountain_hint)
                    );
                }
            }

            const TerrainConstraintSample authored = constraints
                ? constraints->sample(u, v)
                : TerrainConstraintSample{};
            const float authored_influence = clamp01(authored.influence);
            const float broad_constraint = lerp(
                automatic_constraint,
                std::clamp(authored.elevation, -1.0f, 1.0f),
                authored_influence
            );

            const float coastline_noise4 = value_noise(
                settings.seed,
                centered_x + 15.0f / 16.0f,
                centered_y + 15.0f / 16.0f,
                16.0f,
                59ull
            ) * 2.0f - 1.0f;
            const float coastline_noise5 = value_noise(
                settings.seed,
                centered_x + 31.0f / 32.0f,
                centered_y + 31.0f / 32.0f,
                32.0f,
                59ull
            ) * 2.0f - 1.0f;
            const float coastline_noise6 = value_noise(
                settings.seed,
                centered_x + 67.0f / 64.0f,
                centered_y + 67.0f / 64.0f,
                64.0f,
                59ull
            ) * 2.0f - 1.0f;
            const float coastline_noise = coastline_noise4
                + coastline_noise5 * 0.5f
                + coastline_noise6 * 0.25f;
            const float coastline_attenuation = 1.0f - smoothstep(
                0.0f,
                COASTLINE_DETAIL_SUPPORT,
                std::abs(broad_constraint)
            );
            const float landmass_elevation = std::clamp(
                broad_constraint
                    + coastline_noise * std::max(settings.coastline_noise_weight, 0.0f) * coastline_attenuation,
                -1.0f,
                1.0f
            );

            const float base_elevation = fbm(settings.seed, u, v, settings.base_elevation_frequency, 101ull, 5);
            const float mountain_mask = smoothstep(0.05f, 0.85f, broad_constraint);
            const float ridge_influence = std::pow(
                ridged_noise(settings.seed, u, v, settings.ridge_frequency, 307ull, 3),
                3.0f
            ) * mountain_mask;
            const float valley_influence = std::pow(
                ridged_noise(settings.seed, u, v, settings.valley_frequency, 401ull, 4),
                2.0f
            ) * smoothstep(0.02f, 0.75f, broad_constraint);
            const float terrain_noise = fbm(
                settings.seed,
                u,
                v,
                settings.terrain_noise_frequency,
                503ull,
                3
            ) - 0.5f;
            const float ocean_noise = fbm(
                settings.seed,
                u,
                v,
                settings.ocean_noise_frequency,
                601ull,
                3
            ) * 2.0f - 1.0f;

            layers[index] = TerrainLayers{
                base_elevation,
                ridge_influence,
                valley_influence,
                terrain_noise,
                ocean_noise
            };

            auto& cell = map.cells[index];
            cell.x = static_cast<std::int32_t>(x);
            cell.y = static_cast<std::int32_t>(y);
            cell.landmass_elevation = landmass_elevation;
            cell.relief_constraint = broad_constraint;
            cell.is_water = landmass_elevation <= 0.0f;
        }
    }
}

void compose_relief(
    GreaterRealmMap& map,
    const std::vector<TerrainLayers>& layers,
    const GreaterRealmGeneratorSettings& settings
) {
    const auto& character = map.terrain_character;
    const float base_weight = std::max(settings.base_elevation_weight, 0.0f) * character.base_relief_scale;
    const float mountain_weight = std::max(settings.mountain_weight, 0.0f) * character.mountain_relief_scale;
    const float ridge_weight = std::max(settings.ridge_weight, 0.0f) * character.detail_scale;
    const float valley_weight = std::max(settings.valley_weight, 0.0f) * character.detail_scale;
    const float terrain_noise_weight = std::max(settings.terrain_noise_weight, 0.0f) * character.detail_scale;

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
            cell.hill_relief = 0.0f;
            cell.mountain_relief = 0.0f;
            cell.elevation = NORMALIZED_WATERLINE * (1.0f - depth);
            continue;
        }

        const float positive_constraint = clamp01(cell.relief_constraint);
        const float constraint_blend = clamp01(
            positive_constraint * positive_constraint * character.mountain_coverage_scale
        );
        const float hill_relief = clamp01(
            MIN_LAND_RELIEF + layer.base_elevation * base_weight * HILL_RELIEF_SCALE
        );
        const float mountain_profile = std::pow(clamp01(cell.mountain_influence), 0.75f);
        const float mountain_relief = clamp01(
            hill_relief + mountain_profile * mountain_weight * MOUNTAIN_RELIEF_SCALE
        );

        float relief = lerp(hill_relief, mountain_relief, constraint_blend);
        const float extension_mask = smoothstep(0.02f, 0.65f, positive_constraint);
        const float ridge_extension =
            layer.ridge * ridge_weight * RIDGE_RELIEF_SCALE * extension_mask;
        const float valley_extension =
            layer.valley * valley_weight * VALLEY_RELIEF_SCALE * extension_mask;
        const float noise_extension =
            layer.terrain_noise * terrain_noise_weight * TERRAIN_NOISE_RELIEF_SCALE * extension_mask;
        relief = clamp01(relief + ridge_extension - valley_extension + noise_extension);
        relief = std::max(relief, MIN_LAND_RELIEF);

        cell.hill_relief = hill_relief;
        cell.mountain_relief = mountain_relief;
        const float inland_influence = smoothstep(0.0f, 0.45f, positive_constraint);
        const float coastal_rise = 0.01f + 0.14f * clamp01(positive_constraint / 0.45f);
        const float land_height = clamp01(lerp(coastal_rise, relief, inland_influence));
        cell.elevation =
            NORMALIZED_WATERLINE + (1.0f - NORMALIZED_WATERLINE) * land_height;
    }
}

void refresh_geography_and_classification(
    GreaterRealmMap& map,
    const GreaterRealmGeneratorSettings& settings
) {
    for (auto& cell : map.cells) {
        cell.is_ocean = false;
    }
    classify_oceans(map);
    compute_coast_distance(map);
    compute_slopes(map);
    classify_cells(map, settings);
}

template <typename Function>
double measure_stage(Function&& function) {
#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const auto started_at = RegenerationClock::now();
    function();
    return std::chrono::duration<double, std::milli>(
        RegenerationClock::now() - started_at
    ).count();
#else
    function();
    return 0.0;
#endif
}

} // namespace

void GreaterRealmGenerationCache::invalidate(GreaterRealmDirtyStage stage) noexcept {
    m_pending_stages |= stage;
}

void GreaterRealmGenerationCache::reset() noexcept {
    m_settings = GreaterRealmGeneratorSettings{};
    m_layers.clear();
    m_pending_stages = GreaterRealmDirtyStage::TerrainFields;
    m_initialized = false;
    m_constraints = nullptr;
}

GreaterRealmRegenerationResult GreaterRealmGenerationCache::regenerate(
    GreaterRealmMap& map,
    const GreaterRealmGeneratorSettings& settings
) {
    return regenerate_impl(map, settings, nullptr);
}

GreaterRealmRegenerationResult GreaterRealmGenerationCache::regenerate(
    GreaterRealmMap& map,
    const GreaterRealmGeneratorSettings& settings,
    const TerrainConstraintField& constraints
) {
    return regenerate_impl(map, settings, &constraints);
}

GreaterRealmRegenerationResult GreaterRealmGenerationCache::regenerate_impl(
    GreaterRealmMap& map,
    const GreaterRealmGeneratorSettings& settings,
    const TerrainConstraintField* constraints
) {
#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    const auto total_started_at = RegenerationClock::now();
#endif
    GreaterRealmDirtyStage dirty_stages = m_pending_stages;

    if (!m_initialized
        || !map.has_expected_cell_count()
        || m_layers.size() != map.cells.size()) {
        dirty_stages |= GreaterRealmDirtyStage::TerrainFields;
    } else {
        dirty_stages |= settings_dirty_stage(m_settings, settings);
    }
    if (constraints != m_constraints) {
        dirty_stages |= GreaterRealmDirtyStage::TerrainFields;
    }

    dirty_stages = expand_dirty_stages(dirty_stages);
    map.terrain_character = derive_greater_realm_terrain_character(settings);

    GreaterRealmRegenerationResult result;
    result.rebuilt_stages = dirty_stages;
    if (dirty_stages == GreaterRealmDirtyStage::None) {
        return result;
    }

    if (has_dirty_stage(dirty_stages, GreaterRealmDirtyStage::TerrainFields)) {
        result.timings.terrain_fields_ms = measure_stage([&]() {
            build_terrain_fields(map, m_layers, settings, constraints);
        });
    }

    if (has_dirty_stage(dirty_stages, GreaterRealmDirtyStage::MountainPeaks)) {
        result.timings.mountain_peaks_ms = measure_stage([&]() {
            generate_mountain_peak_field(map, settings);
        });
    }

    if (has_dirty_stage(dirty_stages, GreaterRealmDirtyStage::Relief)) {
        result.timings.relief_ms = measure_stage([&]() {
            compose_relief(map, m_layers, settings);
        });
    }

    if (has_dirty_stage(dirty_stages, GreaterRealmDirtyStage::Classification)) {
        result.timings.classification_ms = measure_stage([&]() {
            if (has_dirty_stage(dirty_stages, GreaterRealmDirtyStage::TerrainFields)) {
                refresh_geography_and_classification(map, settings);
            } else if (has_dirty_stage(dirty_stages, GreaterRealmDirtyStage::Relief)) {
                compute_slopes(map);
                classify_cells(map, settings);
            } else {
                classify_cells(map, settings);
            }
        });
    }

    if (has_dirty_stage(dirty_stages, GreaterRealmDirtyStage::Drainage)) {
        result.timings.drainage_ms = measure_stage([&]() {
            build_greater_realm_drainage(map);
        });
    }

    if (has_dirty_stage(dirty_stages, GreaterRealmDirtyStage::RiverChannels)) {
        result.timings.river_channels_ms = measure_stage([&]() {
            build_greater_realm_river_channels(map, settings);
        });
    }

    m_settings = settings;
    m_constraints = constraints;
    m_pending_stages = GreaterRealmDirtyStage::None;
    m_initialized = true;
#if defined(REALM_ENABLE_PROCGEN_PROFILING)
    result.timings.total_ms = std::chrono::duration<double, std::milli>(
        RegenerationClock::now() - total_started_at
    ).count();
#endif

#if defined(REALM_ENABLE_PROCGEN_PROFILING) && !defined(REALM_TEST_BUILD)
    std::fprintf(
        stderr,
        "DEBUG: Procgen regeneration stages=0x%02x fields=%.2fms peaks=%.2fms relief=%.2fms classify=%.2fms drainage=%.2fms channels=%.2fms total=%.2fms\n",
        static_cast<unsigned>(dirty_stages),
        result.timings.terrain_fields_ms,
        result.timings.mountain_peaks_ms,
        result.timings.relief_ms,
        result.timings.classification_ms,
        result.timings.drainage_ms,
        result.timings.river_channels_ms,
        result.timings.total_ms
    );
#endif

    return result;
}

GreaterRealmMap generate_greater_realm(const GreaterRealmGeneratorSettings& settings) {
    GreaterRealmMap map;
    GreaterRealmGenerationCache cache;
    (void)cache.regenerate(map, settings);
    return map;
}

GreaterRealmMap generate_greater_realm(
    const GreaterRealmGeneratorSettings& settings,
    const TerrainConstraintField& constraints
) {
    GreaterRealmMap map;
    GreaterRealmGenerationCache cache;
    (void)cache.regenerate(map, settings, constraints);
    return map;
}

} // namespace procgen
