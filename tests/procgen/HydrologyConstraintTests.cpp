#if !defined(REALM_TEST_BUILD)
#error This test module must only be compiled in test builds.
#endif

#include "procgen/GreaterRealm.hpp"
#include "procgen/GreaterRealmDebug.hpp"
#include "procgen/TerrainConstraints.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool complete_maps_match(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    if (a.seed != b.seed
        || a.width != b.width
        || a.height != b.height
        || a.drainage_order != b.drainage_order
        || a.rivers.size() != b.rivers.size()
        || a.cells.size() != b.cells.size()) {
        return false;
    }

    for (std::size_t index = 0; index < a.cells.size(); ++index) {
        const auto& left = a.cells[index];
        const auto& right = b.cells[index];
        if (left.landmass_elevation != right.landmass_elevation
            || left.hill_relief != right.hill_relief
            || left.mountain_relief != right.mountain_relief
            || left.elevation != right.elevation
            || left.drainage_elevation != right.drainage_elevation
            || left.drainage_area != right.drainage_area
            || left.downslope_index != right.downslope_index
            || left.is_drainage_outlet != right.is_drainage_outlet) {
            return false;
        }
    }

    for (std::size_t index = 0; index < a.rivers.size(); ++index) {
        const auto& left = a.rivers[index];
        const auto& right = b.rivers[index];
        if (left.source_index != right.source_index
            || left.destination_index != right.destination_index
            || left.drainage_area != right.drainage_area
            || left.width != right.width) {
            return false;
        }
    }
    return true;
}

bool topology_matches(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    if (a.cells.size() != b.cells.size()) {
        return false;
    }
    for (std::size_t index = 0; index < a.cells.size(); ++index) {
        if (a.cells[index].is_water != b.cells[index].is_water
            || a.cells[index].is_ocean != b.cells[index].is_ocean
            || a.cells[index].elevation != b.cells[index].elevation) {
            return false;
        }
    }
    return true;
}

float normalized_coord(std::uint32_t coordinate, std::uint32_t extent) {
    return extent > 1 ? static_cast<float>(coordinate) / static_cast<float>(extent - 1) : 0.0f;
}

float normalized_distance_from_center(
    const procgen::GreaterRealmMap& map,
    std::uint32_t x,
    std::uint32_t y
) {
    const float dx = normalized_coord(x, map.width) - 0.5f;
    const float dy = normalized_coord(y, map.height) - 0.5f;
    return std::sqrt(dx * dx + dy * dy);
}

procgen::GreaterRealmGeneratorSettings one_stage_constraint_test_settings() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 424242;
    settings.width = 65;
    settings.height = 65;
    settings.coastline_noise_weight = 0.0f;
    settings.mountain_weight = 0.0f;
    settings.ridge_weight = 0.0f;
    settings.valley_weight = 0.0f;
    settings.terrain_noise_weight = 0.0f;
    return settings;
}

bool river_export_candidate(
    const procgen::GreaterRealmMap& map,
    std::uint32_t source_index,
    float minimum_area
) {
    if (source_index >= map.cells.size()) {
        return false;
    }

    const auto& source = map.cells[source_index];
    if (source.is_water
        || source.is_coastal
        || source.downslope_index == procgen::INVALID_CELL_INDEX
        || source.downslope_index >= map.cells.size()
        || source.drainage_area < minimum_area) {
        return false;
    }

    const auto& destination = map.cells[source.downslope_index];
    if (destination.is_coastal
        || destination.drainage_elevation > source.drainage_elevation) {
        return false;
    }
    if (source.distance_to_coast <= 3.0f
        && destination.distance_to_coast >= source.distance_to_coast) {
        return false;
    }
    return true;
}

bool has_upstream_river_candidate(
    const procgen::GreaterRealmMap& map,
    std::uint32_t destination_index,
    float minimum_area
) {
    for (std::uint32_t source_index = 0; source_index < map.cells.size(); ++source_index) {
        if (map.cells[source_index].downslope_index == destination_index
            && river_export_candidate(map, source_index, minimum_area)) {
            return true;
        }
    }
    return false;
}

