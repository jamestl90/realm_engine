#pragma once

#include "procgen/Biome.hpp"
#include "world/SeasonalClimate.hpp"
#include <cstddef>
#include <cstdint>

namespace world {

struct ClimateWeatherSample {
    bool valid{false};
    float annual_temperature_normal{0.5f};
    float annual_precipitation_normal{0.0f};
    float seasonal_temperature_offset{0.0f};
    float seasonal_temperature_normal{0.5f};
    float seasonal_precipitation_multiplier{1.0f};
    float seasonal_precipitation_normal{0.0f};
    procgen::BiomeId biome_id{procgen::INVALID_BIOME_ID};
};

[[nodiscard]] ClimateWeatherSample sample_climate_weather_cell(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureMap* seasonal_temperature,
    const SeasonalPrecipitationMap* seasonal_precipitation,
    const procgen::GreaterRealmBiomeMap* biomes,
    std::uint32_t x,
    std::uint32_t y
) noexcept;

} // namespace world
