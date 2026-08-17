#include "../../include/world/Weather.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace world {
namespace {

constexpr std::uint64_t RUNTIME_WEATHER_DOMAIN = 0x7274776561746865ull;

[[nodiscard]] float clamp01(float value) noexcept {
    return std::clamp(value, 0.0f, 1.0f);
}

[[nodiscard]] float lerp(float from, float to, float amount) noexcept {
    return from + (to - from) * amount;
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

void initialize_runtime_state(
    RuntimeAtmosphericState& state,
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureMap& seasonal_temperature,
    const SeasonalPrecipitationMap& seasonal_precipitation,
    const RuntimeWeatherSettings& settings,
    std::uint64_t simulation_tick
) {
    state.version = RUNTIME_ATMOSPHERE_VERSION;
    state.schema_version = settings.schema_version;
    state.weather_seed = settings.weather_seed;
    state.region_identity = settings.region_identity;
    state.source_width = terrain.width;
    state.source_height = terrain.height;
    state.source_cell_size = terrain.cell_size;
    state.weather_cell_size = settings.weather_cell_size;
    state.source_terrain_fingerprint =
        procgen::greater_realm_climate_source_fingerprint(terrain);
    state.source_climate_fingerprint = procgen::greater_realm_climate_fingerprint(climate);
    state.source_seasonal_temperature_provenance_fingerprint =
        seasonal_temperature_provenance_fingerprint(seasonal_temperature);
    state.source_seasonal_precipitation_provenance_fingerprint =
        seasonal_precipitation_provenance_fingerprint(seasonal_precipitation);
    state.source_seasonal_temperature_fingerprint =
        seasonal_temperature_fingerprint(seasonal_temperature);
    state.source_seasonal_precipitation_fingerprint =
        seasonal_precipitation_fingerprint(seasonal_precipitation);
    state.settings_fingerprint = runtime_weather_settings_fingerprint(settings);
    state.simulation_tick = simulation_tick;
    state.cells.assign(terrain.cells.size(), RuntimeAtmosphericCell{});
}

[[nodiscard]] bool previous_state_can_blend(
    const RuntimeAtmosphericState* previous,
    const RuntimeAtmosphericState& target_identity,
    std::uint64_t simulation_tick
) noexcept {
    return previous != nullptr
        && previous->version == target_identity.version
        && previous->schema_version == target_identity.schema_version
        && previous->weather_seed == target_identity.weather_seed
        && previous->region_identity == target_identity.region_identity
        && previous->source_width == target_identity.source_width
        && previous->source_height == target_identity.source_height
        && previous->source_cell_size == target_identity.source_cell_size
        && previous->weather_cell_size == target_identity.weather_cell_size
        && previous->source_terrain_fingerprint == target_identity.source_terrain_fingerprint
        && previous->source_climate_fingerprint == target_identity.source_climate_fingerprint
        && previous->source_seasonal_temperature_provenance_fingerprint
            == target_identity.source_seasonal_temperature_provenance_fingerprint
        && previous->source_seasonal_precipitation_provenance_fingerprint
            == target_identity.source_seasonal_precipitation_provenance_fingerprint
        && previous->settings_fingerprint == target_identity.settings_fingerprint
        && previous->has_expected_cell_count()
        && previous->simulation_tick < simulation_tick;
}

} // namespace

std::size_t RuntimeAtmosphericState::expected_cell_count() const noexcept {
    return static_cast<std::size_t>(source_width) * static_cast<std::size_t>(source_height);
}

bool RuntimeAtmosphericState::has_expected_cell_count() const noexcept {
    return cells.size() == expected_cell_count();
}

bool RuntimeAtmosphericState::source_matches(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureMap& seasonal_temperature,
    const SeasonalPrecipitationMap& seasonal_precipitation,
    const RuntimeWeatherSettings& settings
) const noexcept {
    return version == RUNTIME_ATMOSPHERE_VERSION
        && schema_version == settings.schema_version
        && weather_seed == settings.weather_seed
        && region_identity == settings.region_identity
        && source_width == terrain.width
        && source_height == terrain.height
        && source_cell_size == terrain.cell_size
        && weather_cell_size == clamp_runtime_weather_settings(settings).weather_cell_size
        && source_terrain_fingerprint
            == procgen::greater_realm_climate_source_fingerprint(terrain)
        && source_climate_fingerprint == procgen::greater_realm_climate_fingerprint(climate)
        && source_seasonal_temperature_provenance_fingerprint
            == seasonal_temperature_provenance_fingerprint(seasonal_temperature)
        && source_seasonal_precipitation_provenance_fingerprint
            == seasonal_precipitation_provenance_fingerprint(seasonal_precipitation)
        && source_seasonal_temperature_fingerprint
            == seasonal_temperature_fingerprint(seasonal_temperature)
        && source_seasonal_precipitation_fingerprint
            == seasonal_precipitation_fingerprint(seasonal_precipitation)
        && settings_fingerprint == runtime_weather_settings_fingerprint(settings)
        && terrain.has_expected_cell_count()
        && climate.source_matches(terrain)
        && seasonal_temperature.source_maps_match(terrain, climate)
        && seasonal_precipitation.source_maps_match(terrain, climate)
        && has_expected_cell_count();
}

RuntimeWeatherSettings clamp_runtime_weather_settings(
    const RuntimeWeatherSettings& settings
) noexcept {
    RuntimeWeatherSettings clamped = settings;
    const RuntimeWeatherSettings defaults;
    const auto finite_or = [](float value, float fallback) {
        return std::isfinite(value) ? value : fallback;
    };
    clamped.weather_cell_size = finite_or(clamped.weather_cell_size, defaults.weather_cell_size);
    clamped.update_cadence_seconds = finite_or(
        clamped.update_cadence_seconds,
        defaults.update_cadence_seconds
    );
    clamped.temperature_anomaly_strength = finite_or(
        clamped.temperature_anomaly_strength,
        defaults.temperature_anomaly_strength
    );
    clamped.pressure_variation_strength = finite_or(
        clamped.pressure_variation_strength,
        defaults.pressure_variation_strength
    );
    clamped.wind_speed_scale = finite_or(clamped.wind_speed_scale, defaults.wind_speed_scale);
    clamped.humidity_variation_strength = finite_or(
        clamped.humidity_variation_strength,
        defaults.humidity_variation_strength
    );
    clamped.cloud_variation_strength = finite_or(
        clamped.cloud_variation_strength,
        defaults.cloud_variation_strength
    );
    clamped.precipitation_threshold = finite_or(
        clamped.precipitation_threshold,
        defaults.precipitation_threshold
    );
    clamped.precipitation_intensity_scale = finite_or(
        clamped.precipitation_intensity_scale,
        defaults.precipitation_intensity_scale
    );
    clamped.state_memory = finite_or(clamped.state_memory, defaults.state_memory);

    clamped.schema_version = std::max(clamped.schema_version, 1u);
    clamped.weather_cell_size = std::max(clamped.weather_cell_size, 1.0f);
    clamped.update_cadence_seconds = std::max(clamped.update_cadence_seconds, 1.0f);
    clamped.temperature_anomaly_strength = std::clamp(
        clamped.temperature_anomaly_strength, 0.0f, 0.5f
    );
    clamped.pressure_variation_strength = clamp01(clamped.pressure_variation_strength);
    clamped.wind_speed_scale = std::clamp(clamped.wind_speed_scale, 0.0f, 4.0f);
    clamped.humidity_variation_strength = clamp01(clamped.humidity_variation_strength);
    clamped.cloud_variation_strength = clamp01(clamped.cloud_variation_strength);
    clamped.precipitation_threshold = clamp01(clamped.precipitation_threshold);
    clamped.precipitation_intensity_scale = std::clamp(
        clamped.precipitation_intensity_scale, 0.0f, 4.0f
    );
    clamped.state_memory = clamp01(clamped.state_memory);
    return clamped;
}

std::uint64_t runtime_weather_settings_fingerprint(
    const RuntimeWeatherSettings& settings
) noexcept {
    const auto clamped = clamp_runtime_weather_settings(settings);
    std::uint64_t hash = mix_hash(clamped.weather_seed, clamped.region_identity);
    hash = mix_hash(hash, clamped.schema_version);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.weather_cell_size));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.update_cadence_seconds));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.temperature_anomaly_strength));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.pressure_variation_strength));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.wind_speed_scale));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.humidity_variation_strength));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.cloud_variation_strength));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.precipitation_threshold));
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.precipitation_intensity_scale));
    return mix_hash(hash, std::bit_cast<std::uint32_t>(clamped.state_memory));
}

