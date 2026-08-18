#include "../../include/world/SeasonalClimate.hpp"
#include "../../include/procgen/detail/GreaterRealmUtility.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace world {
namespace {

constexpr std::uint64_t SEASONAL_TEMPERATURE_REGIONAL_DOMAIN = 0x736561736f6e7465ull;
constexpr std::uint64_t SEASONAL_PRECIPITATION_REGIONAL_DOMAIN = 0x736561736f6e7072ull;

using procgen::detail::clamp01;
using procgen::detail::lerp;
using procgen::detail::mix_hash;
using procgen::detail::mixed_value_noise;
using procgen::detail::smoothstep01;

[[nodiscard]] float blended_hemisphere_wave(
    float normalized_year,
    float northern_peak,
    float southern_peak,
    float latitude,
    float transition_degrees,
    float phase_variation
) noexcept {
    constexpr float PI = 3.14159265358979323846f;
    const float northern_wave = std::cos(
        2.0f * PI * (
            normalized_year - normalize_year_fraction(northern_peak + phase_variation)
        )
    );
    const float southern_wave = std::cos(
        2.0f * PI * (
            normalized_year - normalize_year_fraction(southern_peak + phase_variation)
        )
    );
    const float northern_weight = smoothstep01(
        (latitude + transition_degrees) / (2.0f * transition_degrees)
    );
    return lerp(southern_wave, northern_wave, northern_weight);
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

void initialize_seasonal_precipitation_map(
    SeasonalPrecipitationMap& seasonal_precipitation,
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalPrecipitationSettings& settings,
    float year_fraction
) {
    seasonal_precipitation.version = SEASONAL_PRECIPITATION_VERSION;
    seasonal_precipitation.source_seed = terrain.seed;
    seasonal_precipitation.source_width = terrain.width;
    seasonal_precipitation.source_height = terrain.height;
    seasonal_precipitation.source_cell_size = terrain.cell_size;
    seasonal_precipitation.source_terrain_fingerprint =
        procgen::greater_realm_climate_source_fingerprint(terrain);
    seasonal_precipitation.source_precipitation_fingerprint =
        annual_precipitation_fingerprint(climate);
    seasonal_precipitation.settings_fingerprint =
        seasonal_precipitation_settings_fingerprint(settings);
    seasonal_precipitation.year_fraction = normalize_year_fraction(year_fraction);
    seasonal_precipitation.cells.assign(
        terrain.cells.size(),
        SeasonalPrecipitationCell{}
    );
}

} // namespace

std::size_t SeasonalTemperatureMap::expected_cell_count() const noexcept {
    return static_cast<std::size_t>(source_width) * static_cast<std::size_t>(source_height);
}

bool SeasonalTemperatureMap::has_expected_cell_count() const noexcept {
    return cells.size() == expected_cell_count();
}

bool SeasonalTemperatureMap::source_maps_match(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate
) const noexcept {
    return version == SEASONAL_TEMPERATURE_VERSION
        && source_seed == terrain.seed
        && source_width == terrain.width
        && source_height == terrain.height
        && source_cell_size == terrain.cell_size
        && source_terrain_fingerprint
            == procgen::greater_realm_climate_source_fingerprint(terrain)
        && source_temperature_fingerprint == annual_temperature_fingerprint(climate)
        && terrain.has_expected_cell_count()
        && climate.source_matches(terrain)
        && has_expected_cell_count();
}

bool SeasonalTemperatureMap::source_matches(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureSettings& settings,
    float requested_year_fraction
) const noexcept {
    return source_maps_match(terrain, climate)
        && settings_fingerprint == seasonal_temperature_settings_fingerprint(settings)
        && year_fraction == normalize_year_fraction(requested_year_fraction);
}

std::size_t SeasonalPrecipitationMap::expected_cell_count() const noexcept {
    return static_cast<std::size_t>(source_width) * static_cast<std::size_t>(source_height);
}

bool SeasonalPrecipitationMap::has_expected_cell_count() const noexcept {
    return cells.size() == expected_cell_count();
}

bool SeasonalPrecipitationMap::source_maps_match(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate
) const noexcept {
    return version == SEASONAL_PRECIPITATION_VERSION
        && source_seed == terrain.seed
        && source_width == terrain.width
        && source_height == terrain.height
        && source_cell_size == terrain.cell_size
        && source_terrain_fingerprint
            == procgen::greater_realm_climate_source_fingerprint(terrain)
        && source_precipitation_fingerprint == annual_precipitation_fingerprint(climate)
        && terrain.has_expected_cell_count()
        && climate.source_matches(terrain)
        && has_expected_cell_count();
}

bool SeasonalPrecipitationMap::source_matches(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalPrecipitationSettings& settings,
    float requested_year_fraction
) const noexcept {
    return source_maps_match(terrain, climate)
        && settings_fingerprint == seasonal_precipitation_settings_fingerprint(settings)
        && year_fraction == normalize_year_fraction(requested_year_fraction);
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
    clamped.equatorial_transition_degrees = finite_or(
        clamped.equatorial_transition_degrees,
        defaults.equatorial_transition_degrees
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
    clamped.equatorial_transition_degrees = std::clamp(
        clamped.equatorial_transition_degrees, 1.0f, 90.0f
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
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(
        clamped.equatorial_transition_degrees
    ));
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

SeasonalPrecipitationSettings clamp_seasonal_precipitation_settings(
    const SeasonalPrecipitationSettings& settings
) noexcept {
    SeasonalPrecipitationSettings clamped = settings;
    const SeasonalPrecipitationSettings defaults;
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
    clamped.inland_damping = finite_or(clamped.inland_damping, defaults.inland_damping);
    clamped.northern_wet_peak_year_fraction = finite_or(
        clamped.northern_wet_peak_year_fraction,
        defaults.northern_wet_peak_year_fraction
    );
    clamped.southern_wet_peak_year_fraction = finite_or(
        clamped.southern_wet_peak_year_fraction,
        defaults.southern_wet_peak_year_fraction
    );
    clamped.equatorial_transition_degrees = finite_or(
        clamped.equatorial_transition_degrees,
        defaults.equatorial_transition_degrees
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
    clamped.minimum_multiplier = finite_or(
        clamped.minimum_multiplier, defaults.minimum_multiplier
    );
    clamped.maximum_multiplier = finite_or(
        clamped.maximum_multiplier, defaults.maximum_multiplier
    );

    clamped.north_edge_latitude_degrees = std::clamp(
        clamped.north_edge_latitude_degrees, -90.0f, 90.0f
    );
    clamped.south_edge_latitude_degrees = std::clamp(
        clamped.south_edge_latitude_degrees, -90.0f, 90.0f
    );
    clamped.base_amplitude = clamp01(clamped.base_amplitude);
    clamped.latitude_amplitude = clamp01(clamped.latitude_amplitude);
    clamped.inland_damping = clamp01(clamped.inland_damping);
    clamped.northern_wet_peak_year_fraction = normalize_year_fraction(
        clamped.northern_wet_peak_year_fraction
    );
    clamped.southern_wet_peak_year_fraction = normalize_year_fraction(
        clamped.southern_wet_peak_year_fraction
    );
    clamped.equatorial_transition_degrees = std::clamp(
        clamped.equatorial_transition_degrees, 1.0f, 90.0f
    );
    clamped.regional_phase_variation = std::clamp(
        clamped.regional_phase_variation, 0.0f, 0.5f
    );
    clamped.regional_amplitude_variation = clamp01(clamped.regional_amplitude_variation);
    clamped.regional_variation_frequency = std::clamp(
        clamped.regional_variation_frequency, 0.25f, 8.0f
    );
    clamped.minimum_multiplier = std::clamp(clamped.minimum_multiplier, 0.0f, 1.0f);
    clamped.maximum_multiplier = std::clamp(clamped.maximum_multiplier, 1.0f, 3.0f);
    if (clamped.maximum_multiplier < clamped.minimum_multiplier) {
        clamped.maximum_multiplier = clamped.minimum_multiplier;
    }
    return clamped;
}

float seasonal_precipitation_latitude_for_row(
    const SeasonalPrecipitationSettings& settings,
    std::uint32_t row,
    std::uint32_t height
) noexcept {
    const auto clamped = clamp_seasonal_precipitation_settings(settings);
    const float amount = height > 1
        ? static_cast<float>(std::min(row, height - 1)) / static_cast<float>(height - 1)
        : 0.5f;
    return lerp(
        clamped.north_edge_latitude_degrees,
        clamped.south_edge_latitude_degrees,
        amount
    );
}

std::uint64_t seasonal_precipitation_settings_fingerprint(
    const SeasonalPrecipitationSettings& settings
) noexcept {
    const auto clamped = clamp_seasonal_precipitation_settings(settings);
    std::uint64_t hash = mix_hash(clamped.profile_seed, clamped.profile_identity);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.north_edge_latitude_degrees));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.south_edge_latitude_degrees));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.base_amplitude));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.latitude_amplitude));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.inland_damping));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.northern_wet_peak_year_fraction));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.southern_wet_peak_year_fraction));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(
        clamped.equatorial_transition_degrees
    ));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.regional_phase_variation));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.regional_amplitude_variation));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.regional_variation_frequency));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.minimum_multiplier));
    return mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.maximum_multiplier));
}

