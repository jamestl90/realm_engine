#include "game/GreaterRealmClimateWeatherInspection.hpp"
#include "procgen/Climate.hpp"
#include "procgen/GreaterRealm.hpp"
#include "world/SeasonalClimate.hpp"
#include "world/Weather.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <vector>

#if !defined(REALM_TEST_BUILD)
#error "ClimateWeatherInspectionTests.cpp must only be compiled for test builds"
#endif

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool images_match(const procgen::DebugImage& left, const procgen::DebugImage& right) {
    return left.width == right.width
        && left.height == right.height
        && left.rgba == right.rgba;
}

procgen::DebugColour pixel_at(
    const procgen::DebugImage& image,
    std::uint32_t x,
    std::uint32_t y
) {
    const std::size_t pixel = (
        static_cast<std::size_t>(y) * image.width + static_cast<std::size_t>(x)
    ) * 4;
    return {
        image.rgba[pixel],
        image.rgba[pixel + 1],
        image.rgba[pixel + 2],
        image.rgba[pixel + 3]
    };
}

int colour_distance(procgen::DebugColour left, procgen::DebugColour right) {
    return std::abs(static_cast<int>(left.r) - static_cast<int>(right.r))
        + std::abs(static_cast<int>(left.g) - static_cast<int>(right.g))
        + std::abs(static_cast<int>(left.b) - static_cast<int>(right.b));
}

bool test_climate_weather_inspection_views() {
    procgen::GreaterRealmGeneratorSettings terrain_settings;
    terrain_settings.seed = 24680;
    terrain_settings.width = 24;
    terrain_settings.height = 24;
    const auto terrain = procgen::generate_greater_realm(terrain_settings);
    const auto climate = procgen::generate_greater_realm_climate(terrain);
    const auto seasonal_temperature = world::evaluate_seasonal_temperature(
        terrain,
        climate,
        world::SeasonalTemperatureSettings{},
        0.25f
    );
    const auto seasonal_precipitation = world::evaluate_seasonal_precipitation(
        terrain,
        climate,
        world::SeasonalPrecipitationSettings{},
        0.25f
    );
    procgen::GreaterRealmDebugOptions overlays;
    overlays.show_coastline = false;
    overlays.show_mountain_peaks = false;
    overlays.show_rivers = false;
    overlays.show_drainage_directions = false;

    std::vector<procgen::DebugImage> images;
    for (std::uint8_t value = static_cast<std::uint8_t>(
            game::GreaterRealmInspectionView::SeasonalTemperature
        );
         value <= static_cast<std::uint8_t>(
            game::GreaterRealmInspectionView::SeasonalPrecipitation
         );
         ++value) {
        images.push_back(game::build_greater_realm_climate_weather_inspection_image(
            terrain,
            climate,
            seasonal_temperature,
            seasonal_precipitation,
            static_cast<game::GreaterRealmInspectionView>(value),
            overlays
        ));
    }

    const bool all_images_complete = std::all_of(
        images.begin(),
        images.end(),
        [](const auto& image) { return image.has_expected_byte_count(); }
    );
    if (!require(
            all_images_complete,
            "every seasonal inspection layer produces a complete image"
        )) {
        return false;
    }
    bool ok = true;
    ok &= require(
        !images_match(images.front(), images.back()),
        "seasonal temperature and seasonal precipitation use distinct visual fields"
    );

    const auto precipitation_repeat = game::build_greater_realm_climate_weather_inspection_image(
        terrain,
        climate,
        seasonal_temperature,
        seasonal_precipitation,
        game::GreaterRealmInspectionView::SeasonalPrecipitation,
        overlays
    );
    ok &= require(
        images_match(images.back(), precipitation_repeat),
        "inspection rendering is deterministic for explicit seasonal inputs"
    );

    auto changed_climate = climate;
    changed_climate.cells[0].temperature_normal += 0.01f;
    const auto stale = game::build_greater_realm_climate_weather_inspection_image(
        terrain,
        changed_climate,
        seasonal_temperature,
        seasonal_precipitation,
        game::GreaterRealmInspectionView::SeasonalTemperature,
        overlays
    );
    ok &= require(
        !stale.has_expected_byte_count(),
        "inspection rendering rejects seasonal data from stale climate sources"
    );

    procgen::GreaterRealmDebugView stable_view;
    ok &= require(
        game::procgen_debug_view_for(
            game::GreaterRealmInspectionView::AnnualPrecipitation,
            stable_view
        )
            && stable_view == procgen::GreaterRealmDebugView::PrecipitationNormal
            && !game::procgen_debug_view_for(
                game::GreaterRealmInspectionView::SeasonalPrecipitation,
                stable_view
            ),
        "unified selector maps only stable annual views to procgen"
    );
    return ok;
}

