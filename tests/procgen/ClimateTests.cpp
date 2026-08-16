#include "procgen/Climate.hpp"
#include "procgen/GreaterRealmDebug.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#if !defined(REALM_TEST_BUILD)
#error "ClimateTests.cpp must only be compiled for test builds"
#endif

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool nearly_equal(float left, float right, float tolerance = 0.0001f) {
    return std::abs(left - right) <= tolerance;
}

procgen::GreaterRealmMap make_flat_map(
    std::uint32_t width,
    std::uint32_t height,
    procgen::Seed seed = 1
) {
    procgen::GreaterRealmMap map;
    map.seed = seed;
    map.width = width;
    map.height = height;
    map.cell_size = 1.0f;
    map.cells.resize(map.expected_cell_count());
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            auto& cell = map.cells[map.index(x, y)];
            cell.x = static_cast<std::int32_t>(x);
            cell.y = static_cast<std::int32_t>(y);
            cell.elevation = procgen::NORMALIZED_WATERLINE;
            cell.terrain_form = procgen::TerrainForm::Plains;
        }
    }
    return map;
}

bool climate_values_match(
    const procgen::GreaterRealmClimateMap& left,
    const procgen::GreaterRealmClimateMap& right
) {
    if (left.cells.size() != right.cells.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.cells.size(); ++index) {
        if (left.cells[index].temperature_normal
            != right.cells[index].temperature_normal) {
            return false;
        }
    }
    return true;
}

bool precipitation_values_match(
    const procgen::GreaterRealmClimateMap& left,
    const procgen::GreaterRealmClimateMap& right
) {
    if (left.cells.size() != right.cells.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.cells.size(); ++index) {
        if (left.cells[index].precipitation_normal
            != right.cells[index].precipitation_normal) {
            return false;
        }
    }
    return true;
}

bool test_shape_range_determinism_and_summary() {
    procgen::GreaterRealmGeneratorSettings terrain_settings;
    terrain_settings.seed = 24680;
    terrain_settings.width = 48;
    terrain_settings.height = 32;
    const auto terrain = procgen::generate_greater_realm(terrain_settings);

    const auto first = procgen::generate_greater_realm_climate(terrain);
    const auto second = procgen::generate_greater_realm_climate(terrain);
    const auto summary = procgen::summarize_temperature_normals(first);
    const auto precipitation_summary = procgen::summarize_precipitation_normals(first);

    bool ok = require(first.source_matches(terrain), "climate map retains matching terrain identity");
    ok &= require(first.has_expected_cell_count(), "climate map shape matches its source dimensions");
    ok &= require(
        climate_values_match(first, second) && precipitation_values_match(first, second),
        "temperature and precipitation generation are deterministic"
    );
    ok &= require(
        std::all_of(first.cells.begin(), first.cells.end(), [](const auto& cell) {
            return std::isfinite(cell.temperature_normal)
                && cell.temperature_normal >= 0.0f
                && cell.temperature_normal <= 1.0f
                && std::isfinite(cell.precipitation_normal)
                && cell.precipitation_normal >= 0.0f
                && cell.precipitation_normal <= 1.0f;
        }),
        "land and water climate normals remain in the fixed 0..1 range"
    );
    ok &= require(
        summary.sample_count == terrain.cells.size()
            && summary.minimum >= 0.0f
            && summary.maximum <= 1.0f
            && summary.mean >= summary.minimum
            && summary.mean <= summary.maximum,
        "temperature summary reports a valid fixed-scale range and mean"
    );
    ok &= require(
        precipitation_summary.sample_count == terrain.cells.size()
            && precipitation_summary.minimum >= 0.0f
            && precipitation_summary.maximum <= 1.0f
            && precipitation_summary.mean >= precipitation_summary.minimum
            && precipitation_summary.mean <= precipitation_summary.maximum,
        "precipitation summary reports a valid fixed-scale range and mean"
    );
    return ok;
}

