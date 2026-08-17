#pragma once

#include "procgen/Biome.hpp"
#include "world/SeasonalClimate.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace world {

inline constexpr std::uint32_t RUNTIME_ATMOSPHERE_VERSION = 2;

enum class RuntimePrecipitationType : std::uint8_t {
    None,
    Rain,
    Snow
};

struct RuntimeWeatherSettings {
    procgen::Seed weather_seed{1};
    std::uint64_t region_identity{1};
    std::uint32_t schema_version{1};
    float weather_cell_size{8.0f};
    float update_cadence_seconds{900.0f};
    float temperature_anomaly_strength{0.16f};
    float pressure_variation_strength{0.35f};
    float wind_speed_scale{1.0f};
    float humidity_variation_strength{0.20f};
    float cloud_variation_strength{0.25f};
    float precipitation_threshold{0.58f};
    float precipitation_intensity_scale{1.8f};
    float state_memory{0.35f};

    [[nodiscard]] bool operator==(const RuntimeWeatherSettings&) const noexcept = default;
};

struct RuntimeAtmosphericCell {
    float temperature_anomaly{0.0f};
    float pressure_normal{0.5f};
    float wind_x{0.0f};
    float wind_y{0.0f};
    float humidity{0.0f};
    float cloud_cover{0.0f};
    float active_precipitation{0.0f};
    RuntimePrecipitationType active_precipitation_type{RuntimePrecipitationType::None};
};

struct RuntimeAtmosphericState {
    std::uint32_t version{RUNTIME_ATMOSPHERE_VERSION};
    std::uint32_t schema_version{1};
    procgen::Seed weather_seed{1};
    std::uint64_t region_identity{1};
    std::uint32_t source_width{0};
    std::uint32_t source_height{0};
    float source_cell_size{1.0f};
    float weather_cell_size{8.0f};
    std::uint64_t source_terrain_fingerprint{0};
    std::uint64_t source_climate_fingerprint{0};
    std::uint64_t source_seasonal_temperature_provenance_fingerprint{0};
    std::uint64_t source_seasonal_precipitation_provenance_fingerprint{0};
    std::uint64_t source_seasonal_temperature_fingerprint{0};
    std::uint64_t source_seasonal_precipitation_fingerprint{0};
    std::uint64_t settings_fingerprint{0};
    std::uint64_t simulation_tick{0};
    std::vector<RuntimeAtmosphericCell> cells;

    [[nodiscard]] std::size_t expected_cell_count() const noexcept;
    [[nodiscard]] bool has_expected_cell_count() const noexcept;
    [[nodiscard]] bool source_matches(
        const procgen::GreaterRealmMap& terrain,
        const procgen::GreaterRealmClimateMap& climate,
        const SeasonalTemperatureMap& seasonal_temperature,
        const SeasonalPrecipitationMap& seasonal_precipitation,
        const RuntimeWeatherSettings& settings
    ) const noexcept;
};

struct ClimateWeatherSample {
    bool valid{false};
    float annual_temperature_normal{0.5f};
    float annual_precipitation_normal{0.0f};
    float seasonal_temperature_offset{0.0f};
    float seasonal_temperature_normal{0.5f};
    float seasonal_precipitation_multiplier{1.0f};
    float seasonal_precipitation_normal{0.0f};
    float runtime_temperature_anomaly{0.0f};
    float pressure_normal{0.5f};
    float wind_x{0.0f};
    float wind_y{0.0f};
    float humidity{0.0f};
    float cloud_cover{0.0f};
    float active_precipitation{0.0f};
    RuntimePrecipitationType active_precipitation_type{RuntimePrecipitationType::None};
    float experienced_temperature_normal{0.5f};
    float experienced_precipitation_normal{0.0f};
    procgen::BiomeId biome_id{procgen::INVALID_BIOME_ID};
};

[[nodiscard]] RuntimeWeatherSettings clamp_runtime_weather_settings(
    const RuntimeWeatherSettings& settings
) noexcept;
[[nodiscard]] std::uint64_t runtime_weather_settings_fingerprint(
    const RuntimeWeatherSettings& settings
) noexcept;
[[nodiscard]] std::uint64_t seasonal_temperature_fingerprint(
    const SeasonalTemperatureMap& seasonal_temperature
) noexcept;
[[nodiscard]] std::uint64_t seasonal_precipitation_fingerprint(
    const SeasonalPrecipitationMap& seasonal_precipitation
) noexcept;
[[nodiscard]] std::uint64_t seasonal_temperature_provenance_fingerprint(
    const SeasonalTemperatureMap& seasonal_temperature
) noexcept;
[[nodiscard]] std::uint64_t seasonal_precipitation_provenance_fingerprint(
    const SeasonalPrecipitationMap& seasonal_precipitation
) noexcept;
[[nodiscard]] RuntimeAtmosphericState evolve_runtime_weather(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureMap& seasonal_temperature,
    const SeasonalPrecipitationMap& seasonal_precipitation,
    const RuntimeWeatherSettings& settings,
    std::uint64_t simulation_tick,
    const RuntimeAtmosphericState* previous_state = nullptr
);
[[nodiscard]] ClimateWeatherSample sample_climate_weather_cell(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureMap* seasonal_temperature,
    const SeasonalPrecipitationMap* seasonal_precipitation,
    const RuntimeAtmosphericState* weather,
    const procgen::GreaterRealmBiomeMap* biomes,
    std::uint32_t x,
    std::uint32_t y
) noexcept;

} // namespace world
