#include "../../include/procgen/Climate.hpp"
#include "../../include/procgen/detail/GreaterRealmUtility.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <numeric>

namespace procgen {
namespace {

constexpr std::uint64_t TEMPERATURE_NOISE_DOMAIN = 0x74656d7065726174ull;
constexpr std::uint64_t PRECIPITATION_CHARACTER_DOMAIN = 0x7072656369706368ull;
constexpr std::uint64_t WIND_CHARACTER_DOMAIN = 0x77696e6463686172ull;
constexpr std::uint64_t REGIONAL_WIND_DOMAIN = 0x726567696f6e7764ull;

using detail::clamp01;
using detail::lerp;
using detail::mix_hash;
using detail::mixed_grid_random01;
using detail::mixed_value_noise;
using detail::smoothstep01;

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
        total += mixed_value_noise(
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

void initialize_climate_map(
    GreaterRealmClimateMap& climate,
    const GreaterRealmMap& terrain
) {
    climate.version = GREATER_REALM_CLIMATE_VERSION;
    climate.source_seed = terrain.seed;
    climate.source_width = terrain.width;
    climate.source_height = terrain.height;
    climate.source_cell_size = terrain.cell_size;
    climate.source_terrain_fingerprint = greater_realm_climate_source_fingerprint(terrain);
    climate.precipitation_character = {};
    climate.wind_character = {};
    climate.cells.assign(terrain.cells.size(), GreaterRealmClimateCell{});
}

void generate_temperature_normals(
    GreaterRealmClimateMap& climate,
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateSettings& settings
) {
    const auto water_distances = detail::distance_to_water(terrain);
    for (std::uint32_t y = 0; y < terrain.height; ++y) {
        const float latitude = greater_realm_latitude_for_row(settings, y, terrain.height);
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
                settings.temperature_variation_frequency
            ) * 2.0f - 1.0f;
            float temperature = latitude_temperature
                + noise * settings.temperature_variation;
            temperature -= detail::normalized_land_height(terrain.cells[index])
                * settings.elevation_cooling;

            float maritime_influence = 0.0f;
            if (settings.maritime_influence_distance > 0.0f
                && std::isfinite(water_distances[index])) {
                maritime_influence = 1.0f - smoothstep01(
                    water_distances[index] / settings.maritime_influence_distance
                );
            }
            temperature = lerp(
                temperature,
                0.5f,
                maritime_influence * settings.maritime_moderation
            );
            climate.cells[index].temperature_normal = clamp01(temperature);
        }
    }
}

[[nodiscard]] std::vector<float> generate_precipitation_pass(
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateSettings& settings,
    float wind_degrees
) {
    constexpr float PI = 3.14159265358979323846f;
    const float radians = wind_degrees * PI / 180.0f;
    const float wind_x = std::cos(radians);
    const float wind_y = std::sin(radians);

    std::vector<std::uint32_t> order(terrain.cells.size());
    std::iota(order.begin(), order.end(), 0u);
    std::stable_sort(order.begin(), order.end(), [&](std::uint32_t left, std::uint32_t right) {
        const float left_projection = static_cast<float>(left % terrain.width) * wind_x
            + static_cast<float>(left / terrain.width) * wind_y;
        const float right_projection = static_cast<float>(right % terrain.width) * wind_x
            + static_cast<float>(right / terrain.width) * wind_y;
        if (left_projection != right_projection) {
            return left_projection < right_projection;
        }
        return left < right;
    });

    std::vector<float> outgoing_moisture(terrain.cells.size(), settings.ambient_moisture);
    std::vector<float> outgoing_shadow(terrain.cells.size(), 0.0f);
    std::vector<float> precipitation(terrain.cells.size(), 0.0f);
    const float map_step = terrain.cell_size > 0.0f ? terrain.cell_size : 1.0f;
    const float retention = std::pow(settings.moisture_retention, map_step);
    const float shadow_retention = std::pow(settings.rain_shadow_decay, map_step);

    for (const std::uint32_t index : order) {
        const std::int32_t x = static_cast<std::int32_t>(index % terrain.width);
        const std::int32_t y = static_cast<std::int32_t>(index / terrain.width);
        float moisture_sum = 0.0f;
        float shadow_sum = 0.0f;
        float elevation_sum = 0.0f;
        float weight_sum = 0.0f;

        for (const auto& offset : detail::EIGHT_WAY_NEIGHBORS) {
            const float upwind_alignment = -(
                static_cast<float>(offset[0]) * wind_x
                + static_cast<float>(offset[1]) * wind_y
            );
            if (upwind_alignment <= 0.0001f) {
                continue;
            }
            const std::int32_t neighbor_x = x + offset[0];
            const std::int32_t neighbor_y = y + offset[1];
            if (!terrain.contains(neighbor_x, neighbor_y)) {
                continue;
            }
            const std::size_t neighbor_index = terrain.index(
                static_cast<std::uint32_t>(neighbor_x),
                static_cast<std::uint32_t>(neighbor_y)
            );
            const bool diagonal = offset[0] != 0 && offset[1] != 0;
            const float weight = upwind_alignment
                / (diagonal ? detail::SQRT_TWO : 1.0f);
            moisture_sum += outgoing_moisture[neighbor_index] * weight;
            shadow_sum += outgoing_shadow[neighbor_index] * weight;
            elevation_sum += detail::normalized_land_height(terrain.cells[neighbor_index])
                * weight;
            weight_sum += weight;
        }

        float moisture = weight_sum > 0.0f
            ? moisture_sum / weight_sum * retention
            : settings.ambient_moisture;
        float shadow = weight_sum > 0.0f
            ? shadow_sum / weight_sum * shadow_retention
            : 0.0f;
        const float upwind_elevation = weight_sum > 0.0f
            ? elevation_sum / weight_sum
            : 0.0f;

        const auto& terrain_cell = terrain.cells[index];
        if (terrain_cell.is_water) {
            const float source = terrain_cell.is_ocean
                ? settings.ocean_moisture_source
                : settings.inland_water_moisture_source;
            moisture = std::max(moisture, source);
            shadow = 0.0f;
        }

        const float elevation = detail::normalized_land_height(terrain_cell);
        const float rise = std::max(elevation - upwind_elevation, 0.0f);
        const float lift_rain = std::min(
            moisture,
            moisture * rise * settings.orographic_lift
        );
        const float background_rain = moisture * settings.precipitation_efficiency;
        precipitation[index] = clamp01(
            (background_rain + lift_rain - shadow) * settings.precipitation_scale
        );

        outgoing_moisture[index] = clamp01(
            moisture - lift_rain * 0.70f - background_rain * 0.08f
        );
        outgoing_shadow[index] = std::max(
            shadow,
            lift_rain * settings.rain_shadow_strength
        );
    }
    return precipitation;
}

[[nodiscard]] std::vector<float> generate_latitude_band_precipitation(
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateSettings& settings
) {
    const auto rotated = [&](float degrees) {
        float result = std::fmod(degrees + settings.prevailing_wind_degrees, 360.0f);
        if (result < 0.0f) {
            result += 360.0f;
        }
        return result;
    };
    if (settings.latitude_wind_band_strength <= 0.0f) {
        return generate_precipitation_pass(terrain, settings, rotated(0.0f));
    }

    const auto north_tropical = generate_precipitation_pass(
        terrain, settings, rotated(135.0f)
    );
    const auto north_midlatitude = generate_precipitation_pass(
        terrain, settings, rotated(315.0f)
    );
    const auto south_tropical = generate_precipitation_pass(
        terrain, settings, rotated(225.0f)
    );
    const auto south_midlatitude = generate_precipitation_pass(
        terrain, settings, rotated(45.0f)
    );
    const auto global = settings.latitude_wind_band_strength < 1.0f
        ? generate_precipitation_pass(terrain, settings, rotated(0.0f))
        : std::vector<float>{};
    std::vector<float> precipitation(terrain.cells.size(), 0.0f);

    for (std::uint32_t y = 0; y < terrain.height; ++y) {
        const float latitude = greater_realm_latitude_for_row(settings, y, terrain.height);
        const float north_weight = smoothstep01((latitude + 8.0f) / 16.0f);
        const float midlatitude_weight = smoothstep01((std::abs(latitude) - 25.0f) / 15.0f);
        for (std::uint32_t x = 0; x < terrain.width; ++x) {
            const std::size_t index = terrain.index(x, y);
            const float tropical_primary = lerp(
                south_tropical[index], north_tropical[index], north_weight
            );
            const float tropical_secondary = lerp(
                south_midlatitude[index], north_midlatitude[index], north_weight
            );
            const float midlatitude_primary = tropical_secondary;
            const float midlatitude_secondary = tropical_primary;
            const float tropical = lerp(
                tropical_primary,
                tropical_secondary,
                settings.secondary_wind_strength
            );
            const float midlatitude = lerp(
                midlatitude_primary,
                midlatitude_secondary,
                settings.secondary_wind_strength
            );
            const float circulation = lerp(
                tropical,
                midlatitude,
                midlatitude_weight
            );
            precipitation[index] = global.empty()
                ? circulation
                : lerp(
                    global[index],
                    circulation,
                    settings.latitude_wind_band_strength
                );
        }
    }
    return precipitation;
}

[[nodiscard]] std::vector<float> generate_regionally_varied_precipitation(
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateSettings& settings,
    const GreaterRealmWindCharacter& character
) {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float DIRECTION_STEP = 45.0f;
    constexpr std::size_t DIRECTION_COUNT = 8;

    const auto normalize_degrees = [](float degrees) {
        float result = std::fmod(degrees, 360.0f);
        return result < 0.0f ? result + 360.0f : result;
    };
    const float rotation = normalize_degrees(
        settings.prevailing_wind_degrees + character.global_rotation_degrees
    );
    std::array<std::vector<float>, DIRECTION_COUNT> passes;
    for (std::size_t direction = 0; direction < passes.size(); ++direction) {
        passes[direction] = generate_precipitation_pass(
            terrain,
            settings,
            normalize_degrees(rotation + static_cast<float>(direction) * DIRECTION_STEP)
        );
    }

    const auto direction_vector = [PI](float degrees) {
        const float radians = degrees * PI / 180.0f;
        return std::array<float, 2>{std::cos(radians), std::sin(radians)};
    };
    const auto blend_vector = [](const auto& from, const auto& to, float amount) {
        return std::array<float, 2>{
            lerp(from[0], to[0], amount),
            lerp(from[1], to[1], amount)
        };
    };
    const auto sample_passes = [&](std::size_t index, float degrees) {
        const float slot = normalize_degrees(degrees) / DIRECTION_STEP;
        const auto lower = static_cast<std::size_t>(std::floor(slot)) % DIRECTION_COUNT;
        const std::size_t upper = (lower + 1) % DIRECTION_COUNT;
        return lerp(passes[lower][index], passes[upper][index], slot - std::floor(slot));
    };

    const auto north_tropical = direction_vector(135.0f);
    const auto north_midlatitude = direction_vector(315.0f);
    const auto south_tropical = direction_vector(225.0f);
    const auto south_midlatitude = direction_vector(45.0f);
    const auto global = direction_vector(0.0f);
    std::vector<float> precipitation(terrain.cells.size(), 0.0f);

    for (std::uint32_t y = 0; y < terrain.height; ++y) {
        const float latitude = greater_realm_latitude_for_row(settings, y, terrain.height)
            + character.latitude_shift_degrees;
        const float north_weight = smoothstep01((latitude + 8.0f) / 16.0f);
        const float midlatitude_weight = smoothstep01((std::abs(latitude) - 25.0f) / 15.0f);
        const auto tropical = blend_vector(
            south_tropical, north_tropical, north_weight
        );
        const auto midlatitude = blend_vector(
            south_midlatitude, north_midlatitude, north_weight
        );
        const auto circulation = blend_vector(
            tropical, midlatitude, midlatitude_weight
        );
        const auto base = blend_vector(
            global, circulation, settings.latitude_wind_band_strength
        );
        const float hemisphere_offset = lerp(
            character.south_angle_offset_degrees,
            character.north_angle_offset_degrees,
            north_weight
        );

        for (std::uint32_t x = 0; x < terrain.width; ++x) {
            const std::size_t index = terrain.index(x, y);
            const float normalized_x = terrain.width > 1
                ? static_cast<float>(x) / static_cast<float>(terrain.width - 1)
                : 0.5f;
            const float normalized_y = terrain.height > 1
                ? static_cast<float>(y) / static_cast<float>(terrain.height - 1)
                : 0.5f;
            const float regional_noise = mixed_value_noise(
                terrain.seed,
                normalized_x,
                normalized_y,
                settings.regional_wind_frequency,
                REGIONAL_WIND_DOMAIN
            ) * 2.0f - 1.0f;
            const float regional_offset = regional_noise
                * 180.0f
                * settings.regional_wind_strength
                * character.regional_strength_scale;
            const float base_degrees = std::atan2(base[1], base[0]) * 180.0f / PI;
            const float direction = base_degrees + hemisphere_offset + regional_offset;
            const float primary = sample_passes(index, direction);
            const float opposing = sample_passes(index, direction + 180.0f);
            precipitation[index] = lerp(
                primary, opposing, settings.secondary_wind_strength
            );
        }
    }
    return precipitation;
}

void generate_precipitation_normals(
    GreaterRealmClimateMap& climate,
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateSettings& settings
) {
    const auto precipitation_character = derive_greater_realm_precipitation_character(
        terrain.seed,
        settings.precipitation_seed_variation
    );
    const auto wind_character = derive_greater_realm_wind_character(
        terrain.seed,
        settings.wind_seed_variation
    );
    climate.precipitation_character = precipitation_character;
    climate.wind_character = wind_character;

    GreaterRealmClimateSettings effective = settings;
    effective.precipitation_scale = std::clamp(
        settings.precipitation_scale * precipitation_character.wetness_scale,
        0.0f,
        2.0f
    );
    effective.moisture_retention = clamp01(
        1.0f
            - (1.0f - settings.moisture_retention)
                * precipitation_character.retention_scale
    );

    const auto baseline = settings.wind_seed_variation < 1.0f
        ? generate_latitude_band_precipitation(terrain, effective)
        : std::vector<float>{};
    const auto regional = settings.wind_seed_variation > 0.0f
        ? generate_regionally_varied_precipitation(
            terrain, effective, wind_character
        )
        : std::vector<float>{};
    for (std::size_t index = 0; index < climate.cells.size(); ++index) {
        climate.cells[index].precipitation_normal = baseline.empty()
            ? regional[index]
            : regional.empty()
                ? baseline[index]
                : lerp(baseline[index], regional[index], settings.wind_seed_variation);
    }
}

[[nodiscard]] GreaterRealmClimateDirtyStage climate_settings_dirty_stages(
    const GreaterRealmClimateSettings& previous,
    const GreaterRealmClimateSettings& current
) noexcept {
    GreaterRealmClimateDirtyStage stages = GreaterRealmClimateDirtyStage::None;
    if (previous.north_edge_latitude_degrees != current.north_edge_latitude_degrees
        || previous.south_edge_latitude_degrees != current.south_edge_latitude_degrees
        || previous.elevation_cooling != current.elevation_cooling
        || previous.maritime_moderation != current.maritime_moderation
        || previous.maritime_influence_distance != current.maritime_influence_distance
        || previous.temperature_variation != current.temperature_variation
        || previous.temperature_variation_frequency != current.temperature_variation_frequency) {
        stages |= GreaterRealmClimateDirtyStage::Temperature;
    }
    if (previous.prevailing_wind_degrees != current.prevailing_wind_degrees
        || previous.ambient_moisture != current.ambient_moisture
        || previous.ocean_moisture_source != current.ocean_moisture_source
        || previous.inland_water_moisture_source != current.inland_water_moisture_source
        || previous.moisture_retention != current.moisture_retention
        || previous.precipitation_efficiency != current.precipitation_efficiency
        || previous.orographic_lift != current.orographic_lift
        || previous.rain_shadow_strength != current.rain_shadow_strength
        || previous.rain_shadow_decay != current.rain_shadow_decay
        || previous.precipitation_scale != current.precipitation_scale
        || previous.latitude_wind_band_strength != current.latitude_wind_band_strength
        || previous.secondary_wind_strength != current.secondary_wind_strength
        || previous.wind_seed_variation != current.wind_seed_variation
        || previous.regional_wind_strength != current.regional_wind_strength
        || previous.regional_wind_frequency != current.regional_wind_frequency
        || previous.precipitation_seed_variation != current.precipitation_seed_variation) {
        stages |= GreaterRealmClimateDirtyStage::Precipitation;
    }
    return stages;
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
    const GreaterRealmClimateSettings defaults;
    const auto finite_or = [](float value, float fallback) {
        return std::isfinite(value) ? value : fallback;
    };
    clamped.north_edge_latitude_degrees = finite_or(
        clamped.north_edge_latitude_degrees, defaults.north_edge_latitude_degrees
    );
    clamped.south_edge_latitude_degrees = finite_or(
        clamped.south_edge_latitude_degrees, defaults.south_edge_latitude_degrees
    );
    clamped.elevation_cooling = finite_or(
        clamped.elevation_cooling, defaults.elevation_cooling
    );
    clamped.maritime_moderation = finite_or(
        clamped.maritime_moderation, defaults.maritime_moderation
    );
    clamped.maritime_influence_distance = finite_or(
        clamped.maritime_influence_distance, defaults.maritime_influence_distance
    );
    clamped.temperature_variation = finite_or(
        clamped.temperature_variation, defaults.temperature_variation
    );
    clamped.temperature_variation_frequency = finite_or(
        clamped.temperature_variation_frequency, defaults.temperature_variation_frequency
    );
    clamped.ambient_moisture = finite_or(
        clamped.ambient_moisture, defaults.ambient_moisture
    );
    clamped.ocean_moisture_source = finite_or(
        clamped.ocean_moisture_source, defaults.ocean_moisture_source
    );
    clamped.inland_water_moisture_source = finite_or(
        clamped.inland_water_moisture_source, defaults.inland_water_moisture_source
    );
    clamped.moisture_retention = finite_or(
        clamped.moisture_retention, defaults.moisture_retention
    );
    clamped.precipitation_efficiency = finite_or(
        clamped.precipitation_efficiency, defaults.precipitation_efficiency
    );
    clamped.orographic_lift = finite_or(
        clamped.orographic_lift, defaults.orographic_lift
    );
    clamped.rain_shadow_strength = finite_or(
        clamped.rain_shadow_strength, defaults.rain_shadow_strength
    );
    clamped.rain_shadow_decay = finite_or(
        clamped.rain_shadow_decay, defaults.rain_shadow_decay
    );
    clamped.precipitation_scale = finite_or(
        clamped.precipitation_scale, defaults.precipitation_scale
    );
    clamped.latitude_wind_band_strength = finite_or(
        clamped.latitude_wind_band_strength, defaults.latitude_wind_band_strength
    );
    clamped.secondary_wind_strength = finite_or(
        clamped.secondary_wind_strength, defaults.secondary_wind_strength
    );
    clamped.wind_seed_variation = finite_or(
        clamped.wind_seed_variation, defaults.wind_seed_variation
    );
    clamped.regional_wind_strength = finite_or(
        clamped.regional_wind_strength, defaults.regional_wind_strength
    );
    clamped.regional_wind_frequency = finite_or(
        clamped.regional_wind_frequency, defaults.regional_wind_frequency
    );
    clamped.precipitation_seed_variation = finite_or(
        clamped.precipitation_seed_variation, defaults.precipitation_seed_variation
    );
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
    if (!std::isfinite(clamped.prevailing_wind_degrees)) {
        clamped.prevailing_wind_degrees = 0.0f;
    }
    clamped.prevailing_wind_degrees = std::fmod(clamped.prevailing_wind_degrees, 360.0f);
    if (clamped.prevailing_wind_degrees < 0.0f) {
        clamped.prevailing_wind_degrees += 360.0f;
    }
    clamped.ambient_moisture = clamp01(clamped.ambient_moisture);
    clamped.ocean_moisture_source = clamp01(clamped.ocean_moisture_source);
    clamped.inland_water_moisture_source = clamp01(clamped.inland_water_moisture_source);
    clamped.moisture_retention = clamp01(clamped.moisture_retention);
    clamped.precipitation_efficiency = clamp01(clamped.precipitation_efficiency);
    clamped.orographic_lift = std::clamp(clamped.orographic_lift, 0.0f, 4.0f);
    clamped.rain_shadow_strength = clamp01(clamped.rain_shadow_strength);
    clamped.rain_shadow_decay = clamp01(clamped.rain_shadow_decay);
    clamped.precipitation_scale = std::clamp(clamped.precipitation_scale, 0.0f, 2.0f);
    clamped.latitude_wind_band_strength = clamp01(clamped.latitude_wind_band_strength);
    clamped.secondary_wind_strength = std::clamp(
        clamped.secondary_wind_strength, 0.0f, 0.5f
    );
    clamped.wind_seed_variation = clamp01(clamped.wind_seed_variation);
    clamped.regional_wind_strength = clamp01(clamped.regional_wind_strength);
    clamped.regional_wind_frequency = std::clamp(
        clamped.regional_wind_frequency, 0.25f, 8.0f
    );
    clamped.precipitation_seed_variation = clamp01(clamped.precipitation_seed_variation);
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

GreaterRealmPrecipitationCharacter derive_greater_realm_precipitation_character(
    Seed seed,
    float variation
) noexcept {
    const float amount = std::isfinite(variation) ? clamp01(variation) : 1.0f;
    if (amount == 0.0f) {
        return {};
    }

    const float wetness = mixed_grid_random01(seed, 0, 0, PRECIPITATION_CHARACTER_DOMAIN);
    GreaterRealmPrecipitationCharacter character;
    character.wetness_scale = lerp(1.0f, lerp(0.70f, 1.75f, wetness), amount);
    character.retention_scale = lerp(1.0f, lerp(1.35f, 0.50f, wetness), amount);
    return character;
}

GreaterRealmWindCharacter derive_greater_realm_wind_character(
    Seed seed,
    float variation
) noexcept {
    const float amount = std::isfinite(variation) ? clamp01(variation) : 1.0f;
    if (amount == 0.0f) {
        return {};
    }

    const auto sample = [seed](std::int32_t coordinate) {
        return mixed_grid_random01(seed, coordinate, 0, WIND_CHARACTER_DOMAIN)
            * 2.0f
            - 1.0f;
    };
    GreaterRealmWindCharacter character;
    character.global_rotation_degrees = sample(0) * 180.0f * amount;
    character.latitude_shift_degrees = sample(1) * 8.0f * amount;
    character.north_angle_offset_degrees = sample(2) * 20.0f * amount;
    character.south_angle_offset_degrees = sample(3) * 20.0f * amount;
    character.regional_strength_scale = lerp(0.0f, 0.8f + (sample(4) + 1.0f) * 0.2f, amount);
    return character;
}

GreaterRealmClimateMap generate_greater_realm_climate(
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateSettings& settings
) {
    GreaterRealmClimateMap climate;
    if (!terrain.has_expected_cell_count()) {
        climate.source_seed = terrain.seed;
        climate.source_width = terrain.width;
        climate.source_height = terrain.height;
        climate.source_cell_size = terrain.cell_size;
        climate.source_terrain_fingerprint = greater_realm_climate_source_fingerprint(terrain);
        return climate;
    }

    const auto clamped = clamp_greater_realm_climate_settings(settings);
    initialize_climate_map(climate, terrain);
    generate_temperature_normals(climate, terrain, clamped);
    generate_precipitation_normals(climate, terrain, clamped);
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

PrecipitationNormalSummary summarize_precipitation_normals(
    const GreaterRealmClimateMap& climate
) noexcept {
    PrecipitationNormalSummary summary;
    if (!climate.has_expected_cell_count() || climate.cells.empty()) {
        return summary;
    }

    summary.minimum = std::numeric_limits<float>::infinity();
    summary.maximum = -std::numeric_limits<float>::infinity();
    double total = 0.0;
    for (const auto& cell : climate.cells) {
        summary.minimum = std::min(summary.minimum, cell.precipitation_normal);
        summary.maximum = std::max(summary.maximum, cell.precipitation_normal);
        total += cell.precipitation_normal;
    }
    summary.sample_count = climate.cells.size();
    summary.mean = static_cast<float>(total / static_cast<double>(summary.sample_count));
    return summary;
}

void GreaterRealmClimateGenerationCache::invalidate(
    GreaterRealmClimateDirtyStage stage
) noexcept {
    m_pending_stages |= stage;
}

void GreaterRealmClimateGenerationCache::reset() noexcept {
    m_settings = GreaterRealmClimateSettings{};
    m_pending_stages = GreaterRealmClimateDirtyStage::Temperature
        | GreaterRealmClimateDirtyStage::Precipitation;
    m_initialized = false;
}

GreaterRealmClimateRegenerationResult GreaterRealmClimateGenerationCache::regenerate(
    GreaterRealmClimateMap& climate,
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateSettings& settings
) {
    const auto clamped = clamp_greater_realm_climate_settings(settings);
    GreaterRealmClimateDirtyStage dirty_stages = m_pending_stages;
    if (!m_initialized || !climate.source_matches(terrain)) {
        dirty_stages |= GreaterRealmClimateDirtyStage::Temperature
            | GreaterRealmClimateDirtyStage::Precipitation;
        if (terrain.has_expected_cell_count()) {
            initialize_climate_map(climate, terrain);
        }
    } else {
        dirty_stages |= climate_settings_dirty_stages(m_settings, clamped);
    }
    if (dirty_stages == GreaterRealmClimateDirtyStage::None) {
        return {};
    }

    if (!terrain.has_expected_cell_count()) {
        climate = generate_greater_realm_climate(terrain, clamped);
    } else {
        if (has_climate_dirty_stage(dirty_stages, GreaterRealmClimateDirtyStage::Temperature)) {
            generate_temperature_normals(climate, terrain, clamped);
        }
        if (has_climate_dirty_stage(dirty_stages, GreaterRealmClimateDirtyStage::Precipitation)) {
            generate_precipitation_normals(climate, terrain, clamped);
        }
    }
    m_settings = clamped;
    m_pending_stages = GreaterRealmClimateDirtyStage::None;
    m_initialized = climate.source_matches(terrain);
    return {dirty_stages};
}

} // namespace procgen