bool test_latitude_validation_and_response() {
    const auto terrain = make_flat_map(1, 5);
    procgen::GreaterRealmClimateSettings settings;
    settings.north_edge_latitude_degrees = 120.0f;
    settings.south_edge_latitude_degrees = -120.0f;
    settings.elevation_cooling = 0.0f;
    settings.maritime_moderation = 0.0f;
    settings.temperature_variation = 0.0f;

    const auto clamped = procgen::clamp_greater_realm_climate_settings(settings);
    const auto climate = procgen::generate_greater_realm_climate(terrain, settings);
    bool ok = require(
        clamped.north_edge_latitude_degrees == 90.0f
            && clamped.south_edge_latitude_degrees == -90.0f,
        "edge latitudes are clamped to geographic range"
    );
    ok &= require(
        nearly_equal(procgen::greater_realm_latitude_for_row(settings, 2, 5), 0.0f),
        "map rows interpolate latitude between explicit edges"
    );
    ok &= require(
        nearly_equal(climate.cells[0].temperature_normal, 0.0f)
            && nearly_equal(climate.cells[2].temperature_normal, 1.0f)
            && nearly_equal(climate.cells[4].temperature_normal, 0.0f),
        "absolute latitude produces cold poles and a warm equator without map normalization"
    );
    return ok;
}

bool test_elevation_cooling() {
    auto terrain = make_flat_map(2, 1);
    terrain.cells[0].elevation = procgen::NORMALIZED_WATERLINE;
    terrain.cells[1].elevation = 1.0f;

    procgen::GreaterRealmClimateSettings settings;
    settings.north_edge_latitude_degrees = 0.0f;
    settings.south_edge_latitude_degrees = 0.0f;
    settings.elevation_cooling = 0.40f;
    settings.maritime_moderation = 0.0f;
    settings.temperature_variation = 0.0f;
    const auto climate = procgen::generate_greater_realm_climate(terrain, settings);

    return require(
        nearly_equal(climate.cells[0].temperature_normal, 1.0f)
            && nearly_equal(climate.cells[1].temperature_normal, 0.60f),
        "higher normalized land elevation cools temperature by the configured fixed weight"
    );
}

bool test_maritime_moderation() {
    auto terrain = make_flat_map(5, 1);
    terrain.cells[0].is_water = true;
    terrain.cells[0].is_ocean = true;
    terrain.cells[0].terrain_form = procgen::TerrainForm::Ocean;
    terrain.cells[0].elevation = 0.25f;

    procgen::GreaterRealmClimateSettings settings;
    settings.north_edge_latitude_degrees = 60.0f;
    settings.south_edge_latitude_degrees = 60.0f;
    settings.elevation_cooling = 0.0f;
    settings.maritime_moderation = 0.75f;
    settings.maritime_influence_distance = 3.0f;
    settings.temperature_variation = 0.0f;
    const auto climate = procgen::generate_greater_realm_climate(terrain, settings);

    return require(
        climate.cells[0].temperature_normal > climate.cells[1].temperature_normal
            && climate.cells[1].temperature_normal > climate.cells[3].temperature_normal
            && nearly_equal(climate.cells[3].temperature_normal, climate.cells[4].temperature_normal),
        "water proximity smoothly moderates temperature toward the fixed midpoint"
    );
}

bool test_seed_domain_behavior() {
    const auto first_terrain = make_flat_map(12, 8, 1001);
    const auto second_terrain = make_flat_map(12, 8, 2002);
    procgen::GreaterRealmClimateSettings settings;
    settings.elevation_cooling = 0.0f;
    settings.maritime_moderation = 0.0f;
    settings.temperature_variation = 0.0f;
    const auto neutral_first = procgen::generate_greater_realm_climate(first_terrain, settings);
    const auto neutral_second = procgen::generate_greater_realm_climate(second_terrain, settings);

    settings.temperature_variation = 0.20f;
    const auto varied_first = procgen::generate_greater_realm_climate(first_terrain, settings);
    const auto varied_second = procgen::generate_greater_realm_climate(second_terrain, settings);
    return require(
        climate_values_match(neutral_first, neutral_second)
            && !climate_values_match(varied_first, varied_second),
        "seed affects only the optional domain-separated broad temperature variation"
    );
}