std::uint64_t annual_precipitation_fingerprint(
    const procgen::GreaterRealmClimateMap& climate
) noexcept {
    std::uint64_t hash = mix_hash(climate.version, climate.source_seed);
    hash = mix_hash(hash, climate.source_width);
    hash = mix_hash(hash, climate.source_height);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(climate.source_cell_size));
    hash = mix_hash(hash, climate.source_terrain_fingerprint);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(
        climate.precipitation_character.wetness_scale
    ));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(
        climate.precipitation_character.retention_scale
    ));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(
        climate.wind_character.global_rotation_degrees
    ));
    for (const auto& cell : climate.cells) {
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(cell.precipitation_normal));
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

    const float normalized_year = normalize_year_fraction(year_fraction);
    const auto water_distances = procgen::detail::distance_to_water(terrain);
    for (std::uint32_t y = 0; y < terrain.height; ++y) {
        const float latitude = seasonal_temperature_latitude_for_row(clamped, y, terrain.height);
        const float latitude_strength = std::abs(latitude) / 90.0f;
        for (std::uint32_t x = 0; x < terrain.width; ++x) {
            const std::size_t index = terrain.index(x, y);
            const float normalized_x = terrain.width > 1
                ? static_cast<float>(x) / static_cast<float>(terrain.width - 1)
                : 0.5f;
            const float normalized_y = terrain.height > 1
                ? static_cast<float>(y) / static_cast<float>(terrain.height - 1)
                : 0.5f;
            float phase_variation = 0.0f;
            if (clamped.regional_phase_variation > 0.0f) {
                phase_variation = (
                    mixed_value_noise(
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
                + procgen::detail::normalized_land_height(terrain.cells[index])
                    * clamped.elevation_amplitude;
            if (clamped.maritime_influence_distance > 0.0f
                && std::isfinite(water_distances[index])) {
                const float maritime_influence = 1.0f - smoothstep01(
                    water_distances[index] / clamped.maritime_influence_distance
                );
                amplitude *= 1.0f - maritime_influence * clamped.maritime_damping;
            }
            if (clamped.regional_amplitude_variation > 0.0f) {
                const float noise = mixed_value_noise(
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

            const float seasonal_wave = blended_hemisphere_wave(
                normalized_year,
                clamped.northern_peak_year_fraction,
                clamped.southern_peak_year_fraction,
                latitude,
                clamped.equatorial_transition_degrees,
                phase_variation
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

SeasonalPrecipitationMap evaluate_seasonal_precipitation(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalPrecipitationSettings& settings,
    float year_fraction
) {
    const auto clamped = clamp_seasonal_precipitation_settings(settings);
    SeasonalPrecipitationMap seasonal_precipitation;
    initialize_seasonal_precipitation_map(
        seasonal_precipitation,
        terrain,
        climate,
        clamped,
        year_fraction
    );
    if (!terrain.has_expected_cell_count() || !climate.source_matches(terrain)) {
        seasonal_precipitation.cells.clear();
        return seasonal_precipitation;
    }

    const float normalized_year = normalize_year_fraction(year_fraction);
    for (std::uint32_t y = 0; y < terrain.height; ++y) {
        const float latitude = seasonal_precipitation_latitude_for_row(
            clamped,
            y,
            terrain.height
        );
        const float latitude_strength = std::abs(latitude) / 90.0f;
        for (std::uint32_t x = 0; x < terrain.width; ++x) {
            const std::size_t index = terrain.index(x, y);
            const float normalized_x = terrain.width > 1
                ? static_cast<float>(x) / static_cast<float>(terrain.width - 1)
                : 0.5f;
            const float normalized_y = terrain.height > 1
                ? static_cast<float>(y) / static_cast<float>(terrain.height - 1)
                : 0.5f;
            float phase_variation = 0.0f;
            if (clamped.regional_phase_variation > 0.0f) {
                phase_variation = (
                    mixed_value_noise(
                        clamped.profile_seed,
                        normalized_x,
                        normalized_y,
                        clamped.regional_variation_frequency,
                        SEASONAL_PRECIPITATION_REGIONAL_DOMAIN
                    ) * 2.0f - 1.0f
                ) * clamped.regional_phase_variation;
            }

            float amplitude = clamped.base_amplitude
                + latitude_strength * clamped.latitude_amplitude;
            amplitude *= 1.0f
                - procgen::detail::normalized_land_height(terrain.cells[index])
                    * clamped.inland_damping;
            if (clamped.regional_amplitude_variation > 0.0f) {
                const float noise = mixed_value_noise(
                    clamped.profile_seed,
                    normalized_x,
                    normalized_y,
                    clamped.regional_variation_frequency,
                    SEASONAL_PRECIPITATION_REGIONAL_DOMAIN + 1013ull
                ) * 2.0f - 1.0f;
                amplitude *= std::max(
                    0.0f,
                    1.0f + noise * clamped.regional_amplitude_variation
                );
            }

            const float wetness_wave = blended_hemisphere_wave(
                normalized_year,
                clamped.northern_wet_peak_year_fraction,
                clamped.southern_wet_peak_year_fraction,
                latitude,
                clamped.equatorial_transition_degrees,
                phase_variation
            );
            auto& cell = seasonal_precipitation.cells[index];
            cell.annual_precipitation_normal = climate.cells[index].precipitation_normal;
            cell.seasonal_multiplier = std::clamp(
                1.0f + amplitude * wetness_wave,
                clamped.minimum_multiplier,
                clamped.maximum_multiplier
            );
            cell.seasonal_precipitation_normal = clamp01(
                cell.annual_precipitation_normal * cell.seasonal_multiplier
            );
        }
    }
    return seasonal_precipitation;
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

void SeasonalPrecipitationEvaluationCache::invalidate() noexcept {
    m_pending = true;
}

void SeasonalPrecipitationEvaluationCache::reset() noexcept {
    m_settings = SeasonalPrecipitationSettings{};
    m_year_fraction = 0.0f;
    m_pending = true;
    m_initialized = false;
}

SeasonalPrecipitationEvaluationResult SeasonalPrecipitationEvaluationCache::regenerate(
    SeasonalPrecipitationMap& seasonal_precipitation,
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalPrecipitationSettings& settings,
    float year_fraction
) {
    const auto clamped = clamp_seasonal_precipitation_settings(settings);
    const float normalized_year = normalize_year_fraction(year_fraction);
    if (!m_pending
        && m_initialized
        && m_settings == clamped
        && m_year_fraction == normalized_year
        && seasonal_precipitation.source_matches(terrain, climate, clamped, normalized_year)) {
        return {};
    }

    seasonal_precipitation = evaluate_seasonal_precipitation(
        terrain,
        climate,
        clamped,
        normalized_year
    );
    m_settings = clamped;
    m_year_fraction = normalized_year;
    m_pending = false;
    m_initialized = seasonal_precipitation.source_matches(
        terrain,
        climate,
        clamped,
        normalized_year
    );
    return {true};
}

} // namespace world
