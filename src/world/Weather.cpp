#include "../../include/world/Weather.hpp"

namespace world {

ClimateWeatherSample sample_climate_weather_cell(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureMap* seasonal_temperature,
    const SeasonalPrecipitationMap* seasonal_precipitation,
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
        && seasonal_temperature->source_maps_match(terrain, climate)
        && index < seasonal_temperature->cells.size()) {
        const auto& cell = seasonal_temperature->cells[index];
        sample.seasonal_temperature_offset = cell.seasonal_offset;
        sample.seasonal_temperature_normal = cell.seasonal_temperature_normal;
    }
    if (seasonal_precipitation != nullptr
        && seasonal_precipitation->source_maps_match(terrain, climate)
        && index < seasonal_precipitation->cells.size()) {
        const auto& cell = seasonal_precipitation->cells[index];
        sample.seasonal_precipitation_multiplier = cell.seasonal_multiplier;
        sample.seasonal_precipitation_normal = cell.seasonal_precipitation_normal;
    }
    if (biomes != nullptr
        && biomes->source_maps_match(terrain, climate)
        && index < biomes->cells.size()) {
        sample.biome_id = biomes->cells[index].biome_id;
    }
    return sample;
}

} // namespace world
