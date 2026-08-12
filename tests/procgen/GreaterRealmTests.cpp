#include "procgen/GreaterRealm.hpp"
#include "procgen/GreaterRealmDebug.hpp"
#include <array>
#include <cmath>
#include <cstddef>
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

bool maps_match(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    if (a.seed != b.seed || a.width != b.width || a.height != b.height || a.cells.size() != b.cells.size()) {
        return false;
    }

    for (std::size_t i = 0; i < a.cells.size(); ++i) {
        const auto& left = a.cells[i];
        const auto& right = b.cells[i];
        if (left.x != right.x
            || left.y != right.y
            || left.landmass_elevation != right.landmass_elevation
            || left.elevation != right.elevation
            || left.is_water != right.is_water
            || left.is_ocean != right.is_ocean
            || left.is_coastal != right.is_coastal
            || left.distance_to_coast != right.distance_to_coast
            || left.slope != right.slope
            || left.terrain_form != right.terrain_form) {
            return false;
        }
    }

    return true;
}

float elevation_difference_sum(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    const std::size_t count = std::min(a.cells.size(), b.cells.size());
    float total = 0.0f;

    for (std::size_t i = 0; i < count; ++i) {
        total += std::abs(a.cells[i].elevation - b.cells[i].elevation);
    }

    return total;
}

std::size_t count_land(const procgen::GreaterRealmMap& map) {
    std::size_t count = 0;
    for (const auto& cell : map.cells) {
        if (!cell.is_water) {
            ++count;
        }
    }
    return count;
}

bool boundaries_are_water(const procgen::GreaterRealmMap& map) {
    for (std::uint32_t x = 0; x < map.width; ++x) {
        if (!map.cells[map.index(x, 0)].is_water
            || !map.cells[map.index(x, map.height - 1)].is_water) {
            return false;
        }
    }

    for (std::uint32_t y = 0; y < map.height; ++y) {
        if (!map.cells[map.index(0, y)].is_water
            || !map.cells[map.index(map.width - 1, y)].is_water) {
            return false;
        }
    }

    return true;
}

std::size_t topology_difference_count(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    const std::size_t count = std::min(a.cells.size(), b.cells.size());
    std::size_t differences = 0;

    for (std::size_t i = 0; i < count; ++i) {
        if (a.cells[i].is_water != b.cells[i].is_water) {
            ++differences;
        }
    }

    return differences;
}

bool topology_matches(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    if (a.width != b.width || a.height != b.height || a.cells.size() != b.cells.size()) {
        return false;
    }

    for (std::size_t i = 0; i < a.cells.size(); ++i) {
        if (a.cells[i].landmass_elevation != b.cells[i].landmass_elevation
            || a.cells[i].is_water != b.cells[i].is_water
            || a.cells[i].is_ocean != b.cells[i].is_ocean) {
            return false;
        }
    }

    return true;
}

