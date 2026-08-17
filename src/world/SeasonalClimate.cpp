#include "../../include/world/SeasonalClimate.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <utility>

namespace world {
namespace {

constexpr std::uint64_t SEASONAL_TEMPERATURE_REGIONAL_DOMAIN = 0x736561736f6e7465ull;

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
    procgen::Seed seed,
    std::int32_t x,
    std::int32_t y,
    std::uint64_t salt
) noexcept {
    std::uint64_t hash = mix_hash(seed, salt);
    hash = mix_hash(hash, static_cast<std::uint32_t>(x));
    return mix_hash(hash, static_cast<std::uint32_t>(y));
}

[[nodiscard]] float random01(
    procgen::Seed seed,
    std::int32_t x,
    std::int32_t y,
    std::uint64_t salt
) noexcept {
    return static_cast<float>((hash_coords(seed, x, y, salt) >> 40) & 0xffffffu)
        / static_cast<float>(0xffffffu);
}

[[nodiscard]] float value_noise(
    procgen::Seed seed,
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

[[nodiscard]] float normalized_land_height(const procgen::GreaterRealmCell& cell) noexcept {
    if (cell.is_water) {
        return 0.0f;
    }
    return clamp01(
        (cell.elevation - procgen::NORMALIZED_WATERLINE)
            / (1.0f - procgen::NORMALIZED_WATERLINE)
    );
}

[[nodiscard]] std::vector<float> distance_to_water(const procgen::GreaterRealmMap& terrain) {
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

void initialize_seasonal_temperature_map(
    SeasonalTemperatureMap& seasonal_temperature,
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureSettings& settings,
    float year_fraction
) {
    seasonal_temperature.version = SEASONAL_TEMPERATURE_VERSION;
    seasonal_temperature.source_seed = terrain.seed;
    seasonal_temperature.source_width = terrain.width;
    seasonal_temperature.source_height = terrain.height;
    seasonal_temperature.source_cell_size = terrain.cell_size;
    seasonal_temperature.source_terrain_fingerprint =
        procgen::greater_realm_climate_source_fingerprint(terrain);
    seasonal_temperature.source_temperature_fingerprint =
        annual_temperature_fingerprint(climate);
    seasonal_temperature.settings_fingerprint =
        seasonal_temperature_settings_fingerprint(settings);
    seasonal_temperature.year_fraction = normalize_year_fraction(year_fraction);
    seasonal_temperature.cells.assign(
        terrain.cells.size(),
        SeasonalTemperatureCell{}
    );
}

} // namespace

std::size_t SeasonalTemperatureMap::expected_cell_count() const noexcept {
    return static_cast<std::size_t>(source_width) * static_cast<std::size_t>(source_height);
}

bool SeasonalTemperatureMap::has_expected_cell_count() const noexcept {
    return cells.size() == expected_cell_count();
}

bool SeasonalTemperatureMap::source_matches(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureSettings& settings,
    float requested_year_fraction
) const noexcept {
    return version == SEASONAL_TEMPERATURE_VERSION
        && source_seed == terrain.seed
        && source_width == terrain.width
        && source_height == terrain.height
        && source_cell_size == terrain.cell_size
        && source_terrain_fingerprint
            == procgen::greater_realm_climate_source_fingerprint(terrain)
        && source_temperature_fingerprint == annual_temperature_fingerprint(climate)
        && settings_fingerprint == seasonal_temperature_settings_fingerprint(settings)
        && year_fraction == normalize_year_fraction(requested_year_fraction)
        && terrain.has_expected_cell_count()
        && climate.source_matches(terrain)
        && has_expected_cell_count();
}

SeasonalTemperatureSettings clamp_seasonal_temperature_settings(
    const SeasonalTemperatureSettings& settings
) noexcept {
    SeasonalTemperatureSettings clamped = settings;
    const SeasonalTemperatureSettings defaults;
    const auto finite_or = [](float value, float fallback) {
        return std::isfinite(value) ? value : fallback;
    };
    clamped.north_edge_latitude_degrees = finite_or(
        clamped.north_edge_latitude_degrees, defaults.north_edge_latitude_degrees
    );
    clamped.south_edge_latitude_degrees = finite_or(
        clamped.south_edge_latitude_degrees, defaults.south_edge_latitude_degrees
    );
    clamped.base_amplitude = finite_or(clamped.base_amplitude, defaults.base_amplitude);
    clamped.latitude_amplitude = finite_or(
        clamped.latitude_amplitude, defaults.latitude_amplitude
    );
    clamped.elevation_amplitude = finite_or(
        clamped.elevation_amplitude, defaults.elevation_amplitude
    );
    clamped.maritime_damping = finite_or(
        clamped.maritime_damping, defaults.maritime_damping
    );
    clamped.maritime_influence_distance = finite_or(
        clamped.maritime_influence_distance,
        defaults.maritime_influence_distance
    );
    clamped.northern_peak_year_fraction = finite_or(
        clamped.northern_peak_year_fraction,
        defaults.northern_peak_year_fraction
    );
    clamped.southern_peak_year_fraction = finite_or(
        clamped.southern_peak_year_fraction,
        defaults.southern_peak_year_fraction
    );
    clamped.regional_phase_variation = finite_or(
        clamped.regional_phase_variation,
        defaults.regional_phase_variation
    );
    clamped.regional_amplitude_variation = finite_or(
        clamped.regional_amplitude_variation,
        defaults.regional_amplitude_variation
    );
    clamped.regional_variation_frequency = finite_or(
        clamped.regional_variation_frequency,
        defaults.regional_variation_frequency
    );

    clamped.north_edge_latitude_degrees = std::clamp(
        clamped.north_edge_latitude_degrees, -90.0f, 90.0f
    );
    clamped.south_edge_latitude_degrees = std::clamp(
        clamped.south_edge_latitude_degrees, -90.0f, 90.0f
    );
    clamped.base_amplitude = std::clamp(clamped.base_amplitude, 0.0f, 0.5f);
    clamped.latitude_amplitude = std::clamp(clamped.latitude_amplitude, 0.0f, 0.5f);
    clamped.elevation_amplitude = std::clamp(clamped.elevation_amplitude, 0.0f, 0.5f);
    clamped.maritime_damping = clamp01(clamped.maritime_damping);
    clamped.maritime_influence_distance = std::max(
        clamped.maritime_influence_distance, 0.0f
    );
    clamped.northern_peak_year_fraction = normalize_year_fraction(
        clamped.northern_peak_year_fraction
    );
    clamped.southern_peak_year_fraction = normalize_year_fraction(
        clamped.southern_peak_year_fraction
    );
    clamped.regional_phase_variation = std::clamp(
        clamped.regional_phase_variation, 0.0f, 0.5f
    );
    clamped.regional_amplitude_variation = clamp01(clamped.regional_amplitude_variation);
    clamped.regional_variation_frequency = std::clamp(
        clamped.regional_variation_frequency, 0.25f, 8.0f
    );
    return clamped;
}

float normalize_year_fraction(float year_fraction) noexcept {
    if (!std::isfinite(year_fraction)) {
        return 0.0f;
    }
    float normalized = std::fmod(year_fraction, 1.0f);
    if (normalized < 0.0f) {
        normalized += 1.0f;
    }
    constexpr float YEAR_FRACTION_QUANTA = 1048576.0f;
    normalized = std::round(normalized * YEAR_FRACTION_QUANTA) / YEAR_FRACTION_QUANTA;
    return normalized >= 1.0f ? 0.0f : normalized;
}

float seasonal_temperature_latitude_for_row(
    const SeasonalTemperatureSettings& settings,
    std::uint32_t row,
    std::uint32_t height
) noexcept {
    const auto clamped = clamp_seasonal_temperature_settings(settings);
    const float amount = height > 1
        ? static_cast<float>(std::min(row, height - 1)) / static_cast<float>(height - 1)
        : 0.5f;
    return lerp(
        clamped.north_edge_latitude_degrees,
        clamped.south_edge_latitude_degrees,
        amount
    );
}

std::uint64_t seasonal_temperature_settings_fingerprint(
    const SeasonalTemperatureSettings& settings
) noexcept {
    const auto clamped = clamp_seasonal_temperature_settings(settings);
    std::uint64_t hash = mix_hash(clamped.profile_seed, clamped.profile_identity);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.north_edge_latitude_degrees));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.south_edge_latitude_degrees));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.base_amplitude));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.latitude_amplitude));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.elevation_amplitude));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.maritime_damping));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.maritime_influence_distance));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.northern_peak_year_fraction));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.southern_peak_year_fraction));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.regional_phase_variation));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.regional_amplitude_variation));
    return mix_hash(
        hash,
        std::bit_cast<std::uint32_t>(clamped.regional_variation_frequency)
    );
}