bool test_precipitation_settings_validation_and_wind_response() {
    auto terrain = make_flat_map(7, 1);
    terrain.cells[0].is_water = true;
    terrain.cells[0].is_ocean = true;
    terrain.cells[0].terrain_form = procgen::TerrainForm::Ocean;
    terrain.cells[0].elevation = 0.25f;

    procgen::GreaterRealmClimateSettings settings;
    settings.prevailing_wind_degrees = 725.0f;
    settings.ambient_moisture = -1.0f;
    settings.moisture_retention = 1.0f;
    settings.precipitation_efficiency = 0.5f;
    settings.orographic_lift = 0.0f;
    settings.rain_shadow_strength = 0.0f;
    settings.latitude_wind_band_strength = -1.0f;
    settings.secondary_wind_strength = 2.0f;
    settings.precipitation_seed_variation = 0.0f;
    const auto clamped = procgen::clamp_greater_realm_climate_settings(settings);
    bool ok = require(
        nearly_equal(clamped.prevailing_wind_degrees, 5.0f)
            && clamped.ambient_moisture == 0.0f
            && clamped.latitude_wind_band_strength == 0.0f
            && clamped.secondary_wind_strength == 0.5f,
        "wind direction wraps to 0..360 and transport settings clamp to valid ranges"
    );
    settings.ocean_moisture_source = std::numeric_limits<float>::quiet_NaN();
    ok &= require(
        procgen::clamp_greater_realm_climate_settings(settings).ocean_moisture_source
            == procgen::GreaterRealmClimateSettings{}.ocean_moisture_source,
        "non-finite transport settings restore deterministic defaults"
    );

    settings = clamped;
    settings.prevailing_wind_degrees = 0.0f;
    const auto eastward = procgen::generate_greater_realm_climate(terrain, settings);
    settings.prevailing_wind_degrees = 180.0f;
    const auto westward = procgen::generate_greater_realm_climate(terrain, settings);
    ok &= require(
        eastward.cells[6].precipitation_normal > westward.cells[6].precipitation_normal,
        "prevailing wind transports water-source moisture downwind"
    );
    return ok;
}

bool test_ocean_and_inland_water_source_strengths() {
    auto terrain = make_flat_map(2, 1);
    terrain.cells[0].is_water = true;
    terrain.cells[0].is_ocean = true;
    terrain.cells[0].terrain_form = procgen::TerrainForm::Ocean;
    terrain.cells[1].is_water = true;
    terrain.cells[1].terrain_form = procgen::TerrainForm::InlandWater;

    procgen::GreaterRealmClimateSettings settings;
    settings.prevailing_wind_degrees = 90.0f;
    settings.ambient_moisture = 0.0f;
    settings.ocean_moisture_source = 1.0f;
    settings.inland_water_moisture_source = 0.4f;
    settings.precipitation_efficiency = 0.5f;
    settings.orographic_lift = 0.0f;
    settings.latitude_wind_band_strength = 0.0f;
    settings.precipitation_seed_variation = 0.0f;
    const auto climate = procgen::generate_greater_realm_climate(terrain, settings);
    return require(
        nearly_equal(climate.cells[0].precipitation_normal, 0.5f)
            && nearly_equal(climate.cells[1].precipitation_normal, 0.2f),
        "ocean and inland water use separate documented moisture-source strengths"
    );
}

bool test_orographic_lift_and_downwind_shadow() {
    auto terrain = make_flat_map(7, 1);
    terrain.cells[0].is_water = true;
    terrain.cells[0].is_ocean = true;
    terrain.cells[0].terrain_form = procgen::TerrainForm::Ocean;
    terrain.cells[0].elevation = 0.25f;
    terrain.cells[2].elevation = 0.65f;
    terrain.cells[3].elevation = 1.0f;

    procgen::GreaterRealmClimateSettings settings;
    settings.prevailing_wind_degrees = 0.0f;
    settings.ambient_moisture = 0.0f;
    settings.moisture_retention = 1.0f;
    settings.precipitation_efficiency = 0.25f;
    settings.orographic_lift = 1.5f;
    settings.rain_shadow_strength = 0.7f;
    settings.rain_shadow_decay = 0.95f;
    settings.latitude_wind_band_strength = 0.0f;
    settings.precipitation_seed_variation = 0.0f;
    const auto climate = procgen::generate_greater_realm_climate(terrain, settings);
    return require(
        climate.cells[3].precipitation_normal > climate.cells[1].precipitation_normal
            && climate.cells[4].precipitation_normal < climate.cells[1].precipitation_normal,
        "rising terrain increases precipitation and carries a dry shadow downwind"
    );
}