bool ocean_flags_match_boundary_connectivity(const procgen::GreaterRealmMap& map) {
    std::vector<bool> expected(map.cells.size(), false);
    std::vector<std::size_t> open;

    const auto enqueue = [&](std::uint32_t x, std::uint32_t y) {
        const std::size_t index = map.index(x, y);
        if (!map.cells[index].is_water || expected[index]) {
            return;
        }

        expected[index] = true;
        open.push_back(index);
    };

    for (std::uint32_t x = 0; x < map.width; ++x) {
        enqueue(x, 0);
        enqueue(x, map.height - 1);
    }
    for (std::uint32_t y = 0; y < map.height; ++y) {
        enqueue(0, y);
        enqueue(map.width - 1, y);
    }

    constexpr std::array<std::array<std::int32_t, 2>, 4> neighbors{{
        {{-1, 0}},
        {{1, 0}},
        {{0, -1}},
        {{0, 1}}
    }};

    while (!open.empty()) {
        const std::size_t index = open.back();
        open.pop_back();

        const auto& cell = map.cells[index];
        for (const auto& offset : neighbors) {
            const std::int32_t x = cell.x + offset[0];
            const std::int32_t y = cell.y + offset[1];
            if (map.contains(x, y)) {
                enqueue(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
            }
        }
    }

    for (std::size_t i = 0; i < map.cells.size(); ++i) {
        if (map.cells[i].is_ocean != expected[i]) {
            return false;
        }
    }
    return true;
}

bool coastal_flags_match_land_water_boundary(const procgen::GreaterRealmMap& map) {
    for (const auto& cell : map.cells) {
        bool touches_water = false;
        for (std::int32_t y = cell.y - 1; y <= cell.y + 1 && !touches_water; ++y) {
            for (std::int32_t x = cell.x - 1; x <= cell.x + 1; ++x) {
                if (x == cell.x && y == cell.y) {
                    continue;
                }

                const auto* neighbor = map.cell(x, y);
                if (neighbor && neighbor->is_water) {
                    touches_water = true;
                    break;
                }
            }
        }

        if (cell.is_coastal != (!cell.is_water && touches_water)) {
            return false;
        }
    }
    return true;
}

bool test_generated_map_shape() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 12345;
    settings.width = 128;
    settings.height = 96;

    const auto map = procgen::generate_greater_realm(settings);
    const auto counts = procgen::count_terrain_forms(map);
    const std::size_t land_count = map.cells.size() - counts.ocean;

    bool ok = true;
    ok &= require(map.seed == settings.seed, "map stores the requested seed");
    ok &= require(map.width == settings.width, "map stores the requested width");
    ok &= require(map.height == settings.height, "map stores the requested height");
    ok &= require(map.has_expected_cell_count(), "map contains width * height cells");
    ok &= require(counts.ocean > 0, "map contains ocean");
    ok &= require(land_count > 0, "map contains land");
    ok &= require(counts.coastal_land > 0, "map contains coastal land");
    ok &= require(counts.hills + counts.highlands + counts.mountains > 0, "map contains raised terrain forms");

    for (const auto& cell : map.cells) {
        ok &= require(cell.landmass_elevation >= -1.0f && cell.landmass_elevation <= 1.0f, "landmass elevation is signed and normalized");
        ok &= require(cell.elevation >= 0.0f && cell.elevation <= 1.0f, "cell elevation is normalized");
        ok &= require(cell.slope >= 0.0f, "cell slope is non-negative");
        ok &= require(cell.distance_to_coast >= 0.0f, "cell coast distance is non-negative");
        ok &= require(!cell.is_ocean || cell.is_water, "ocean cells are water");
        ok &= require(!cell.is_coastal || !cell.is_water, "coastal metadata applies only to land");
        ok &= require(!cell.is_coastal || cell.distance_to_coast == 0.0f, "coastal land lies on the land-water boundary");

        if (cell.is_water) {
            ok &= require(cell.landmass_elevation <= 0.0f, "water has a non-positive landmass constraint");
            ok &= require(cell.elevation <= settings.sea_level, "water elevation does not exceed sea level");
        } else {
            ok &= require(cell.landmass_elevation > 0.0f, "land has a positive landmass constraint");
            ok &= require(cell.elevation > settings.sea_level, "land elevation exceeds sea level");
        }
    }

    ok &= require(ocean_flags_match_boundary_connectivity(map), "ocean flags match boundary-connected water");
    ok &= require(coastal_flags_match_land_water_boundary(map), "coastal flags match land touching water");

    return ok;
}

bool test_inland_relief_preserves_landmass_topology() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 24680;
    settings.width = 96;
    settings.height = 72;

    const auto baseline = procgen::generate_greater_realm(settings);

    settings.base_elevation_weight = 0.2f;
    settings.mountain_weight = 1.4f;
    settings.ridge_weight = 1.2f;
    settings.valley_weight = 0.8f;
    settings.terrain_noise_weight = 0.6f;
    const auto changed_relief = procgen::generate_greater_realm(settings);

    bool ok = true;
    ok &= require(topology_matches(baseline, changed_relief), "inland relief weights do not change land or ocean topology");
    ok &= require(elevation_difference_sum(baseline, changed_relief) > 1.0f, "inland relief weights still change terrain elevation");
    return ok;
}

bool test_terrain_noise_changes_land_relief_only() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 11223;
    settings.width = 96;
    settings.height = 72;
    settings.terrain_noise_weight = 0.0f;
    const auto smooth = procgen::generate_greater_realm(settings);

    settings.terrain_noise_weight = 1.5f;
    const auto noisy = procgen::generate_greater_realm(settings);

    float land_elevation_difference = 0.0f;
    bool water_elevation_unchanged = true;
    for (std::size_t i = 0; i < smooth.cells.size(); ++i) {
        if (smooth.cells[i].is_water) {
            if (smooth.cells[i].elevation != noisy.cells[i].elevation) {
                water_elevation_unchanged = false;
            }
        } else {
            land_elevation_difference += std::abs(smooth.cells[i].elevation - noisy.cells[i].elevation);
        }
    }

    bool ok = true;
    ok &= require(topology_matches(smooth, noisy), "terrain noise does not change land or ocean topology");
    ok &= require(water_elevation_unchanged, "terrain noise does not change water elevation");
    ok &= require(land_elevation_difference > 1.0f, "terrain noise changes land elevation");
    return ok;
}

