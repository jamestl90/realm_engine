#include "../../include/procgen/Climate.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <utility>

namespace procgen {
namespace {

constexpr std::uint64_t TEMPERATURE_NOISE_DOMAIN = 0x74656d7065726174ull;

[[nodiscard]] float clamp01(float value) noexcept {
    return std::clamp(value, 0.0f, 1.0f);
}

[[nodiscard]] float lerp(float from, float to, float amount) noexcept {
    return from + (to - from) * amount;
}

[[nodiscard]] float smoothstep01(float value) noexcept {
    const float clamped = clamp01(value);
    return clamped * clamped * (3.0f - 2.0f * clamped);
}

[[nodiscard]] std::uint64_t mix_hash(std::uint64_t hash, std::uint64_t value) noexcept {
    hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
    hash ^= hash >> 30;
    hash *= 0xbf58476d1ce4e5b9ull;
    hash ^= hash >> 27;
    hash *= 0x94d049bb133111ebull;
    return hash ^ (hash >> 31);
}

[[nodiscard]] std::uint64_t hash_coords(
    Seed seed,
    std::int32_t x,
    std::int32_t y,
    std::uint64_t salt
) noexcept {
    std::uint64_t hash = mix_hash(seed, salt);
    hash = mix_hash(hash, static_cast<std::uint32_t>(x));
    return mix_hash(hash, static_cast<std::uint32_t>(y));
}

[[nodiscard]] float random01(
    Seed seed,
    std::int32_t x,
    std::int32_t y,
    std::uint64_t salt
) noexcept {
    return static_cast<float>((hash_coords(seed, x, y, salt) >> 40) & 0xffffffu)
        / static_cast<float>(0xffffffu);
}

[[nodiscard]] float value_noise(
    Seed seed,
    float x,
    float y,
    float frequency,
    std::uint64_t salt
) noexcept {
    const float sample_x = x * frequency;
    const float sample_y = y * frequency;
    const auto x0 = static_cast<std::int32_t>(std::floor(sample_x));
    const auto y0 = static_cast<std::int32_t>(std::floor(sample_y));
    const float tx = smoothstep01(sample_x - static_cast<float>(x0));
    const float ty = smoothstep01(sample_y - static_cast<float>(y0));
    const float low = lerp(
        random01(seed, x0, y0, salt),
        random01(seed, x0 + 1, y0, salt),
        tx
    );
    const float high = lerp(
        random01(seed, x0, y0 + 1, salt),
        random01(seed, x0 + 1, y0 + 1, salt),
        tx
    );
    return lerp(low, high, ty);
}

[[nodiscard]] float broad_temperature_noise(
    Seed seed,
    float x,
    float y,
    float frequency
) noexcept {
    float total = 0.0f;
    float amplitude = 1.0f;
    float amplitude_sum = 0.0f;
    for (std::uint32_t octave = 0; octave < 3; ++octave) {
        total += value_noise(
            seed,
            x,
            y,
            frequency,
            TEMPERATURE_NOISE_DOMAIN + octave * 1013ull
        ) * amplitude;
        amplitude_sum += amplitude;
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    return amplitude_sum > 0.0f ? total / amplitude_sum : 0.5f;
}

[[nodiscard]] std::vector<float> distance_to_water(const GreaterRealmMap& terrain) {
    constexpr float SQRT_TWO = 1.41421356237f;
    constexpr std::array<std::array<std::int32_t, 2>, 8> NEIGHBORS{{
        {{-1, -1}}, {{0, -1}}, {{1, -1}}, {{-1, 0}},
        {{1, 0}}, {{-1, 1}}, {{0, 1}}, {{1, 1}}
    }};
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

        const std::int32_t x = static_cast<std::int32_t>(index % terrain.width);
        const std::int32_t y = static_cast<std::int32_t>(index / terrain.width);
        for (const auto& offset : NEIGHBORS) {
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

} // namespace

std::size_t GreaterRealmClimateMap::expected_cell_count() const noexcept {
    return static_cast<std::size_t>(source_width) * static_cast<std::size_t>(source_height);
}

bool GreaterRealmClimateMap::has_expected_cell_count() const noexcept {
    return cells.size() == expected_cell_count();
}

bool GreaterRealmClimateMap::source_matches(const GreaterRealmMap& map) const noexcept {
    return version == GREATER_REALM_CLIMATE_VERSION
        && source_seed == map.seed
        && source_width == map.width
        && source_height == map.height
        && source_cell_size == map.cell_size
        && map.has_expected_cell_count()
        && has_expected_cell_count()
        && source_terrain_fingerprint == greater_realm_climate_source_fingerprint(map);
}

GreaterRealmClimateSettings clamp_greater_realm_climate_settings(
    const GreaterRealmClimateSettings& settings
) noexcept {
    GreaterRealmClimateSettings clamped = settings;
    clamped.north_edge_latitude_degrees = std::clamp(
        clamped.north_edge_latitude_degrees, -90.0f, 90.0f
    );
    clamped.south_edge_latitude_degrees = std::clamp(
        clamped.south_edge_latitude_degrees, -90.0f, 90.0f
    );
    clamped.elevation_cooling = std::clamp(clamped.elevation_cooling, 0.0f, 1.0f);
    clamped.maritime_moderation = std::clamp(clamped.maritime_moderation, 0.0f, 1.0f);
    clamped.maritime_influence_distance = std::max(clamped.maritime_influence_distance, 0.0f);
    clamped.temperature_variation = std::clamp(clamped.temperature_variation, 0.0f, 0.5f);
    clamped.temperature_variation_frequency = std::clamp(
        clamped.temperature_variation_frequency, 0.01f, 32.0f
    );
    return clamped;
}

float greater_realm_latitude_for_row(
    const GreaterRealmClimateSettings& settings,
    std::uint32_t row,
    std::uint32_t height
) noexcept {
    const auto clamped = clamp_greater_realm_climate_settings(settings);
    const float amount = height > 1
        ? static_cast<float>(std::min(row, height - 1)) / static_cast<float>(height - 1)
        : 0.5f;
    return lerp(
        clamped.north_edge_latitude_degrees,
        clamped.south_edge_latitude_degrees,
        amount
    );
}

std::uint64_t greater_realm_climate_source_fingerprint(const GreaterRealmMap& map) noexcept {
    std::uint64_t hash = mix_hash(map.seed, map.width);
    hash = mix_hash(hash, map.height);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(map.cell_size));
    for (const auto& cell : map.cells) {
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(cell.elevation));
        hash = mix_hash(hash, cell.is_water ? 1u : 0u);
    }
    return hash;
}

GreaterRealmClimateMap generate_greater_realm_climate(
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateSettings& settings
) {
    GreaterRealmClimateMap climate;
    climate.source_seed = terrain.seed;
    climate.source_width = terrain.width;
    climate.source_height = terrain.height;
    climate.source_cell_size = terrain.cell_size;
    climate.source_terrain_fingerprint = greater_realm_climate_source_fingerprint(terrain);
    if (!terrain.has_expected_cell_count()) {
        return climate;
    }

    const auto clamped = clamp_greater_realm_climate_settings(settings);
    const auto water_distances = distance_to_water(terrain);
    climate.cells.resize(terrain.cells.size());

    for (std::uint32_t y = 0; y < terrain.height; ++y) {
        const float latitude = greater_realm_latitude_for_row(clamped, y, terrain.height);
        const float latitude_temperature = 1.0f - std::abs(latitude) / 90.0f;
        for (std::uint32_t x = 0; x < terrain.width; ++x) {
            const std::size_t index = terrain.index(x, y);
            const float normalized_x = terrain.width > 1
                ? static_cast<float>(x) / static_cast<float>(terrain.width - 1)
                : 0.5f;
            const float normalized_y = terrain.height > 1
                ? static_cast<float>(y) / static_cast<float>(terrain.height - 1)
                : 0.5f;
            const float noise = broad_temperature_noise(
                terrain.seed,
                normalized_x,
                normalized_y,
                clamped.temperature_variation_frequency
            ) * 2.0f - 1.0f;
            float temperature = latitude_temperature
                + noise * clamped.temperature_variation;

            if (!terrain.cells[index].is_water) {
                const float land_height = clamp01(
                    (terrain.cells[index].elevation - NORMALIZED_WATERLINE)
                    / (1.0f - NORMALIZED_WATERLINE)
                );
                temperature -= land_height * clamped.elevation_cooling;
            }

            float maritime_influence = 0.0f;
            if (clamped.maritime_influence_distance > 0.0f
                && std::isfinite(water_distances[index])) {
                maritime_influence = 1.0f - smoothstep01(
                    water_distances[index] / clamped.maritime_influence_distance
                );
            }
            temperature = lerp(
                temperature,
                0.5f,
                maritime_influence * clamped.maritime_moderation
            );
            climate.cells[index].temperature_normal = clamp01(temperature);
        }
    }
    return climate;
}

TemperatureNormalSummary summarize_temperature_normals(
    const GreaterRealmClimateMap& climate
) noexcept {
    TemperatureNormalSummary summary;
    if (!climate.has_expected_cell_count() || climate.cells.empty()) {
        return summary;
    }

    summary.minimum = std::numeric_limits<float>::infinity();
    summary.maximum = -std::numeric_limits<float>::infinity();
    double total = 0.0;
    for (const auto& cell : climate.cells) {
        summary.minimum = std::min(summary.minimum, cell.temperature_normal);
        summary.maximum = std::max(summary.maximum, cell.temperature_normal);
        total += cell.temperature_normal;
    }
    summary.sample_count = climate.cells.size();
    summary.mean = static_cast<float>(total / static_cast<double>(summary.sample_count));
    return summary;
}

void GreaterRealmClimateGenerationCache::invalidate() noexcept {
    m_pending_temperature = true;
}

void GreaterRealmClimateGenerationCache::reset() noexcept {
    m_settings = GreaterRealmClimateSettings{};
    m_pending_temperature = true;
    m_initialized = false;
}

GreaterRealmClimateRegenerationResult GreaterRealmClimateGenerationCache::regenerate(
    GreaterRealmClimateMap& climate,
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateSettings& settings
) {
    const auto clamped = clamp_greater_realm_climate_settings(settings);
    const bool rebuild = m_pending_temperature
        || !m_initialized
        || m_settings != clamped
        || !climate.source_matches(terrain);
    if (!rebuild) {
        return {};
    }

    climate = generate_greater_realm_climate(terrain, clamped);
    m_settings = clamped;
    m_pending_temperature = false;
    m_initialized = climate.source_matches(terrain);
    return {GreaterRealmClimateDirtyStage::Temperature};
}

} // namespace procgen
