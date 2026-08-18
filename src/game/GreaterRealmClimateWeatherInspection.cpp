#include "GreaterRealmClimateWeatherInspection.hpp"
#include "procgen/detail/GenerationUtility.hpp"
#include <algorithm>

namespace game {
namespace {

using procgen::detail::clamp01;

[[nodiscard]] procgen::DebugColour mix_colour(
    procgen::DebugColour from,
    procgen::DebugColour to,
    float amount
) noexcept {
    const float t = clamp01(amount);
    const auto channel = [t](std::uint8_t left, std::uint8_t right) {
        return static_cast<std::uint8_t>(std::clamp(
            static_cast<float>(left)
                + (static_cast<float>(right) - static_cast<float>(left)) * t,
            0.0f,
            255.0f
        ));
    };
    return {
        channel(from.r, to.r),
        channel(from.g, to.g),
        channel(from.b, to.b),
        channel(from.a, to.a)
    };
}

[[nodiscard]] procgen::DebugColour three_colour_gradient(
    float value,
    procgen::DebugColour low,
    procgen::DebugColour middle,
    procgen::DebugColour high
) noexcept {
    const float clamped = clamp01(value);
    return clamped < 0.5f
        ? mix_colour(low, middle, clamped * 2.0f)
        : mix_colour(middle, high, (clamped - 0.5f) * 2.0f);
}

[[nodiscard]] world::ClimateWeatherSample compose_validated_sample(
    const procgen::GreaterRealmClimateCell& climate,
    const world::SeasonalTemperatureCell& seasonal_temperature,
    const world::SeasonalPrecipitationCell& seasonal_precipitation
) noexcept {
    world::ClimateWeatherSample sample;
    sample.valid = true;
    sample.annual_temperature_normal = climate.temperature_normal;
    sample.annual_precipitation_normal = climate.precipitation_normal;
    sample.seasonal_temperature_offset = seasonal_temperature.seasonal_offset;
    sample.seasonal_temperature_normal = seasonal_temperature.seasonal_temperature_normal;
    sample.seasonal_precipitation_multiplier = seasonal_precipitation.seasonal_multiplier;
    sample.seasonal_precipitation_normal
        = seasonal_precipitation.seasonal_precipitation_normal;
    return sample;
}

[[nodiscard]] procgen::DebugColour colour_for_sample(
    GreaterRealmInspectionView view,
    const world::ClimateWeatherSample& sample
) noexcept {
    constexpr procgen::DebugColour cold{38, 82, 148, 255};
    constexpr procgen::DebugColour temperate{104, 172, 120, 255};
    constexpr procgen::DebugColour hot{222, 82, 48, 255};
    constexpr procgen::DebugColour dry{196, 158, 78, 255};
    constexpr procgen::DebugColour mild_wet{72, 158, 104, 255};
    constexpr procgen::DebugColour wet{46, 102, 178, 255};
    switch (view) {
        case GreaterRealmInspectionView::SeasonalTemperature:
            return three_colour_gradient(sample.seasonal_temperature_normal, cold, temperate, hot);
        case GreaterRealmInspectionView::SeasonalPrecipitation:
            return three_colour_gradient(sample.seasonal_precipitation_normal, dry, mild_wet, wet);
        default:
            return {255, 0, 255, 255};
    }
}

} // namespace

procgen::DebugImage build_greater_realm_climate_weather_inspection_image(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const world::SeasonalTemperatureMap& seasonal_temperature,
    const world::SeasonalPrecipitationMap& seasonal_precipitation,
    GreaterRealmInspectionView view,
    const procgen::GreaterRealmDebugOptions& overlay_options
) {
    procgen::DebugImage image;
    image.width = terrain.width;
    image.height = terrain.height;
    if (!terrain.has_expected_cell_count()
        || !climate.source_matches(terrain)
        || !seasonal_temperature.source_maps_match(terrain, climate)
        || !seasonal_precipitation.source_maps_match(terrain, climate)) {
        return image;
    }

    image.rgba.resize(image.expected_byte_count());
    for (std::size_t index = 0; index < terrain.cells.size(); ++index) {
        const auto sample = compose_validated_sample(
            climate.cells[index],
            seasonal_temperature.cells[index],
            seasonal_precipitation.cells[index]
        );
        const auto colour = colour_for_sample(view, sample);
        const std::size_t pixel = index * 4;
        image.rgba[pixel] = colour.r;
        image.rgba[pixel + 1] = colour.g;
        image.rgba[pixel + 2] = colour.b;
        image.rgba[pixel + 3] = colour.a;
    }

    procgen::apply_greater_realm_debug_overlays(image, terrain, overlay_options);
    return image;
}

} // namespace game