bool test_sea_level_controls_landmass_topology() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 13579;
    settings.width = 96;
    settings.height = 72;
    settings.sea_level = 0.35f;
    const auto low_sea = procgen::generate_greater_realm(settings);

    settings.sea_level = 0.65f;
    const auto high_sea = procgen::generate_greater_realm(settings);

    return require(count_land(low_sea) > count_land(high_sea), "higher sea level reduces generated land area");
}

bool test_ocean_depth_preserves_topology() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 97531;
    settings.width = 96;
    settings.height = 72;
    settings.ocean_depth_weight = 0.1f;
    const auto shallow = procgen::generate_greater_realm(settings);

    settings.ocean_depth_weight = 2.0f;
    const auto deep = procgen::generate_greater_realm(settings);

    float water_elevation_difference = 0.0f;
    bool land_elevation_unchanged = true;
    for (std::size_t i = 0; i < shallow.cells.size(); ++i) {
        if (shallow.cells[i].is_water) {
            water_elevation_difference += std::abs(shallow.cells[i].elevation - deep.cells[i].elevation);
        } else if (shallow.cells[i].elevation != deep.cells[i].elevation) {
            land_elevation_unchanged = false;
        }
    }

    bool ok = true;
    ok &= require(topology_matches(shallow, deep), "ocean depth does not change land or ocean topology");
    ok &= require(land_elevation_unchanged, "ocean depth does not change land elevation");
    ok &= require(water_elevation_difference > 1.0f, "ocean depth changes water elevation");
    return ok;
}

bool test_island_bias_matches_mapgen4_contract() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 86420;
    settings.width = 128;
    settings.height = 80;
    settings.coastline_noise_weight = 0.0f;

    bool ok = true;
    ok &= require(settings.island_bias == 0.5f, "island bias defaults to Mapgen4's 0.5");

    settings.island_bias = 0.0f;
    const auto unbiased = procgen::generate_greater_realm(settings);

    settings.island_bias = 1.0f;
    const auto island = procgen::generate_greater_realm(settings);

    ok &= require(topology_difference_count(unbiased, island) > 0, "island bias changes landmass topology");
    ok &= require(boundaries_are_water(island), "strong island bias keeps rectangular map boundaries underwater");

    settings.island_bias = -5.0f;
    const auto below_range = procgen::generate_greater_realm(settings);
    settings.island_bias = 5.0f;
    const auto above_range = procgen::generate_greater_realm(settings);

    ok &= require(maps_match(unbiased, below_range), "island bias clamps values below Mapgen4's zero minimum");
    ok &= require(maps_match(island, above_range), "island bias clamps values above Mapgen4's one maximum");
    ok &= require(elevation_difference_sum(unbiased, island) > 1.0f, "island bias preserves signed elevation influence before ocean depth");
    return ok;
}

bool test_seed_determinism() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 67890;
    settings.width = 64;
    settings.height = 64;

    const auto first = procgen::generate_greater_realm(settings);
    const auto second = procgen::generate_greater_realm(settings);

    bool ok = true;
    ok &= require(maps_match(first, second), "same seed and settings produce identical maps");

    settings.seed = 67891;
    const auto different = procgen::generate_greater_realm(settings);
    ok &= require(elevation_difference_sum(first, different) > 1.0f, "different seeds produce different elevation fields");

    return ok;
}

bool test_map_cell_lookup() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 42;
    settings.width = 16;
    settings.height = 12;

    auto map = procgen::generate_greater_realm(settings);

    bool ok = true;
    ok &= require(map.cell(0, 0) != nullptr, "cell lookup finds origin");
    ok &= require(map.cell(15, 11) != nullptr, "cell lookup finds max in-bounds coordinate");
    ok &= require(map.cell(-1, 0) == nullptr, "cell lookup rejects negative x");
    ok &= require(map.cell(0, -1) == nullptr, "cell lookup rejects negative y");
    ok &= require(map.cell(16, 0) == nullptr, "cell lookup rejects x past width");
    ok &= require(map.cell(0, 12) == nullptr, "cell lookup rejects y past height");

    return ok;
}

bool test_coastal_land_preserves_elevation_form() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 556677;
    settings.width = 96;
    settings.height = 72;
    settings.mountain_threshold = settings.sea_level;

    const auto map = procgen::generate_greater_realm(settings);
    std::size_t coastal_count = 0;
    bool all_coastal_land_is_mountain = true;
    for (const auto& cell : map.cells) {
        if (cell.is_coastal) {
            ++coastal_count;
            all_coastal_land_is_mountain &= cell.terrain_form == procgen::TerrainForm::Mountains;
        }
    }

    bool ok = true;
    ok &= require(coastal_count > 0, "threshold test map contains coastal land");
    ok &= require(all_coastal_land_is_mountain, "coastal metadata does not override elevation terrain form");
    return ok;
}