std::uint64_t seasonal_temperature_fingerprint(
    const SeasonalTemperatureMap& seasonal_temperature
) noexcept {
    std::uint64_t hash = seasonal_temperature_provenance_fingerprint(seasonal_temperature);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(seasonal_temperature.year_fraction));
    for (const auto& cell : seasonal_temperature.cells) {
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(cell.seasonal_offset));
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(cell.seasonal_temperature_normal));
    }
    return hash;
}

std::uint64_t seasonal_precipitation_fingerprint(
    const SeasonalPrecipitationMap& seasonal_precipitation
) noexcept {
    std::uint64_t hash = seasonal_precipitation_provenance_fingerprint(
        seasonal_precipitation
    );
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(seasonal_precipitation.year_fraction));
    for (const auto& cell : seasonal_precipitation.cells) {
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(cell.seasonal_multiplier));
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(
            cell.seasonal_precipitation_normal
        ));
    }
    return hash;
}

std::uint64_t seasonal_temperature_provenance_fingerprint(
    const SeasonalTemperatureMap& seasonal_temperature
) noexcept {
    std::uint64_t hash = mix_hash(seasonal_temperature.version, seasonal_temperature.source_seed);
    hash = mix_hash(hash, seasonal_temperature.source_width);
    hash = mix_hash(hash, seasonal_temperature.source_height);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(
        seasonal_temperature.source_cell_size
    ));
    hash = mix_hash(hash, seasonal_temperature.source_terrain_fingerprint);
    hash = mix_hash(hash, seasonal_temperature.source_temperature_fingerprint);
    return mix_hash(hash, seasonal_temperature.settings_fingerprint);
}