bool test_constraint_tools_sampling_and_serialization() {
    procgen::TerrainConstraintField field(17, 13);
    field.paint(procgen::TerrainConstraintTool::Mountain, 0.5f, 0.5f, 0.3f);

    const auto center = field.sample(0.5f, 0.5f);
    const auto shoulder = field.sample(0.62f, 0.5f);
    const auto edge = field.sample(0.0f, 0.0f);
    const auto bytes = procgen::serialize_terrain_constraints(field);
    const auto restored = procgen::deserialize_terrain_constraints(bytes);

    bool ok = true;
    ok &= require(procgen::terrain_constraint_value(procgen::TerrainConstraintTool::Ocean) == -0.25f, "ocean tool uses Mapgen4's signed value");
    ok &= require(procgen::terrain_constraint_value(procgen::TerrainConstraintTool::ShallowWater) == -0.05f, "shallow-water tool uses Mapgen4's signed value");
    ok &= require(procgen::terrain_constraint_value(procgen::TerrainConstraintTool::Valley) == 0.05f, "valley tool uses Mapgen4's signed value");
    ok &= require(procgen::terrain_constraint_value(procgen::TerrainConstraintTool::Mountain) == 1.0f, "mountain tool uses Mapgen4's signed value");
    ok &= require(center.influence > shoulder.influence && shoulder.influence > edge.influence, "constraint brush has smooth spatial falloff");
    ok &= require(center.elevation > shoulder.elevation && shoulder.elevation > edge.elevation, "constraint field bilinearly samples painted elevation");
    ok &= require(restored.has_value(), "serialized constraint field round trips");
    if (restored) {
        const auto restored_center = restored->sample(0.5f, 0.5f);
        const auto restored_shoulder = restored->sample(0.62f, 0.5f);
        ok &= require(restored->width() == field.width() && restored->height() == field.height(), "constraint dimensions survive serialization");
        ok &= require(restored_center.elevation == center.elevation && restored_center.influence == center.influence, "constraint center survives serialization exactly");
        ok &= require(restored_shoulder.elevation == shoulder.elevation && restored_shoulder.influence == shoulder.influence, "interpolated constraint data survives serialization");
    }

    std::vector<std::uint8_t> malformed(bytes.begin(), bytes.end() - 1);
    ok &= require(!procgen::deserialize_terrain_constraints(malformed).has_value(), "constraint deserializer rejects truncated data");
    return ok;
}

bool test_constraints_have_local_deterministic_influence() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 314159;
    settings.width = 96;
    settings.height = 72;
    const auto baseline = procgen::generate_greater_realm(settings);

    procgen::TerrainConstraintField constraints(32, 24);
    constraints.paint(procgen::TerrainConstraintTool::Mountain, 0.5f, 0.5f, 0.16f);
    const auto edited = procgen::generate_greater_realm(settings, constraints);
    const auto repeated = procgen::generate_greater_realm(settings, constraints);
    const auto center = edited.index(edited.width / 2, edited.height / 2);
    const auto corner = edited.index(0, 0);

    bool ok = true;
    ok &= require(edited.cells[center].landmass_elevation > baseline.cells[center].landmass_elevation, "mountain constraint raises the local signed terrain field");
    ok &= require(edited.cells[center].elevation > baseline.cells[center].elevation, "mountain constraint raises local final elevation");
    ok &= require(edited.cells[corner].landmass_elevation == baseline.cells[corner].landmass_elevation, "local constraint leaves distant topology unchanged");
    ok &= require(complete_maps_match(edited, repeated), "authored constraints preserve deterministic complete output");
    return ok;
}

