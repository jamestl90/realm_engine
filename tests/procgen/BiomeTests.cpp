#include "procgen/Biome.hpp"
#include "procgen/GreaterRealmDebug.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

#if !defined(REALM_TEST_BUILD)
#error "BiomeTests.cpp must only be compiled for test builds"
#endif

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

procgen::GreaterRealmMap make_map(std::uint32_t width = 4) {
    procgen::GreaterRealmMap map;
    map.seed = 123;
    map.width = width;
    map.height = 1;
    map.cell_size = 1.0f;
    map.cells.resize(map.expected_cell_count());
    for (std::uint32_t x = 0; x < width; ++x) {
        auto& cell = map.cells[map.index(x, 0)];
        cell.x = static_cast<std::int32_t>(x);
        cell.elevation = 0.60f;
        cell.slope = 0.10f;
        cell.distance_to_coast = static_cast<float>(x);
        cell.terrain_form = procgen::TerrainForm::Plains;
    }
    return map;
}

procgen::GreaterRealmClimateMap make_climate(const procgen::GreaterRealmMap& map) {
    procgen::GreaterRealmClimateSettings settings;
    settings.temperature_variation = 0.0f;
    settings.precipitation_scale = 0.0f;
    auto climate = procgen::generate_greater_realm_climate(map, settings);
    for (auto& cell : climate.cells) {
        cell.temperature_normal = 0.50f;
        cell.precipitation_normal = 0.30f;
    }
    return climate;
}

procgen::GreaterRealmBiomeRule make_rule(
    procgen::BiomeId id,
    std::int32_t priority = 0
) {
    procgen::GreaterRealmBiomeRule rule;
    rule.biome_id = id;
    rule.priority = priority;
    return rule;
}

bool test_rule_validation() {
    procgen::GreaterRealmBiomeRuleSet valid;
    valid.identity = 42;
    valid.fallback_biome_id = 99;
    auto rule = make_rule(1);
    rule.terrain_form = procgen::TerrainForm::Hills;
    rule.water_class = procgen::BiomeWaterClass::Land;
    rule.elevation = procgen::BiomeValueRange{0.2f, 0.8f};
    rule.slope = procgen::BiomeValueRange{0.0f, 2.0f};
    rule.coast_distance = procgen::BiomeValueRange{0.0f, 100.0f};
    rule.temperature_normal = procgen::BiomeValueRange{0.1f, 0.9f};
    rule.precipitation_normal = procgen::BiomeValueRange{0.2f, 0.7f};
    valid.rules.push_back(rule);

    auto duplicate = valid;
    duplicate.rules.push_back(rule);
    auto invalid_range = valid;
    invalid_range.rules[0].temperature_normal = procgen::BiomeValueRange{0.8f, 0.2f};
    auto invalid_id = valid;
    invalid_id.rules[0].biome_id = procgen::INVALID_BIOME_ID;
    auto invalid_water = valid;
    invalid_water.rules[0].water_class = static_cast<procgen::BiomeWaterClass>(255);

    bool ok = require(
        procgen::validate_greater_realm_biome_rules(valid).valid(),
        "a rule set can constrain every supported generic input"
    );
    ok &= require(
        procgen::validate_greater_realm_biome_rules(duplicate).error
            == procgen::BiomeRuleValidationError::DuplicateBiomeId,
        "rule biome IDs must be unique"
    );
    ok &= require(
        procgen::validate_greater_realm_biome_rules(invalid_range).error
            == procgen::BiomeRuleValidationError::InvalidRange,
        "inverted or out-of-domain ranges are rejected"
    );
    ok &= require(
        procgen::validate_greater_realm_biome_rules(invalid_id).error
            == procgen::BiomeRuleValidationError::InvalidBiomeId,
        "the reserved unmatched ID cannot be application-assigned"
    );
    ok &= require(
        procgen::validate_greater_realm_biome_rules(invalid_water).error
            == procgen::BiomeRuleValidationError::InvalidWaterClass,
        "unknown water requirements are rejected"
    );
    return ok;
}