std::uint64_t seasonal_precipitation_provenance_fingerprint(
    const SeasonalPrecipitationMap& seasonal_precipitation
) noexcept {
    std::uint64_t hash = mix_hash(
        seasonal_precipitation.version,
        seasonal_precipitation.source_seed
    );
    hash = mix_hash(hash, seasonal_precipitation.source_width);
    hash = mix_hash(hash, seasonal_precipitation.source_height);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(
        seasonal_precipitation.source_cell_size
    ));
    hash = mix_hash(hash, seasonal_precipitation.source_terrain_fingerprint);
    hash = mix_hash(hash, seasonal_precipitation.source_precipitation_fingerprint);
    return mix_hash(hash, seasonal_precipitation.settings_fingerprint);
}

RuntimeAtmosphericState evolve_runtime_weather(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureMap& seasonal_temperature,
    const SeasonalPrecipitationMap& seasonal_precipitation,
    const RuntimeWeatherSettings& settings,
    std::uint64_t simulation_tick,
    const RuntimeAtmosphericState* previous_state
) {
    const auto clamped = clamp_runtime_weather_settings(settings);
    RuntimeAtmosphericState state;
    initialize_runtime_state(
        state,
        terrain,
        climate,
        seasonal_temperature,
        seasonal_precipitation,
        clamped,
        simulation_tick
    );
    if (!terrain.has_expected_cell_count()
        || !climate.source_matches(terrain)
        || !seasonal_temperature.source_maps_match(terrain, climate)
        || !seasonal_precipitation.source_maps_match(terrain, climate)) {
        state.cells.clear();
        return state;
    }

    constexpr float PI = 3.14159265358979323846f;
    const bool can_blend = previous_state_can_blend(previous_state, state, simulation_tick);
    const float memory = can_blend ? clamped.state_memory : 0.0f;
    for (std::uint32_t y = 0; y < terrain.height; ++y) {
        for (std::uint32_t x = 0; x < terrain.width; ++x) {
            const std::size_t index = terrain.index(x, y);
            const auto tick = static_cast<std::int32_t>(simulation_tick & 0x7fffffffull);
            const auto sx = static_cast<std::int32_t>(x);
            const auto sy = static_cast<std::int32_t>(y);
            const float pressure_noise = random01(
                clamped.weather_seed,
                sx + tick * 17,
                sy,
                RUNTIME_WEATHER_DOMAIN
            );
            const float humidity_noise = random01(
                clamped.weather_seed,
                sx,
                sy + tick * 19,
                RUNTIME_WEATHER_DOMAIN + 1013ull
            ) * 2.0f - 1.0f;
            const float cloud_noise = random01(
                clamped.weather_seed,
                sx + tick * 23,
                sy + tick * 29,
                RUNTIME_WEATHER_DOMAIN + 2029ull
            ) * 2.0f - 1.0f;
            const float wind_angle = random01(
                clamped.weather_seed,
                sx + tick * 31,
                sy + tick * 37,
                RUNTIME_WEATHER_DOMAIN + 3037ull
            ) * 2.0f * PI;
            const float anomaly_noise = random01(
                clamped.weather_seed,
                sx + tick * 41,
                sy + tick * 43,
                RUNTIME_WEATHER_DOMAIN + 4049ull
            ) * 2.0f - 1.0f;

            RuntimeAtmosphericCell target;
            target.pressure_normal = clamp01(
                0.5f + (pressure_noise - 0.5f) * clamped.pressure_variation_strength
            );
            target.temperature_anomaly = anomaly_noise
                * clamped.temperature_anomaly_strength
                * (0.5f + std::abs(target.pressure_normal - 0.5f));
            target.wind_x = std::cos(wind_angle) * clamped.wind_speed_scale;
            target.wind_y = std::sin(wind_angle) * clamped.wind_speed_scale;
            target.humidity = clamp01(
                seasonal_precipitation.cells[index].seasonal_precipitation_normal
                + humidity_noise * clamped.humidity_variation_strength
            );
            target.cloud_cover = clamp01(
                target.humidity * 0.65f
                + (1.0f - target.pressure_normal) * 0.35f
                + cloud_noise * clamped.cloud_variation_strength
            );
            target.active_precipitation = clamp01(
                (target.humidity * target.cloud_cover - clamped.precipitation_threshold)
                    * clamped.precipitation_intensity_scale
            );
            if (target.active_precipitation <= 0.0f) {
                target.active_precipitation_type = RuntimePrecipitationType::None;
            } else if (seasonal_temperature.cells[index].seasonal_temperature_normal < 0.35f) {
                target.active_precipitation_type = RuntimePrecipitationType::Snow;
            } else {
                target.active_precipitation_type = RuntimePrecipitationType::Rain;
            }

            auto& cell = state.cells[index];
            if (can_blend) {
                const auto& previous = previous_state->cells[index];
                cell.temperature_anomaly = lerp(
                    target.temperature_anomaly,
                    previous.temperature_anomaly,
                    memory
                );
                cell.pressure_normal = lerp(target.pressure_normal, previous.pressure_normal, memory);
                cell.wind_x = lerp(target.wind_x, previous.wind_x, memory);
                cell.wind_y = lerp(target.wind_y, previous.wind_y, memory);
                cell.humidity = clamp01(lerp(target.humidity, previous.humidity, memory));
                cell.cloud_cover = clamp01(lerp(target.cloud_cover, previous.cloud_cover, memory));
                cell.active_precipitation = clamp01(lerp(
                    target.active_precipitation,
                    previous.active_precipitation,
                    memory
                ));
                cell.active_precipitation_type = cell.active_precipitation <= 0.0f
                    ? RuntimePrecipitationType::None
                    : target.active_precipitation_type;
            } else {
                cell = target;
            }
        }
    }
    return state;
}

