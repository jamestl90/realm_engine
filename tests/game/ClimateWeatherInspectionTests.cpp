#include "game/GreaterRealmClimateWeatherInspection.hpp"
#include "procgen/Climate.hpp"
#include "procgen/GreaterRealm.hpp"
#include "world/SeasonalClimate.hpp"
#include "world/Weather.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
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
    world::RuntimeWeatherSettings weather_settings;
    weather_settings.weather_seed = 97531;
    weather_settings.region_identity = 24680;
    weather_settings.precipitation_threshold = 0.0f;
    const auto weather = world::evolve_runtime_weather(
        terrain,
        climate,
        seasonal_temperature,
        seasonal_precipitation,
        weather_settings,
        17
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
            game::GreaterRealmInspectionView::ExperiencedPrecipitation
         );
         ++value) {
        images.push_back(game::build_greater_realm_climate_weather_inspection_image(
            terrain,
            climate,
            seasonal_temperature,
            seasonal_precipitation,
            weather,
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
            "every seasonal and runtime inspection layer produces a complete image"
        )) {
        return false;
    }
    bool ok = true;
    ok &= require(
        !images_match(images.front(), images.back()),
        "seasonal temperature and experienced precipitation use distinct visual fields"
    );

    const std::size_t wind_offset = static_cast<std::size_t>(
        static_cast<std::uint8_t>(game::GreaterRealmInspectionView::RuntimeWind)
            - static_cast<std::uint8_t>(game::GreaterRealmInspectionView::SeasonalTemperature)
    );
    ok &= require(
        pixel_at(images[wind_offset], 6, 6)
            == procgen::DebugColour{244, 248, 238, 255},
        "runtime wind view draws sampled vector origins over wind-speed colour"
    );

    const auto wind_repeat = game::build_greater_realm_climate_weather_inspection_image(
        terrain,
        climate,
        seasonal_temperature,
        seasonal_precipitation,
        weather,
        game::GreaterRealmInspectionView::RuntimeWind,
        overlays
    );
    ok &= require(
        images_match(images[wind_offset], wind_repeat),
        "inspection rendering is deterministic for explicit seasonal and weather inputs"
    );

    auto changed_climate = climate;
    changed_climate.cells[0].temperature_normal += 0.01f;
    const auto stale = game::build_greater_realm_climate_weather_inspection_image(
        terrain,
        changed_climate,
        seasonal_temperature,
        seasonal_precipitation,
        weather,
        game::GreaterRealmInspectionView::Humidity,
        overlays
    );
    ok &= require(
        !stale.has_expected_byte_count(),
        "inspection rendering rejects seasonal and weather data from stale climate sources"
    );

    procgen::GreaterRealmDebugView stable_view;
    ok &= require(
        game::procgen_debug_view_for(
            game::GreaterRealmInspectionView::AnnualPrecipitation,
            stable_view
        )
            && stable_view == procgen::GreaterRealmDebugView::PrecipitationNormal
            && !game::procgen_debug_view_for(
                game::GreaterRealmInspectionView::RuntimeWind,
                stable_view
            ),
        "unified selector maps stable views to procgen without treating runtime wind as climate"
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
    const auto weather = world::evolve_runtime_weather(
        terrain,
        climate,
        seasonal_temperature,
        seasonal_precipitation,
        world::RuntimeWeatherSettings{},
        23
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
        weather,
        game::GreaterRealmInspectionView::ExperiencedPrecipitation,
        overlays
    );
    const auto elapsed = std::chrono::steady_clock::now() - started_at;
    const auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        elapsed
    ).count();
    std::cout << "Greater-realm weather inspection rendered in "
              << elapsed_milliseconds << " ms.\n";

    bool ok = require(
        image.has_expected_byte_count(),
        "greater-realm-resolution weather inspection produces a complete image"
    );
    if (elapsed_milliseconds >= 2000) {
        std::cerr << "Greater-realm inspection render took "
                  << elapsed_milliseconds << " ms.\n";
    }
    ok &= require(
        elapsed_milliseconds < 2000,
        "greater-realm-resolution weather inspection remains interactive"
    );
    return ok;
}

} // namespace

int main() {
    if (!test_climate_weather_inspection_views()) {
        return 1;
    }
    if (!test_greater_realm_resolution_rendering_is_interactive()) {
        return 1;
    }
    std::cout << "Climate and weather inspection tests passed.\n";
    return 0;
}