bool test_assignment_precedence_fallback_water_and_boundaries() {
    auto map = make_map();
    map.cells[0].is_water = true;
    map.cells[0].is_ocean = true;
    map.cells[0].terrain_form = procgen::TerrainForm::Ocean;
    map.cells[1].is_water = true;
    map.cells[1].terrain_form = procgen::TerrainForm::InlandWater;
    map.cells[2].elevation = 0.80f;
    map.cells[3].elevation = 0.95f;
    auto climate = make_climate(map);
    climate.cells[3].temperature_normal = 0.95f;

    procgen::GreaterRealmBiomeRuleSet rules;
    rules.fallback_biome_id = 90;
    auto ocean = make_rule(1, 100);
    ocean.water_class = procgen::BiomeWaterClass::Ocean;
    rules.rules.push_back(ocean);
    auto inland = make_rule(2, 100);
    inland.water_class = procgen::BiomeWaterClass::InlandWater;
    rules.rules.push_back(inland);
    auto temperate = make_rule(3, 10);
    temperate.water_class = procgen::BiomeWaterClass::Land;
    temperate.elevation = procgen::BiomeValueRange{0.60f, 0.80f};
    temperate.temperature_normal = procgen::BiomeValueRange{0.50f, 0.80f};
    temperate.precipitation_normal = procgen::BiomeValueRange{0.30f, 0.60f};
    rules.rules.push_back(temperate);
    auto high = make_rule(4, 20);
    high.water_class = procgen::BiomeWaterClass::Land;
    high.elevation = procgen::BiomeValueRange{0.80f, 1.0f};
    high.temperature_normal = procgen::BiomeValueRange{0.0f, 0.80f};
    rules.rules.push_back(high);

    const auto first = procgen::generate_greater_realm_biomes(map, climate, rules);
    const auto second = procgen::generate_greater_realm_biomes(map, climate, rules);
    bool ok = require(
        first.has_expected_cell_count() && first.sources_match(map, climate, rules),
        "biome output records matching terrain, climate, and rule identities"
    );
    ok &= require(
        first.cells[0].biome_id == 1 && first.cells[1].biome_id == 2,
        "ocean and inland-water rules classify stable water forms separately"
    );
    ok &= require(
        first.cells[2].biome_id == 4,
        "inclusive threshold boundaries match and higher priority wins"
    );
    ok &= require(first.cells[3].biome_id == 90, "fallback handles valid unmatched cells");
    ok &= require(
        std::equal(
            first.cells.begin(), first.cells.end(), second.cells.begin(),
            [](const auto& left, const auto& right) { return left.biome_id == right.biome_id; }
        ),
        "biome assignment is deterministic"
    );

    rules.fallback_biome_id.reset();
    const auto unmatched = procgen::generate_greater_realm_biomes(map, climate, rules);
    ok &= require(
        unmatched.cells[3].biome_id == procgen::INVALID_BIOME_ID,
        "cells remain explicitly unmatched when no rule or fallback applies"
    );
    return ok;
}

bool test_equal_priority_uses_declaration_order() {
    const auto map = make_map(1);
    const auto climate = make_climate(map);
    procgen::GreaterRealmBiomeRuleSet rules;
    rules.rules = {make_rule(11, 5), make_rule(12, 5)};
    const auto first = procgen::generate_greater_realm_biomes(map, climate, rules);
    std::reverse(rules.rules.begin(), rules.rules.end());
    const auto reversed = procgen::generate_greater_realm_biomes(map, climate, rules);
    return require(
        first.cells[0].biome_id == 11 && reversed.cells[0].biome_id == 12,
        "equal-priority ties deterministically select the first declared rule"
    );
}