bool test_dry_wet_scale_and_hydrology_independence() {
    auto terrain = make_flat_map(8, 2);
    terrain.cells[0].is_water = true;
    terrain.cells[0].is_ocean = true;
    terrain.cells[0].terrain_form = procgen::TerrainForm::Ocean;
    procgen::GreaterRealmClimateSettings settings;
    settings.precipitation_seed_variation = 0.0f;
    settings.precipitation_scale = 0.5f;
    const auto dry = procgen::generate_greater_realm_climate(terrain, settings);
    settings.precipitation_scale = 1.5f;
    const auto wet = procgen::generate_greater_realm_climate(terrain, settings);

    auto hydrology_changed = terrain;
    for (auto& cell : hydrology_changed.cells) {
        cell.drainage_area = 10000.0f;
        cell.downslope_index = 0;
        cell.is_drainage_outlet = true;
    }
    hydrology_changed.rivers.push_back({0, 1, 10000.0f, 10.0f});
    const auto unchanged = procgen::generate_greater_realm_climate(hydrology_changed, settings);
    const auto dry_summary = procgen::summarize_precipitation_normals(dry);
    const auto wet_summary = procgen::summarize_precipitation_normals(wet);
    return require(
        wet_summary.mean > dry_summary.mean
            && precipitation_values_match(wet, unchanged),
        "wetness scale changes fixed output while drainage and channels do not affect climate"
    );
}

bool test_latitude_wind_bands_reverse_coastal_influence() {
    auto terrain = make_flat_map(31, 13, 31337);
    for (std::uint32_t y = 0; y < terrain.height; ++y) {
        for (const std::uint32_t x : {0u, terrain.width - 1}) {
            auto& cell = terrain.cells[terrain.index(x, y)];
            cell.is_water = true;
            cell.is_ocean = true;
            cell.terrain_form = procgen::TerrainForm::Ocean;
            cell.elevation = 0.25f;
        }
    }

    procgen::GreaterRealmClimateSettings settings;
    settings.ambient_moisture = 0.0f;
    settings.moisture_retention = 0.99f;
    settings.precipitation_efficiency = 0.4f;
    settings.orographic_lift = 0.0f;
    settings.rain_shadow_strength = 0.0f;
    settings.secondary_wind_strength = 0.0f;
    settings.precipitation_seed_variation = 0.0f;
    const auto climate = procgen::generate_greater_realm_climate(terrain, settings);

    const auto precipitation = [&](std::uint32_t x, std::uint32_t y) {
        return climate.cells[terrain.index(x, y)].precipitation_normal;
    };
    return require(
        precipitation(2, 1) > precipitation(terrain.width - 3, 1)
            && precipitation(2, terrain.height / 2)
                < precipitation(terrain.width - 3, terrain.height / 2)
            && precipitation(2, terrain.height - 2)
                > precipitation(terrain.width - 3, terrain.height - 2),
        "latitude circulation makes mid-latitudes and tropics favor opposing coasts"
    );
}

bool test_seed_driven_precipitation_character() {
    const auto first = procgen::derive_greater_realm_precipitation_character(101, 1.0f);
    const auto repeated = procgen::derive_greater_realm_precipitation_character(101, 1.0f);
    const auto different = procgen::derive_greater_realm_precipitation_character(202, 1.0f);
    const auto neutral_first = procgen::derive_greater_realm_precipitation_character(101, 0.0f);
    const auto neutral_second = procgen::derive_greater_realm_precipitation_character(202, 0.0f);

    return require(
        first == repeated
            && first != different
            && neutral_first == procgen::GreaterRealmPrecipitationCharacter{}
            && neutral_first == neutral_second,
        "realm precipitation character is deterministic and variation zero is exactly neutral"
    );
}

bool test_source_identity_debug_view_and_terrain_immutability() {
    auto terrain = make_flat_map(4, 3, 77);
    terrain.cells[0].is_water = true;
    terrain.cells[0].terrain_form = procgen::TerrainForm::InlandWater;
    const auto elevations_before = [&terrain]() {
        std::vector<float> values;
        values.reserve(terrain.cells.size());
        for (const auto& cell : terrain.cells) values.push_back(cell.elevation);
        return values;
    }();
    const auto fingerprint_before = procgen::greater_realm_climate_source_fingerprint(terrain);
    const auto climate = procgen::generate_greater_realm_climate(terrain);

    procgen::GreaterRealmDebugOptions options;
    options.view = procgen::GreaterRealmDebugView::TemperatureNormal;
    options.show_coastline = false;
    options.show_mountain_peaks = false;
    options.show_rivers = false;
    const auto image = procgen::build_greater_realm_debug_image(
        terrain, climate, procgen::NORMALIZED_WATERLINE, options
    );

    terrain.cells[1].elevation += 0.1f;
    const auto stale_image = procgen::build_greater_realm_debug_image(
        terrain, climate, procgen::NORMALIZED_WATERLINE, options
    );
    terrain.cells[1].elevation -= 0.1f;

    std::vector<float> elevations_after;
    for (const auto& cell : terrain.cells) elevations_after.push_back(cell.elevation);
    bool ok = require(
        fingerprint_before == procgen::greater_realm_climate_source_fingerprint(terrain)
            && elevations_before == elevations_after,
        "climate generation leaves canonical terrain data unchanged"
    );
    ok &= require(image.has_expected_byte_count(), "temperature debug view renders current climate data");
    ok &= require(
        !stale_image.has_expected_byte_count(),
        "temperature debug view rejects climate data from a changed terrain source"
    );
    return ok;
}