bool test_debug_visualization_data() {
    procgen::GreaterRealmMap map;
    map.width = 3;
    map.height = 2;
    map.cells.resize(6);

    constexpr std::array forms{
        procgen::TerrainForm::Ocean,
        procgen::TerrainForm::Plains,
        procgen::TerrainForm::Plains,
        procgen::TerrainForm::Hills,
        procgen::TerrainForm::Highlands,
        procgen::TerrainForm::Mountains
    };
    constexpr std::array elevations{0.1f, 0.58f, 0.58f, 0.65f, 0.75f, 0.95f};

    for (std::size_t index = 0; index < map.cells.size(); ++index) {
        map.cells[index].terrain_form = forms[index];
        map.cells[index].elevation = elevations[index];
    }
    map.cells[1].is_coastal = true;
    map.cells[5].is_coastal = true;

    const auto counts = procgen::count_terrain_forms(map);
    const auto image = procgen::build_greater_realm_debug_image(map, 0.5f);

    bool ok = true;
    ok &= require(counts.ocean == 1, "debug terrain counts include ocean cells");
    ok &= require(counts.coastal_land == 2, "debug terrain counts include coastal land independently");
    ok &= require(counts.plains == 2, "debug terrain counts preserve coastal plains");
    ok &= require(counts.hills == 1, "debug terrain counts include hills cells");
    ok &= require(counts.highlands == 1, "debug terrain counts include highland cells");
    ok &= require(counts.mountains == 1, "debug terrain counts include mountain cells");
    ok &= require(image.width == map.width && image.height == map.height, "debug image preserves map dimensions");
    ok &= require(image.has_expected_byte_count(), "debug image contains one RGBA pixel per map cell");
    const auto coastal_plain = procgen::greater_realm_debug_colour(map.cells[1], 0.5f);
    const auto coastal_mountain = procgen::greater_realm_debug_colour(map.cells[5], 0.5f);
    ok &= require(
        image.rgba[4] < coastal_plain.r && image.rgba[5] < coastal_plain.g && image.rgba[6] < coastal_plain.b,
        "debug image darkens coastal land to form a narrow outline"
    );
    ok &= require(
        image.rgba[4] != image.rgba[20] || image.rgba[5] != image.rgba[21] || image.rgba[6] != image.rgba[22],
        "coastal plains and mountains retain distinct debug colours"
    );
    ok &= require(
        image.rgba[20] < coastal_mountain.r && image.rgba[21] < coastal_mountain.g && image.rgba[22] < coastal_mountain.b,
        "coastline outline preserves the mountain palette beneath it"
    );
    for (std::size_t alpha = 3; alpha < image.rgba.size(); alpha += 4) {
        ok &= require(image.rgba[alpha] == 255, "debug image pixels are opaque");
    }
    return ok;
}

bool test_debug_ocean_depth_shading() {
    procgen::GreaterRealmCell deep;
    deep.terrain_form = procgen::TerrainForm::Ocean;
    deep.elevation = 0.0f;

    procgen::GreaterRealmCell shallow = deep;
    shallow.elevation = 0.49f;

    const auto deep_colour = procgen::greater_realm_debug_colour(deep, 0.5f);
    const auto shallow_colour = procgen::greater_realm_debug_colour(shallow, 0.5f);

    bool ok = true;
    ok &= require(deep_colour.b < shallow_colour.b, "deeper ocean cells render darker than shallow cells");
    ok &= require(deep_colour.a == 255 && shallow_colour.a == 255, "ocean debug colours are opaque");
    return ok;
}

bool test_debug_visualization_rejects_malformed_map() {
    procgen::GreaterRealmMap map;
    map.width = 2;
    map.height = 2;
    map.cells.resize(3);

    const auto image = procgen::build_greater_realm_debug_image(map, 0.5f);
    return require(!image.has_expected_byte_count(), "debug visualization rejects malformed map storage");
}

} // namespace

int main() {
    const std::array tests = {
        test_generated_map_shape,
        test_seed_determinism,
        test_map_cell_lookup,
        test_coastal_land_preserves_elevation_form,
        test_inland_relief_preserves_landmass_topology,
        test_terrain_noise_changes_land_relief_only,
        test_sea_level_controls_landmass_topology,
        test_ocean_depth_preserves_topology,
        test_island_bias_matches_mapgen4_contract,
        test_debug_visualization_data,
        test_debug_ocean_depth_shading,
        test_debug_visualization_rejects_malformed_map
    };

    bool ok = true;
    for (const auto& test : tests) {
        ok &= test();
    }

    if (!ok) {
        return 1;
    }

    std::cout << "Greater realm procgen tests passed.\n";
    return 0;
}