bool test_constraint_tools_route_through_signed_field_once() {
    const auto settings = one_stage_constraint_test_settings();
    const auto baseline = procgen::generate_greater_realm(settings);
    const auto center_index = baseline.index(settings.width / 2, settings.height / 2);
    const auto shoulder_index = baseline.index(settings.width / 2 + 8, settings.height / 2);
    const auto outside_index = baseline.index(0, 0);

    struct ToolExpectation {
        procgen::TerrainConstraintTool tool;
        bool center_is_water;
        const char* name;
    };

    const std::array tools{
        ToolExpectation{procgen::TerrainConstraintTool::Ocean, true, "ocean"},
        ToolExpectation{procgen::TerrainConstraintTool::ShallowWater, true, "shallow-water"},
        ToolExpectation{procgen::TerrainConstraintTool::Valley, false, "valley"},
        ToolExpectation{procgen::TerrainConstraintTool::Mountain, false, "mountain"}
    };

    bool ok = true;
    std::array<float, 4> center_elevations{};
    std::size_t tool_index = 0;
    for (const auto& expectation : tools) {
        procgen::TerrainConstraintField constraints(65, 65);
        constraints.paint(expectation.tool, 0.5f, 0.5f, 0.20f);
        const auto center_sample = constraints.sample(0.5f, 0.5f);
        const auto shoulder_sample = constraints.sample(
            normalized_coord(settings.width / 2 + 8, settings.width),
            0.5f
        );
        const auto edited = procgen::generate_greater_realm(settings, constraints);

        const auto& center = edited.cells[center_index];
        const auto& shoulder = edited.cells[shoulder_index];
        const auto& outside = edited.cells[outside_index];
        const auto& baseline_center = baseline.cells[center_index];
        const auto& baseline_shoulder = baseline.cells[shoulder_index];
        const auto& baseline_outside = baseline.cells[outside_index];

        const float center_delta = std::abs(center.landmass_elevation - baseline_center.landmass_elevation);
        const float shoulder_delta = std::abs(shoulder.landmass_elevation - baseline_shoulder.landmass_elevation);
        ok &= require(center_sample.influence > shoulder_sample.influence, "authored field has stronger center than shoulder influence");
        ok &= require(center_delta > 0.0f, "signed landmass responds at brush center");
        ok &= require(shoulder_delta > 0.0f, "signed landmass responds at brush shoulder");
        ok &= require(outside.landmass_elevation == baseline_outside.landmass_elevation, "outside brush signed field is unchanged");
        ok &= require(outside.elevation == baseline_outside.elevation, "outside brush final elevation is unchanged");
        ok &= require(center.is_water == expectation.center_is_water, expectation.name);
        if (!center.is_water) {
            ok &= require(center.elevation > settings.sea_level, "positive constraint center becomes land above sea level");
            ok &= require(center.hill_relief == center.mountain_relief, "zero mountain strength keeps authored land on hill relief path");
        } else {
            ok &= require(center.elevation < settings.sea_level, "negative constraint center becomes water below sea level");
        }
        center_elevations[tool_index++] = center.elevation;
    }

    ok &= require(center_elevations[0] < center_elevations[1], "ocean center is deeper than shallow-water center");
    ok &= require(center_elevations[1] < settings.sea_level, "shallow-water center remains below sea level");
    ok &= require(center_elevations[2] > settings.sea_level, "valley center remains above sea level");
    ok &= require(center_elevations[2] < center_elevations[3], "mountain center is higher than valley center through signed relief semantics");
    return ok;
}

bool test_authored_mountain_does_not_bypass_relief_pipeline() {
    const auto settings = one_stage_constraint_test_settings();
    const auto baseline = procgen::generate_greater_realm(settings);

    procgen::TerrainConstraintField constraints(65, 65);
    constraints.paint(procgen::TerrainConstraintTool::Mountain, 0.5f, 0.5f, 0.20f);
    const auto edited = procgen::generate_greater_realm(settings, constraints);
    const auto center_index = edited.index(settings.width / 2, settings.height / 2);
    const auto& center = edited.cells[center_index];

    bool distant_elevations_match = true;
    for (std::uint32_t y = 0; y < edited.height; ++y) {
        for (std::uint32_t x = 0; x < edited.width; ++x) {
            if (normalized_distance_from_center(edited, x, y) <= 0.24f) {
                continue;
            }
            const auto index = edited.index(x, y);
            if (edited.cells[index].landmass_elevation != baseline.cells[index].landmass_elevation
                || edited.cells[index].elevation != baseline.cells[index].elevation) {
                distant_elevations_match = false;
                break;
            }
        }
        if (!distant_elevations_match) {
            break;
        }
    }

    bool ok = true;
    ok &= require(center.landmass_elevation > 0.95f, "mountain brush strongly raises the signed landmass field");
    ok &= require(center.hill_relief == center.mountain_relief, "zero mountain strength disables the mountain relief target");
    ok &= require(center.elevation < 0.70f, "authored mountain does not directly force final relief to the painted value");
    ok &= require(distant_elevations_match, "authored constraint leaves distant signed and final elevations unchanged");
    return ok;
}

