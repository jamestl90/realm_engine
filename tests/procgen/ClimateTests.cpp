#include "procgen/Climate.hpp"
#include "procgen/GreaterRealmDebug.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
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

bool test_shape_range_determinism_and_summary() {
    procgen::GreaterRealmGeneratorSettings terrain_settings;
    terrain_settings.seed = 24680;
    terrain_settings.width = 48;
    terrain_settings.height = 32;
    const auto terrain = procgen::generate_greater_realm(terrain_settings);

    const auto first = procgen::generate_greater_realm_climate(terrain);
    const auto second = procgen::generate_greater_realm_climate(terrain);
    const auto summary = procgen::summarize_temperature_normals(first);

    bool ok = require(first.source_matches(terrain), "climate map retains matching terrain identity");
    ok &= require(first.has_expected_cell_count(), "climate map shape matches its source dimensions");
    ok &= require(climate_values_match(first, second), "temperature generation is deterministic");
    ok &= require(
        std::all_of(first.cells.begin(), first.cells.end(), [](const auto& cell) {
            return std::isfinite(cell.temperature_normal)
                && cell.temperature_normal >= 0.0f
                && cell.temperature_normal <= 1.0f;
        }),
        "land and water temperature normals remain in the fixed 0..1 range"
    );
    ok &= require(
        summary.sample_count == terrain.cells.size()
            && summary.minimum >= 0.0f
            && summary.maximum <= 1.0f
            && summary.mean >= summary.minimum
            && summary.mean <= summary.maximum,
        "temperature summary reports a valid fixed-scale range and mean"
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
    settings.elevation_cooling += 0.1f;
    const auto settings_change = cache.regenerate(climate, terrain, settings);
    terrain.cells[0].elevation += 0.05f;
    const auto terrain_change = cache.regenerate(climate, terrain, settings);

    bool ok = require(
        first.rebuilt(procgen::GreaterRealmClimateDirtyStage::Temperature),
        "initial climate generation builds temperature"
    );
    ok &= require(
        unchanged.rebuilt_stages == procgen::GreaterRealmClimateDirtyStage::None
            && after_debug.rebuilt_stages == procgen::GreaterRealmClimateDirtyStage::None,
        "unchanged inputs and debug-only visualization do not rebuild temperature"
    );
    ok &= require(
        settings_change.rebuilt(procgen::GreaterRealmClimateDirtyStage::Temperature),
        "temperature-setting changes rebuild temperature only"
    );
    ok &= require(
        terrain_change.rebuilt(procgen::GreaterRealmClimateDirtyStage::Temperature)
            && climate.source_matches(terrain),
        "changed terrain content invalidates and relinks temperature output"
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
    ok &= test_source_identity_debug_view_and_terrain_immutability();
    ok &= test_climate_regeneration_locality();
    if (!ok) {
        return 1;
    }

    std::cout << "Greater realm temperature-normal tests passed.\n";
    return 0;
}