bool test_climate_regeneration_locality() {
    auto terrain = make_flat_map(8, 6, 99);
    procgen::GreaterRealmClimateSettings settings;
    procgen::GreaterRealmClimateGenerationCache cache;
    procgen::GreaterRealmClimateMap climate;

    const auto first = cache.regenerate(climate, terrain, settings);
    const auto unchanged = cache.regenerate(climate, terrain, settings);
    procgen::GreaterRealmDebugOptions options;
    options.view = procgen::GreaterRealmDebugView::TemperatureNormal;
    (void)procgen::build_greater_realm_debug_image(
        terrain, climate, procgen::NORMALIZED_WATERLINE, options
    );
    const auto after_debug = cache.regenerate(climate, terrain, settings);
    std::vector<float> precipitation_before;
    for (const auto& cell : climate.cells) {
        precipitation_before.push_back(cell.precipitation_normal);
    }
    settings.elevation_cooling += 0.1f;
    const auto temperature_change = cache.regenerate(climate, terrain, settings);
    bool precipitation_unchanged = true;
    for (std::size_t index = 0; index < climate.cells.size(); ++index) {
        precipitation_unchanged &= precipitation_before[index]
            == climate.cells[index].precipitation_normal;
    }
    std::vector<float> temperature_before;
    for (const auto& cell : climate.cells) {
        temperature_before.push_back(cell.temperature_normal);
    }
    settings.precipitation_scale += 0.2f;
    const auto precipitation_change = cache.regenerate(climate, terrain, settings);
    bool temperature_unchanged = true;
    for (std::size_t index = 0; index < climate.cells.size(); ++index) {
        temperature_unchanged &= temperature_before[index] == climate.cells[index].temperature_normal;
    }
    terrain.cells[0].elevation += 0.05f;
    const auto terrain_change = cache.regenerate(climate, terrain, settings);

    bool ok = require(
        first.rebuilt(procgen::GreaterRealmClimateDirtyStage::Temperature)
            && first.rebuilt(procgen::GreaterRealmClimateDirtyStage::Precipitation),
        "initial climate generation builds both normal fields"
    );
    ok &= require(
        unchanged.rebuilt_stages == procgen::GreaterRealmClimateDirtyStage::None
            && after_debug.rebuilt_stages == procgen::GreaterRealmClimateDirtyStage::None,
        "unchanged inputs and debug-only visualization do not rebuild climate"
    );
    ok &= require(
        temperature_change.rebuilt_stages == procgen::GreaterRealmClimateDirtyStage::Temperature
            && precipitation_unchanged,
        "temperature-setting changes rebuild temperature only"
    );
    ok &= require(
        precipitation_change.rebuilt_stages
                == procgen::GreaterRealmClimateDirtyStage::Precipitation
            && temperature_unchanged,
        "precipitation-setting changes rebuild precipitation only"
    );
    ok &= require(
        terrain_change.rebuilt(procgen::GreaterRealmClimateDirtyStage::Temperature)
            && terrain_change.rebuilt(procgen::GreaterRealmClimateDirtyStage::Precipitation)
            && climate.source_matches(terrain),
        "changed terrain content invalidates and relinks both climate outputs"
    );
    return ok;
}

} // namespace

int main() {
    bool ok = true;
    ok &= test_shape_range_determinism_and_summary();
    ok &= test_latitude_validation_and_response();
    ok &= test_elevation_cooling();
    ok &= test_maritime_moderation();
    ok &= test_seed_domain_behavior();
    ok &= test_precipitation_settings_validation_and_wind_response();
    ok &= test_ocean_and_inland_water_source_strengths();
    ok &= test_orographic_lift_and_downwind_shadow();
    ok &= test_dry_wet_scale_and_hydrology_independence();
    ok &= test_latitude_wind_bands_reverse_coastal_influence();
    ok &= test_seed_driven_precipitation_character();
    ok &= test_source_identity_debug_view_and_terrain_immutability();
    ok &= test_climate_regeneration_locality();
    if (!ok) {
        return 1;
    }

    std::cout << "Greater realm climate-normal tests passed.\n";
    return 0;
}