bool test_constraint_strength_response_is_monotonic_and_local() {
    const auto settings = one_stage_constraint_test_settings();
    const auto baseline = procgen::generate_greater_realm(settings);

    procgen::TerrainConstraintField weak_constraints(65, 65);
    weak_constraints.paint(procgen::TerrainConstraintTool::Mountain, 0.5f, 0.5f, 0.20f, 0.35f);
    const auto weak = procgen::generate_greater_realm(settings, weak_constraints);

    procgen::TerrainConstraintField strong_constraints(65, 65);
    strong_constraints.paint(procgen::TerrainConstraintTool::Mountain, 0.5f, 0.5f, 0.20f, 1.0f);
    const auto strong = procgen::generate_greater_realm(settings, strong_constraints);

    const auto center_index = weak.index(settings.width / 2, settings.height / 2);
    const auto shoulder_index = weak.index(settings.width / 2 + 8, settings.height / 2);
    const auto outside_index = weak.index(0, 0);

    bool ok = true;
    ok &= require(strong.cells[center_index].landmass_elevation > weak.cells[center_index].landmass_elevation, "stronger brush increases center signed landmass");
    ok &= require(strong.cells[center_index].elevation > weak.cells[center_index].elevation, "stronger brush increases center final elevation through signed semantics");
    ok &= require(strong.cells[shoulder_index].landmass_elevation > weak.cells[shoulder_index].landmass_elevation, "stronger brush increases shoulder signed landmass");
    ok &= require(weak.cells[outside_index].landmass_elevation == baseline.cells[outside_index].landmass_elevation, "weak brush leaves outside signed field unchanged");
    ok &= require(strong.cells[outside_index].elevation == baseline.cells[outside_index].elevation, "strong brush leaves outside final elevation unchanged");
    return ok;
}

bool test_drainage_is_complete_acyclic_and_downhill() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 271828;
    settings.width = 96;
    settings.height = 72;
    const auto map = procgen::generate_greater_realm(settings);

    std::vector<bool> seen(map.cells.size(), false);
    bool valid = map.drainage_order.size() == map.cells.size();
    for (const std::uint32_t index : map.drainage_order) {
        valid &= index < map.cells.size() && !seen[index];
        if (index < seen.size()) {
            seen[index] = true;
        }
    }

    for (std::uint32_t index = 0; index < map.cells.size() && valid; ++index) {
        const auto& cell = map.cells[index];
        if (cell.is_drainage_outlet) {
            valid &= cell.downslope_index == procgen::INVALID_CELL_INDEX;
            continue;
        }

        valid &= cell.downslope_index < map.cells.size();
        if (!valid) {
            break;
        }
        const auto& downstream = map.cells[cell.downslope_index];
        valid &= std::abs(cell.x - downstream.x) <= 1 && std::abs(cell.y - downstream.y) <= 1;
        valid &= downstream.drainage_elevation <= cell.drainage_elevation;

        std::uint32_t cursor = index;
        std::size_t steps = 0;
        while (!map.cells[cursor].is_drainage_outlet && steps <= map.cells.size()) {
            cursor = map.cells[cursor].downslope_index;
            ++steps;
        }
        valid &= steps <= map.cells.size() && map.cells[cursor].is_drainage_outlet;
    }

    return require(valid, "priority drainage exports unique, adjacent, acyclic downhill paths to outlets");
}

bool test_catchment_area_accumulates_without_weather() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 161803;
    settings.width = 96;
    settings.height = 72;
    settings.cell_size = 2.0f;
    const auto map = procgen::generate_greater_realm(settings);
    const float cell_area = settings.cell_size * settings.cell_size;

    bool valid = true;
    float maximum_area = 0.0f;
    for (const auto& cell : map.cells) {
        valid &= cell.drainage_area >= 0.0f;
        if (!cell.is_water) {
            valid &= cell.drainage_area >= cell_area;
        }
        maximum_area = std::max(maximum_area, cell.drainage_area);
        if (cell.downslope_index != procgen::INVALID_CELL_INDEX) {
            valid &= map.cells[cell.downslope_index].drainage_area >= cell.drainage_area;
        }
    }

    bool ok = true;
    ok &= require(valid, "terrain area accumulates monotonically along drainage paths");
    ok &= require(maximum_area > cell_area, "catchments combine contributions from multiple cells");
    return ok;
}