bool test_greater_realm_resolution_rendering_is_interactive() {
    procgen::GreaterRealmGeneratorSettings terrain_settings;
    terrain_settings.seed = 13579;
    terrain_settings.width = 256;
    terrain_settings.height = 192;
    const auto terrain = procgen::generate_greater_realm(terrain_settings);
    const auto climate = procgen::generate_greater_realm_climate(terrain);
    const auto seasonal_temperature = world::evaluate_seasonal_temperature(
        terrain,
        climate,
        world::SeasonalTemperatureSettings{},
        0.75f
    );
    const auto seasonal_precipitation = world::evaluate_seasonal_precipitation(
        terrain,
        climate,
        world::SeasonalPrecipitationSettings{},
        0.75f
    );
    procgen::GreaterRealmDebugOptions overlays;
    overlays.show_coastline = false;
    overlays.show_mountain_peaks = false;
    overlays.show_rivers = false;
    overlays.show_drainage_directions = false;

    const auto started_at = std::chrono::steady_clock::now();
    const auto image = game::build_greater_realm_climate_weather_inspection_image(
        terrain,
        climate,
        seasonal_temperature,
        seasonal_precipitation,
        game::GreaterRealmInspectionView::SeasonalPrecipitation,
        overlays
    );
    const auto elapsed = std::chrono::steady_clock::now() - started_at;
    const auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        elapsed
    ).count();
    std::cout << "Greater-realm seasonal climate inspection rendered in "
              << elapsed_milliseconds << " ms.\n";

    bool ok = require(
        image.has_expected_byte_count(),
        "greater-realm-resolution seasonal inspection produces a complete image"
    );
    if (elapsed_milliseconds >= 2000) {
        std::cerr << "Greater-realm inspection render took "
                  << elapsed_milliseconds << " ms.\n";
    }
    ok &= require(
        elapsed_milliseconds < 2000,
        "greater-realm-resolution seasonal inspection remains interactive"
    );
    return ok;
}

bool test_all_world_inspection_views_are_continuous_at_equator() {
    procgen::GreaterRealmGeneratorSettings terrain_settings;
    terrain_settings.seed = 86420;
    terrain_settings.width = 3;
    terrain_settings.height = 193;
    const auto terrain = procgen::generate_greater_realm(terrain_settings);
    auto climate = procgen::generate_greater_realm_climate(terrain);
    for (auto& cell : climate.cells) {
        cell.temperature_normal = 0.50f;
        cell.precipitation_normal = 0.50f;
    }

    world::SeasonalTemperatureSettings temperature_settings;
    temperature_settings.base_amplitude = 0.20f;
    temperature_settings.latitude_amplitude = 0.0f;
    temperature_settings.elevation_amplitude = 0.0f;
    temperature_settings.maritime_damping = 0.0f;
    temperature_settings.northern_peak_year_fraction = 0.0f;
    temperature_settings.southern_peak_year_fraction = 0.50f;
    const auto seasonal_temperature = world::evaluate_seasonal_temperature(
        terrain,
        climate,
        temperature_settings,
        0.0f
    );

    world::SeasonalPrecipitationSettings precipitation_settings;
    precipitation_settings.base_amplitude = 0.40f;
    precipitation_settings.latitude_amplitude = 0.0f;
    precipitation_settings.inland_damping = 0.0f;
    precipitation_settings.northern_wet_peak_year_fraction = 0.0f;
    precipitation_settings.southern_wet_peak_year_fraction = 0.50f;
    const auto seasonal_precipitation = world::evaluate_seasonal_precipitation(
        terrain,
        climate,
        precipitation_settings,
        0.0f
    );

    procgen::GreaterRealmDebugOptions overlays;
    overlays.show_coastline = false;
    overlays.show_mountain_peaks = false;
    overlays.show_rivers = false;
    overlays.show_drainage_directions = false;
    for (std::uint8_t value = static_cast<std::uint8_t>(
            game::GreaterRealmInspectionView::SeasonalTemperature
        );
         value <= static_cast<std::uint8_t>(
            game::GreaterRealmInspectionView::SeasonalPrecipitation
         );
         ++value) {
        const auto image = game::build_greater_realm_climate_weather_inspection_image(
            terrain,
            climate,
            seasonal_temperature,
            seasonal_precipitation,
            static_cast<game::GreaterRealmInspectionView>(value),
            overlays
        );
        if (!require(
                colour_distance(pixel_at(image, 1, 95), pixel_at(image, 1, 97)) < 64,
                "seasonal inspection views do not introduce an equatorial seam"
            )) {
            return false;
        }
    }
    return true;
}

} // namespace

int main() {
    if (!test_climate_weather_inspection_views()) {
        return 1;
    }
    if (!test_greater_realm_resolution_rendering_is_interactive()) {
        return 1;
    }
    if (!test_all_world_inspection_views_are_continuous_at_equator()) {
        return 1;
    }
    std::cout << "Climate inspection tests passed.\n";
    return 0;
}