std::uint64_t annual_temperature_fingerprint(
    const procgen::GreaterRealmClimateMap& climate
) noexcept {
    std::uint64_t hash = mix_hash(climate.version, climate.source_seed);
    hash = mix_hash(hash, climate.source_width);
    hash = mix_hash(hash, climate.source_height);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(climate.source_cell_size));
    hash = mix_hash(hash, climate.source_terrain_fingerprint);
    for (const auto& cell : climate.cells) {
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(cell.temperature_normal));
    }
    return hash;
}

SeasonalTemperatureMap evaluate_seasonal_temperature(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureSettings& settings,
    float year_fraction
) {
    const auto clamped = clamp_seasonal_temperature_settings(settings);
    SeasonalTemperatureMap seasonal_temperature;
    initialize_seasonal_temperature_map(
        seasonal_temperature,
        terrain,
        climate,
        clamped,
        year_fraction
    );
    if (!terrain.has_expected_cell_count() || !climate.source_matches(terrain)) {
        seasonal_temperature.cells.clear();
        return seasonal_temperature;
    }

    constexpr float PI = 3.14159265358979323846f;
    const float normalized_year = normalize_year_fraction(year_fraction);
    const auto water_distances = distance_to_water(terrain);
    for (std::uint32_t y = 0; y < terrain.height; ++y) {
        const float latitude = seasonal_temperature_latitude_for_row(clamped, y, terrain.height);
        const float latitude_strength = std::abs(latitude) / 90.0f;
        const float hemisphere_peak = latitude >= 0.0f
            ? clamped.northern_peak_year_fraction
            : clamped.southern_peak_year_fraction;
        for (std::uint32_t x = 0; x < terrain.width; ++x) {
            const std::size_t index = terrain.index(x, y);
            const float normalized_x = terrain.width > 1
                ? static_cast<float>(x) / static_cast<float>(terrain.width - 1)
                : 0.5f;
            const float normalized_y = terrain.height > 1
                ? static_cast<float>(y) / static_cast<float>(terrain.height - 1)
                : 0.5f;
            float phase = hemisphere_peak;
            if (clamped.regional_phase_variation > 0.0f) {
                phase += (
                    value_noise(
                        clamped.profile_seed,
                        normalized_x,
                        normalized_y,
                        clamped.regional_variation_frequency,
                        SEASONAL_TEMPERATURE_REGIONAL_DOMAIN
                    ) * 2.0f - 1.0f
                ) * clamped.regional_phase_variation;
            }

            float amplitude = clamped.base_amplitude
                + latitude_strength * clamped.latitude_amplitude
                + normalized_land_height(terrain.cells[index]) * clamped.elevation_amplitude;
            if (clamped.maritime_influence_distance > 0.0f
                && std::isfinite(water_distances[index])) {
                const float maritime_influence = 1.0f - smoothstep01(
                    water_distances[index] / clamped.maritime_influence_distance
                );
                amplitude *= 1.0f - maritime_influence * clamped.maritime_damping;
            }
            if (clamped.regional_amplitude_variation > 0.0f) {
                const float noise = value_noise(
                    clamped.profile_seed,
                    normalized_x,
                    normalized_y,
                    clamped.regional_variation_frequency,
                    SEASONAL_TEMPERATURE_REGIONAL_DOMAIN + 1013ull
                ) * 2.0f - 1.0f;
                amplitude *= std::max(
                    0.0f,
                    1.0f + noise * clamped.regional_amplitude_variation
                );
            }

            const float seasonal_wave = std::cos(
                2.0f * PI * (normalized_year - normalize_year_fraction(phase))
            );
            auto& cell = seasonal_temperature.cells[index];
            cell.annual_temperature_normal = climate.cells[index].temperature_normal;
            cell.seasonal_offset = amplitude * seasonal_wave;
            cell.seasonal_temperature_normal = clamp01(
                cell.annual_temperature_normal + cell.seasonal_offset
            );
        }
    }
    return seasonal_temperature;
}

void SeasonalTemperatureEvaluationCache::invalidate() noexcept {
    m_pending = true;
}

void SeasonalTemperatureEvaluationCache::reset() noexcept {
    m_settings = SeasonalTemperatureSettings{};
    m_year_fraction = 0.0f;
    m_pending = true;
    m_initialized = false;
}

SeasonalTemperatureEvaluationResult SeasonalTemperatureEvaluationCache::regenerate(
    SeasonalTemperatureMap& seasonal_temperature,
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureSettings& settings,
    float year_fraction
) {
    const auto clamped = clamp_seasonal_temperature_settings(settings);
    const float normalized_year = normalize_year_fraction(year_fraction);
    if (!m_pending
        && m_initialized
        && m_settings == clamped
        && m_year_fraction == normalized_year
        && seasonal_temperature.source_matches(terrain, climate, clamped, normalized_year)) {
        return {};
    }

    seasonal_temperature = evaluate_seasonal_temperature(
        terrain,
        climate,
        clamped,
        normalized_year
    );
    m_settings = clamped;
    m_year_fraction = normalized_year;
    m_pending = false;
    m_initialized = seasonal_temperature.source_matches(
        terrain,
        climate,
        clamped,
        normalized_year
    );
    return {true};
}

} // namespace world