bool test_river_accumulation_connectivity_and_thresholding() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 141421;
    settings.width = 128;
    settings.height = 96;

    settings.river_min_drainage_area = 80.0f;
    const auto previous_default = procgen::generate_greater_realm(settings);
    settings.river_min_drainage_area = procgen::GreaterRealmGeneratorSettings{}.river_min_drainage_area;
    const auto tuned_default = procgen::generate_greater_realm(settings);

    settings.river_min_drainage_area = 1.0f;
    const auto rivers = procgen::generate_greater_realm(settings);
    const float visible_river_minimum = settings.river_min_drainage_area;

    settings.river_min_drainage_area = 1000000.0f;
    const auto hidden = procgen::generate_greater_realm(settings);

    bool valid = !rivers.rivers.empty();
    for (const auto& river : rivers.rivers) {
        valid &= river.source_index < rivers.cells.size();
        valid &= river.destination_index < rivers.cells.size();
        if (!valid) {
            break;
        }
        const auto& source = rivers.cells[river.source_index];
        const auto& destination = rivers.cells[river.destination_index];
        valid &= source.downslope_index == river.destination_index;
        valid &= !source.is_coastal;
        valid &= !destination.is_coastal;
        if (source.distance_to_coast <= 3.0f) {
            valid &= destination.distance_to_coast < source.distance_to_coast;
        }
        valid &= has_upstream_river_candidate(rivers, river.source_index, visible_river_minimum);
        valid &= destination.drainage_area >= source.drainage_area;
        valid &= destination.drainage_elevation <= source.drainage_elevation;
        valid &= river.drainage_area == source.drainage_area && river.width >= 1.0f;
    }

    bool ok = true;
    ok &= require(tuned_default.rivers.size() * 4 < previous_default.rivers.size(), "retuned catchment threshold substantially reduces exported channel density");
    ok &= require(!tuned_default.rivers.empty(), "retuned default preserves a visible potential river network");
    ok &= require(topology_matches(previous_default, tuned_default), "default channel tuning does not alter generated terrain");
    ok &= require(previous_default.drainage_order == tuned_default.drainage_order, "default channel tuning does not alter drainage topology");
    ok &= require(valid, "potential river channels follow connected accumulated downhill drainage");
    ok &= require(hidden.rivers.empty(), "catchment threshold can suppress all exported channels");
    ok &= require(topology_matches(rivers, hidden), "channel visualization parameters do not alter generated terrain");
    ok &= require(rivers.drainage_order == hidden.drainage_order, "channel threshold does not alter drainage topology");
    return ok;
}

bool test_default_rivers_do_not_export_coastal_land_segments() {
    procgen::GreaterRealmGeneratorSettings settings;
    const auto map = procgen::generate_greater_realm(settings);

    bool valid = true;
    for (const auto& river : map.rivers) {
        valid &= river.source_index < map.cells.size();
        valid &= river.destination_index < map.cells.size();
        if (!valid) {
            break;
        }
        valid &= !map.cells[river.source_index].is_coastal;
        valid &= !map.cells[river.destination_index].is_coastal;
        if (map.cells[river.source_index].distance_to_coast <= 3.0f) {
            valid &= map.cells[river.destination_index].distance_to_coast
                < map.cells[river.source_index].distance_to_coast;
        }
        valid &= has_upstream_river_candidate(
            map,
            river.source_index,
            settings.river_min_drainage_area
        );
    }

    return require(valid, "default river overlay does not export coastal specks or isolated starts");
}

bool test_rivers_reach_debug_visualization() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 173205;
    settings.width = 96;
    settings.height = 72;
    settings.river_min_drainage_area = 1.0f;
    const auto map = procgen::generate_greater_realm(settings);
    procgen::GreaterRealmDebugOptions options;
    options.show_coastline = false;
    options.show_mountain_peaks = false;
    options.show_drainage_directions = false;
    const auto image = procgen::build_greater_realm_debug_image(map, settings.sea_level, options);

    bool ok = true;
    ok &= require(!map.rivers.empty(), "river visualization test map exports river segments");
    ok &= require(image.has_expected_byte_count(), "river debug image has valid storage");
    if (!map.rivers.empty() && image.has_expected_byte_count()) {
        const std::size_t pixel = static_cast<std::size_t>(map.rivers.front().source_index) * 4;
        ok &= require(
            image.rgba[pixel] == 40 && image.rgba[pixel + 1] == 156 && image.rgba[pixel + 2] == 224,
            "debug image overlays exported rivers"
        );
    }
    return ok;
}

} // namespace

int main() {
    const std::array tests{
        test_constraint_tools_sampling_and_serialization,
        test_constraints_have_local_deterministic_influence,
        test_constraint_tools_route_through_signed_field_once,
        test_authored_mountain_does_not_bypass_relief_pipeline,
        test_constraint_strength_response_is_monotonic_and_local,
        test_drainage_is_complete_acyclic_and_downhill,
        test_catchment_area_accumulates_without_weather,
        test_river_accumulation_connectivity_and_thresholding,
        test_default_rivers_do_not_export_coastal_land_segments,
        test_rivers_reach_debug_visualization
    };

    bool ok = true;
    for (const auto& test : tests) {
        ok &= test();
    }
    if (!ok) {
        return 1;
    }

    std::cout << "Hydrology and constraint tests passed.\n";
    return 0;
}
