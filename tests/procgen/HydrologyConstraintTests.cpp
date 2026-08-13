#if !defined(RFD_TEST_BUILD)
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

    settings.river_min_drainage_area = 60.0f;
    const auto previous_default = procgen::generate_greater_realm(settings);
    settings.river_min_drainage_area = 80.0f;
    const auto tuned_default = procgen::generate_greater_realm(settings);

    settings.river_min_drainage_area = 1.0f;
    const auto rivers = procgen::generate_greater_realm(settings);

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
        valid &= destination.drainage_area >= source.drainage_area;
        valid &= destination.drainage_elevation <= source.drainage_elevation;
        valid &= river.drainage_area == source.drainage_area && river.width >= 1.0f;
    }

    bool ok = true;
    ok &= require(tuned_default.rivers.size() < previous_default.rivers.size(), "higher default catchment threshold reduces exported channel density");
    ok &= require(topology_matches(previous_default, tuned_default), "default channel tuning does not alter generated terrain");
    ok &= require(previous_default.drainage_order == tuned_default.drainage_order, "default channel tuning does not alter drainage topology");
    ok &= require(valid, "potential river channels follow connected accumulated downhill drainage");
    ok &= require(hidden.rivers.empty(), "catchment threshold can suppress all exported channels");
    ok &= require(topology_matches(rivers, hidden), "channel visualization parameters do not alter generated terrain");
    ok &= require(rivers.drainage_order == hidden.drainage_order, "channel threshold does not alter drainage topology");
    return ok;
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
        test_drainage_is_complete_acyclic_and_downhill,
        test_catchment_area_accumulates_without_weather,
        test_river_accumulation_connectivity_and_thresholding,
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