ClimateWeatherSample sample_climate_weather_cell(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureMap* seasonal_temperature,
    const SeasonalPrecipitationMap* seasonal_precipitation,
    const RuntimeAtmosphericState* weather,
    const procgen::GreaterRealmBiomeMap* biomes,
    std::uint32_t x,
    std::uint32_t y
) noexcept {
    ClimateWeatherSample sample;
    if (!terrain.contains(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y))
        || !climate.source_matches(terrain)) {
        return sample;
    }
    const std::size_t index = terrain.index(x, y);
    sample.valid = true;
    sample.annual_temperature_normal = climate.cells[index].temperature_normal;
    sample.annual_precipitation_normal = climate.cells[index].precipitation_normal;
    sample.seasonal_temperature_normal = sample.annual_temperature_normal;
    sample.seasonal_precipitation_normal = sample.annual_precipitation_normal;

    if (seasonal_temperature != nullptr
        && seasonal_temperature->has_expected_cell_count()
        && index < seasonal_temperature->cells.size()) {
        const auto& cell = seasonal_temperature->cells[index];
        sample.seasonal_temperature_offset = cell.seasonal_offset;
        sample.seasonal_temperature_normal = cell.seasonal_temperature_normal;
    }
    if (seasonal_precipitation != nullptr
        && seasonal_precipitation->has_expected_cell_count()
        && index < seasonal_precipitation->cells.size()) {
        const auto& cell = seasonal_precipitation->cells[index];
        sample.seasonal_precipitation_multiplier = cell.seasonal_multiplier;
        sample.seasonal_precipitation_normal = cell.seasonal_precipitation_normal;
    }
    if (weather != nullptr && weather->has_expected_cell_count() && index < weather->cells.size()) {
        const auto& cell = weather->cells[index];
        sample.runtime_temperature_anomaly = cell.temperature_anomaly;
        sample.pressure_normal = cell.pressure_normal;
        sample.wind_x = cell.wind_x;
        sample.wind_y = cell.wind_y;
        sample.humidity = cell.humidity;
        sample.cloud_cover = cell.cloud_cover;
        sample.active_precipitation = cell.active_precipitation;
        sample.active_precipitation_type = cell.active_precipitation_type;
    }
    sample.experienced_temperature_normal = clamp01(
        sample.seasonal_temperature_normal + sample.runtime_temperature_anomaly
    );
    sample.experienced_precipitation_normal = clamp01(
        sample.seasonal_precipitation_normal + sample.active_precipitation
    );
    if (biomes != nullptr
        && biomes->source_maps_match(terrain, climate)
        && index < biomes->cells.size()) {
        sample.biome_id = biomes->cells[index].biome_id;
    }
    return sample;
}

} // namespace world
