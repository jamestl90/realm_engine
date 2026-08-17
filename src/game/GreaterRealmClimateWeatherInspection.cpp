#include "GreaterRealmClimateWeatherInspection.hpp"
#include <algorithm>
#include <cmath>

namespace game {
namespace {

[[nodiscard]] float clamp01(float value) noexcept {
    return std::clamp(value, 0.0f, 1.0f);
}

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

void set_pixel(
    procgen::DebugImage& image,
    std::int32_t x,
    std::int32_t y,
    procgen::DebugColour colour
) noexcept {
    if (x < 0 || y < 0
        || x >= static_cast<std::int32_t>(image.width)
        || y >= static_cast<std::int32_t>(image.height)) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(y) * image.width
        + static_cast<std::size_t>(x);
    const std::size_t pixel = index * 4;
    image.rgba[pixel] = colour.r;
    image.rgba[pixel + 1] = colour.g;
    image.rgba[pixel + 2] = colour.b;
    image.rgba[pixel + 3] = colour.a;
}

void draw_line(
    procgen::DebugImage& image,
    std::int32_t x0,
    std::int32_t y0,
    std::int32_t x1,
    std::int32_t y1,
    procgen::DebugColour colour
) noexcept {
    const std::int32_t dx = std::abs(x1 - x0);
    const std::int32_t sx = x0 < x1 ? 1 : -1;
    const std::int32_t dy = -std::abs(y1 - y0);
    const std::int32_t sy = y0 < y1 ? 1 : -1;
    std::int32_t error = dx + dy;
    while (true) {
        set_pixel(image, x0, y0, colour);
        if (x0 == x1 && y0 == y1) {
            return;
        }
        const std::int32_t doubled = error * 2;
        if (doubled >= dy) {
            error += dy;
            x0 += sx;
        }
        if (doubled <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

[[nodiscard]] bool weather_sources_match(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const world::SeasonalTemperatureMap& seasonal_temperature,
    const world::SeasonalPrecipitationMap& seasonal_precipitation,
    const world::RuntimeAtmosphericState& weather
) noexcept {
    return weather.version == world::RUNTIME_ATMOSPHERE_VERSION
        && weather.source_width == terrain.width
        && weather.source_height == terrain.height
        && weather.source_cell_size == terrain.cell_size
        && weather.source_terrain_fingerprint
            == procgen::greater_realm_climate_source_fingerprint(terrain)
        && weather.source_climate_fingerprint
            == procgen::greater_realm_climate_fingerprint(climate)
        && weather.source_seasonal_temperature_fingerprint
            == world::seasonal_temperature_fingerprint(seasonal_temperature)
        && weather.source_seasonal_precipitation_fingerprint
            == world::seasonal_precipitation_fingerprint(seasonal_precipitation)
        && weather.has_expected_cell_count();
}

[[nodiscard]] world::ClimateWeatherSample compose_validated_sample(
    const procgen::GreaterRealmClimateCell& climate,
    const world::SeasonalTemperatureCell& seasonal_temperature,
    const world::SeasonalPrecipitationCell& seasonal_precipitation,
    const world::RuntimeAtmosphericCell& weather
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
    sample.runtime_temperature_anomaly = weather.temperature_anomaly;
    sample.pressure_normal = weather.pressure_normal;
    sample.wind_x = weather.wind_x;
    sample.wind_y = weather.wind_y;
    sample.humidity = weather.humidity;
    sample.cloud_cover = weather.cloud_cover;
    sample.active_precipitation = weather.active_precipitation;
    sample.active_precipitation_type = weather.active_precipitation_type;
    sample.experienced_temperature_normal = clamp01(
        sample.seasonal_temperature_normal + sample.runtime_temperature_anomaly
    );
    sample.experienced_precipitation_normal = clamp01(
        sample.seasonal_precipitation_normal + sample.active_precipitation
    );
    return sample;
}

[[nodiscard]] procgen::DebugColour colour_for_sample(
    GreaterRealmInspectionView view,
    const world::ClimateWeatherSample& sample,
    const world::RuntimeAtmosphericCell& weather_cell,
    float maximum_wind_speed
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
        case GreaterRealmInspectionView::TemperatureAnomaly:
            return three_colour_gradient(
                clamp01(sample.runtime_temperature_anomaly + 0.5f),
                {42, 96, 184, 255},
                {226, 226, 216, 255},
                {218, 66, 48, 255}
            );
        case GreaterRealmInspectionView::Pressure:
            return three_colour_gradient(
                sample.pressure_normal,
                {92, 62, 132, 255},
                {72, 146, 154, 255},
                {236, 218, 148, 255}
            );
        case GreaterRealmInspectionView::RuntimeWind: {
            const float speed = std::sqrt(
                weather_cell.wind_x * weather_cell.wind_x
                    + weather_cell.wind_y * weather_cell.wind_y
            );
            return mix_colour(
                {24, 30, 38, 255},
                {66, 158, 178, 255},
                speed / std::max(maximum_wind_speed, 0.0001f)
            );
        }
        case GreaterRealmInspectionView::Humidity:
            return three_colour_gradient(
                sample.humidity,
                {166, 128, 66, 255},
                {72, 154, 126, 255},
                {42, 104, 182, 255}
            );
        case GreaterRealmInspectionView::CloudCover:
            return three_colour_gradient(
                sample.cloud_cover,
                {34, 42, 50, 255},
                {132, 146, 152, 255},
                {242, 244, 240, 255}
            );
        case GreaterRealmInspectionView::ActivePrecipitation:
            if (sample.active_precipitation_type == world::RuntimePrecipitationType::Snow) {
                return mix_colour(
                    {38, 44, 52, 255},
                    {244, 248, 250, 255},
                    sample.active_precipitation
                );
            }
            return mix_colour(
                {38, 44, 52, 255},
                {42, 126, 224, 255},
                sample.active_precipitation
            );
        case GreaterRealmInspectionView::ExperiencedTemperature:
            return three_colour_gradient(sample.experienced_temperature_normal, cold, temperate, hot);
        case GreaterRealmInspectionView::ExperiencedPrecipitation:
            return three_colour_gradient(sample.experienced_precipitation_normal, dry, mild_wet, wet);
        default:
            return {255, 0, 255, 255};
    }
}

void draw_wind_vectors(
    procgen::DebugImage& image,
    const world::RuntimeAtmosphericState& weather
) noexcept {
    constexpr std::uint32_t stride = 12;
    constexpr float vector_length = 4.0f;
    constexpr procgen::DebugColour vector_colour{244, 248, 238, 255};
    for (std::uint32_t y = stride / 2; y < image.height; y += stride) {
        for (std::uint32_t x = stride / 2; x < image.width; x += stride) {
            const std::size_t index = static_cast<std::size_t>(y) * image.width + x;
            const auto& cell = weather.cells[index];
            const float speed = std::sqrt(cell.wind_x * cell.wind_x + cell.wind_y * cell.wind_y);
            if (speed <= 0.0001f) {
                continue;
            }
            const float direction_x = cell.wind_x / speed;
            const float direction_y = cell.wind_y / speed;
            const auto end_x = static_cast<std::int32_t>(std::round(
                static_cast<float>(x) + direction_x * vector_length
            ));
            const auto end_y = static_cast<std::int32_t>(std::round(
                static_cast<float>(y) + direction_y * vector_length
            ));
            draw_line(
                image,
                static_cast<std::int32_t>(x),
                static_cast<std::int32_t>(y),
                end_x,
                end_y,
                vector_colour
            );
            set_pixel(image, end_x, end_y, {255, 222, 112, 255});
        }
    }
}

} // namespace

procgen::DebugImage build_greater_realm_climate_weather_inspection_image(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const world::SeasonalTemperatureMap& seasonal_temperature,
    const world::SeasonalPrecipitationMap& seasonal_precipitation,
    const world::RuntimeAtmosphericState& weather,
    GreaterRealmInspectionView view,
    const procgen::GreaterRealmDebugOptions& overlay_options
) {
    procgen::DebugImage image;
    image.width = terrain.width;
    image.height = terrain.height;
    if (!terrain.has_expected_cell_count()
        || !climate.source_matches(terrain)
        || !seasonal_temperature.source_maps_match(terrain, climate)
        || !seasonal_precipitation.source_maps_match(terrain, climate)
        || !weather_sources_match(
            terrain,
            climate,
            seasonal_temperature,
            seasonal_precipitation,
            weather
        )) {
        return image;
    }

    float maximum_wind_speed = 0.0f;
    for (const auto& cell : weather.cells) {
        maximum_wind_speed = std::max(
            maximum_wind_speed,
            std::sqrt(cell.wind_x * cell.wind_x + cell.wind_y * cell.wind_y)
        );
    }

    image.rgba.resize(image.expected_byte_count());
    for (std::size_t index = 0; index < terrain.cells.size(); ++index) {
        const auto sample = compose_validated_sample(
            climate.cells[index],
            seasonal_temperature.cells[index],
            seasonal_precipitation.cells[index],
            weather.cells[index]
        );
        const auto colour = colour_for_sample(
            view,
            sample,
            weather.cells[index],
            maximum_wind_speed
        );
        const std::size_t pixel = index * 4;
        image.rgba[pixel] = colour.r;
        image.rgba[pixel + 1] = colour.g;
        image.rgba[pixel + 2] = colour.b;
        image.rgba[pixel + 3] = colour.a;
    }

    if (view == GreaterRealmInspectionView::RuntimeWind) {
        draw_wind_vectors(image, weather);
    }
    procgen::apply_greater_realm_debug_overlays(image, terrain, overlay_options);
    return image;
}

} // namespace game