bool test_source_identity_and_regeneration_locality() {
    auto map = make_map(3);
    procgen::GreaterRealmClimateSettings climate_settings;
    climate_settings.temperature_variation = 0.0f;
    procgen::GreaterRealmClimateGenerationCache climate_cache;
    procgen::GreaterRealmClimateMap climate;
    (void)climate_cache.regenerate(climate, map, climate_settings);

    procgen::GreaterRealmBiomeRuleSet rules;
    rules.fallback_biome_id = 21;
    procgen::GreaterRealmBiomeGenerationCache biome_cache;
    procgen::GreaterRealmBiomeMap biomes;
    const auto first = biome_cache.regenerate(biomes, map, climate, rules);
    const auto unchanged = biome_cache.regenerate(biomes, map, climate, rules);
    const auto terrain_fingerprint = procgen::greater_realm_climate_source_fingerprint(map);
    std::vector<float> temperatures;
    for (const auto& cell : climate.cells) temperatures.push_back(cell.temperature_normal);

    climate_settings.precipitation_scale = 1.5f;
    const auto climate_change = climate_cache.regenerate(climate, map, climate_settings);
    const auto dependent_biomes = biome_cache.regenerate(biomes, map, climate, rules);
    rules.identity += 1;
    const auto rule_change = biome_cache.regenerate(biomes, map, climate, rules);

    bool temperatures_unchanged = true;
    for (std::size_t index = 0; index < climate.cells.size(); ++index) {
        temperatures_unchanged &= temperatures[index] == climate.cells[index].temperature_normal;
    }
    bool ok = require(first.rebuilt_assignment(), "initial biome generation builds assignments");
    ok &= require(
        unchanged.rebuilt_stages == procgen::GreaterRealmBiomeDirtyStage::None,
        "unchanged biome sources perform no assignment work"
    );
    ok &= require(
        climate_change.rebuilt_stages == procgen::GreaterRealmClimateDirtyStage::Precipitation
            && temperatures_unchanged,
        "precipitation settings rebuild precipitation without rebuilding temperature"
    );
    ok &= require(
        dependent_biomes.rebuilt_assignment(),
        "changed climate output invalidates biome assignment"
    );
    ok &= require(rule_change.rebuilt_assignment(), "rule identity changes rebuild biomes only");
    ok &= require(
        terrain_fingerprint == procgen::greater_realm_climate_source_fingerprint(map),
        "climate and biome changes preserve canonical terrain"
    );
    return ok;
}

bool test_debug_view_uses_application_colours_and_rejects_stale_sources() {
    const auto map = make_map(2);
    const auto climate = make_climate(map);
    procgen::GreaterRealmBiomeRuleSet rules;
    rules.fallback_biome_id = 31;
    auto first_cell = make_rule(30, 10);
    first_cell.coast_distance = procgen::BiomeValueRange{0.0f, 0.0f};
    rules.rules.push_back(first_cell);
    const auto biomes = procgen::generate_greater_realm_biomes(map, climate, rules);

    procgen::GreaterRealmDebugOptions options;
    options.view = procgen::GreaterRealmDebugView::Biome;
    options.show_coastline = false;
    options.show_mountain_peaks = false;
    options.show_rivers = false;
    const std::vector<procgen::BiomeDebugColour> colours{{30, {1, 2, 3, 255}}};
    const auto image = procgen::build_greater_realm_debug_image(
        map, climate, biomes, colours, procgen::NORMALIZED_WATERLINE, options
    );

    auto stale_climate = climate;
    stale_climate.cells[0].precipitation_normal += 0.1f;
    const auto stale = procgen::build_greater_realm_debug_image(
        map, stale_climate, biomes, colours, procgen::NORMALIZED_WATERLINE, options
    );
    return require(
        image.has_expected_byte_count()
            && image.rgba[0] == 1 && image.rgba[1] == 2 && image.rgba[2] == 3
            && (image.rgba[4] != 1 || image.rgba[5] != 2 || image.rgba[6] != 3)
            && !stale.has_expected_byte_count(),
        "biome view uses application colours, neutral fallback, and source identity checks"
    );
}

} // namespace

int main() {
    bool ok = true;
    ok &= test_rule_validation();
    ok &= test_assignment_precedence_fallback_water_and_boundaries();
    ok &= test_equal_priority_uses_declaration_order();
    ok &= test_source_identity_and_regeneration_locality();
    ok &= test_debug_view_uses_application_colours_and_rejects_stale_sources();
    if (!ok) {
        return 1;
    }
    std::cout << "Greater realm application-driven biome tests passed.\n";
    return 0;
}
