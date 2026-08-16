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
            || left.relief_constraint != right.relief_constraint
            || left.hill_relief != right.hill_relief
            || left.mountain_relief != right.mountain_relief
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

float land_elevation_difference_sum(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    const std::size_t count = std::min(a.cells.size(), b.cells.size());
    float total = 0.0f;

    for (std::size_t i = 0; i < count; ++i) {
        if (!a.cells[i].is_water) {
            total += std::abs(a.cells[i].elevation - b.cells[i].elevation);
        }
    }

    return total;
}

bool water_elevations_match(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    const std::size_t count = std::min(a.cells.size(), b.cells.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (a.cells[i].is_water && a.cells[i].elevation != b.cells[i].elevation) {
            return false;
        }
    }
    return true;
}

bool land_elevation_monotonic(
    const procgen::GreaterRealmMap& lower,
    const procgen::GreaterRealmMap& upper,
    bool expect_non_decreasing
) {
    const std::size_t count = std::min(lower.cells.size(), upper.cells.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (lower.cells[i].is_water) {
            continue;
        }
        if (expect_non_decreasing && upper.cells[i].elevation < lower.cells[i].elevation) {
            return false;
        }
        if (!expect_non_decreasing && upper.cells[i].elevation > lower.cells[i].elevation) {
            return false;
        }
    }
    return true;
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

bool landmass_fields_match(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    if (a.width != b.width || a.height != b.height || a.cells.size() != b.cells.size()) {
        return false;
    }

    for (std::size_t i = 0; i < a.cells.size(); ++i) {
        if (a.cells[i].landmass_elevation != b.cells[i].landmass_elevation
            || a.cells[i].is_water != b.cells[i].is_water) {
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
            ok &= require(cell.elevation <= procgen::NORMALIZED_WATERLINE, "water elevation does not exceed the fixed waterline");
        } else {
            ok &= require(cell.landmass_elevation > 0.0f, "land has a positive landmass constraint");
            ok &= require(cell.elevation > procgen::NORMALIZED_WATERLINE, "land elevation exceeds the fixed waterline");
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

bool test_mountain_strength_changes_only_local_mountain_relief() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 515151;
    settings.width = 128;
    settings.height = 96;
    settings.mountain_peak_radius = 14.0f;
    settings.mountain_weight = 0.0f;
    const auto without_mountains = procgen::generate_greater_realm(settings);

    settings.mountain_weight = 1.0f;
    const auto strong_mountains = procgen::generate_greater_realm(settings);

    bool water_unchanged = true;
    bool uninfluenced_land_unchanged = true;
    bool land_never_lowered = true;
    std::size_t influenced_land_count = 0;
    std::size_t uninfluenced_land_count = 0;
    float influenced_elevation_gain = 0.0f;
    for (std::size_t index = 0; index < without_mountains.cells.size(); ++index) {
        const auto& baseline = without_mountains.cells[index];
        const auto& stronger = strong_mountains.cells[index];
        if (baseline.is_water) {
            water_unchanged &= baseline.elevation == stronger.elevation;
            continue;
        }

        land_never_lowered &= stronger.elevation >= baseline.elevation;
        if (baseline.mountain_influence == 0.0f) {
            ++uninfluenced_land_count;
            uninfluenced_land_unchanged &= baseline.elevation == stronger.elevation;
        } else {
            ++influenced_land_count;
            influenced_elevation_gain += stronger.elevation - baseline.elevation;
        }
    }

    bool ok = true;
    ok &= require(topology_matches(without_mountains, strong_mountains), "mountain strength preserves land and ocean topology");
    ok &= require(water_unchanged, "mountain strength does not change water elevation");
    ok &= require(influenced_land_count > 0, "test map contains mountain-influenced land");
    ok &= require(uninfluenced_land_count > 0, "test map contains land outside mountain influence");
    ok &= require(uninfluenced_land_unchanged, "mountain strength does not shift land outside mountain influence");
    ok &= require(land_never_lowered, "stronger mountains never lower land elevation");
    return ok && require(influenced_elevation_gain > 1.0f, "mountain strength raises localized influenced terrain");
}

bool test_land_relief_stages_are_inspectable_and_separated() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 314159;
    settings.width = 128;
    settings.height = 96;
    const auto map = procgen::generate_greater_realm(settings);

    std::size_t land_count = 0;
    std::size_t peak_like_count = 0;
    std::size_t hill_stage_count = 0;
    float peak_relief_sum = 0.0f;
    float hill_relief_sum = 0.0f;
    bool stages_valid = true;

    for (const auto& cell : map.cells) {
        if (cell.is_water) {
            stages_valid &= cell.hill_relief == 0.0f && cell.mountain_relief == 0.0f;
            continue;
        }

        ++land_count;
        stages_valid &= cell.hill_relief >= 0.0f && cell.hill_relief <= 1.0f;
        stages_valid &= cell.mountain_relief >= 0.0f && cell.mountain_relief <= 1.0f;
        stages_valid &= cell.mountain_relief >= cell.hill_relief;
        ++hill_stage_count;
        hill_relief_sum += cell.hill_relief;
        if (cell.mountain_influence > 0.90f && cell.landmass_elevation > 0.35f) {
            ++peak_like_count;
            peak_relief_sum += cell.mountain_relief;
        }
    }

    const float average_peak_relief = peak_like_count > 0 ? peak_relief_sum / static_cast<float>(peak_like_count) : 0.0f;
    const float average_hill_relief = hill_stage_count > 0 ? hill_relief_sum / static_cast<float>(hill_stage_count) : 0.0f;

    bool ok = true;
    ok &= require(land_count > 0, "relief-stage test map contains land");
    ok &= require(stages_valid, "hill and mountain relief stages are valid normalized cell data");
    ok &= require(peak_like_count > 0, "test map contains peak-distance mountain cells");
    ok &= require(average_peak_relief > average_hill_relief + 0.20f, "mountain target is visibly separated from low hill relief");
    return ok;
}

bool test_relief_control_ranges_preserve_topology_and_monotonicity() {
    constexpr std::array seeds{314159ull, 515151ull, 616161ull};
    bool ok = require(
        procgen::GreaterRealmGeneratorSettings{}.coastline_noise_weight == 0.01f,
        "coastline detail defaults to Mapgen4's 0.01 strength"
    );

    for (const auto seed : seeds) {
        procgen::GreaterRealmGeneratorSettings settings;
        settings.seed = seed;
        settings.width = 96;
        settings.height = 72;

        auto lower = settings;
        auto upper = settings;

        lower.base_elevation_weight = 0.0f;
        upper.base_elevation_weight = 2.0f;
        const auto low_base = procgen::generate_greater_realm(lower);
        const auto default_base = procgen::generate_greater_realm(settings);
        const auto high_base = procgen::generate_greater_realm(upper);
        ok &= require(topology_matches(low_base, default_base) && topology_matches(default_base, high_base), "base relief does not change topology");
        ok &= require(water_elevations_match(low_base, high_base), "base relief does not change water elevation");
        ok &= require(land_elevation_monotonic(low_base, default_base, true) && land_elevation_monotonic(default_base, high_base, true), "base relief is monotonic across lower/default/upper settings");
        ok &= require(land_elevation_difference_sum(low_base, high_base) > 5.0f, "base relief has visible land significance");

        lower = settings;
        upper = settings;
        lower.ridge_weight = 0.0f;
        upper.ridge_weight = 1.5f;
        const auto low_ridge = procgen::generate_greater_realm(lower);
        const auto default_ridge = procgen::generate_greater_realm(settings);
        const auto high_ridge = procgen::generate_greater_realm(upper);
        ok &= require(topology_matches(low_ridge, high_ridge), "ridge relief does not change topology");
        ok &= require(water_elevations_match(low_ridge, high_ridge), "ridge relief does not change water elevation");
        ok &= require(land_elevation_monotonic(low_ridge, default_ridge, true) && land_elevation_monotonic(default_ridge, high_ridge, true), "ridge relief is monotonic across lower/default/upper settings");
        ok &= require(land_elevation_difference_sum(low_ridge, high_ridge) > 1.0f, "ridge relief has visible land significance");

        lower = settings;
        upper = settings;
        lower.valley_weight = 0.0f;
        upper.valley_weight = 1.5f;
        const auto low_valley = procgen::generate_greater_realm(lower);
        const auto default_valley = procgen::generate_greater_realm(settings);
        const auto high_valley = procgen::generate_greater_realm(upper);
        ok &= require(topology_matches(low_valley, high_valley), "valley relief does not change topology");
        ok &= require(water_elevations_match(low_valley, high_valley), "valley relief does not change water elevation");
        ok &= require(land_elevation_monotonic(low_valley, default_valley, false) && land_elevation_monotonic(default_valley, high_valley, false), "valley relief is monotonic across lower/default/upper settings");
        ok &= require(land_elevation_difference_sum(low_valley, high_valley) > 1.0f, "valley relief has visible land significance");
    }

    return ok;
}

bool test_mountain_blend_uses_local_positive_constraint() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 818181;
    settings.width = 128;
    settings.height = 96;
    settings.mountain_weight = 0.0f;
    const auto low = procgen::generate_greater_realm(settings);

    settings.mountain_weight = 1.2f;
    const auto high = procgen::generate_greater_realm(settings);

    std::size_t low_constraint_count = 0;
    std::size_t high_constraint_count = 0;
    float low_constraint_gain = 0.0f;
    float high_constraint_gain = 0.0f;
    bool uninfluenced_unchanged = true;
    bool monotonic = true;

    for (std::size_t i = 0; i < low.cells.size(); ++i) {
        const auto& baseline = low.cells[i];
        const auto& raised = high.cells[i];
        if (baseline.is_water) {
            continue;
        }

        monotonic &= raised.elevation >= baseline.elevation;
        if (baseline.mountain_influence == 0.0f) {
            uninfluenced_unchanged &= raised.elevation == baseline.elevation;
            continue;
        }

        const float gain = raised.elevation - baseline.elevation;
        if (baseline.landmass_elevation < 0.20f) {
            ++low_constraint_count;
            low_constraint_gain += gain;
        } else if (baseline.landmass_elevation > 0.60f) {
            ++high_constraint_count;
            high_constraint_gain += gain;
        }
    }

    const float low_average = low_constraint_count > 0 ? low_constraint_gain / static_cast<float>(low_constraint_count) : 0.0f;
    const float high_average = high_constraint_count > 0 ? high_constraint_gain / static_cast<float>(high_constraint_count) : 0.0f;

    bool ok = true;
    ok &= require(topology_matches(low, high), "mountain blend does not change topology");
    ok &= require(water_elevations_match(low, high), "mountain blend does not change water elevation");
    ok &= require(monotonic, "mountain strength remains monotonic on land");
    ok &= require(uninfluenced_unchanged, "mountain strength remains local to peak-distance influence");
    ok &= require(low_constraint_count > 0 && high_constraint_count > 0, "test map contains low and high positive-constraint mountain cells");
    ok &= require(high_average > low_average * 2.0f, "positive signed constraint locally controls the mountain blend strength");
    return ok;
}

bool test_terrain_noise_range_is_masked_to_land_relief() {
    constexpr std::array seeds{314159ull, 515151ull, 616161ull};
    bool ok = require(
        procgen::GreaterRealmGeneratorSettings{}.coastline_noise_weight == 0.01f,
        "coastline detail defaults to Mapgen4's 0.01 strength"
    );

    for (const auto seed : seeds) {
        procgen::GreaterRealmGeneratorSettings settings;
        settings.seed = seed;
        settings.width = 96;
        settings.height = 72;
        settings.terrain_noise_weight = 0.0f;
        const auto smooth = procgen::generate_greater_realm(settings);

        settings.terrain_noise_weight = 2.0f;
        const auto noisy = procgen::generate_greater_realm(settings);

        std::size_t raised_count = 0;
        std::size_t lowered_count = 0;
        for (std::size_t i = 0; i < smooth.cells.size(); ++i) {
            if (smooth.cells[i].is_water) {
                continue;
            }
            if (noisy.cells[i].elevation > smooth.cells[i].elevation) {
                ++raised_count;
            } else if (noisy.cells[i].elevation < smooth.cells[i].elevation) {
                ++lowered_count;
            }
        }

        ok &= require(topology_matches(smooth, noisy), "terrain noise range does not change topology");
        ok &= require(water_elevations_match(smooth, noisy), "terrain noise range does not change water elevation");
        ok &= require(raised_count > 0 && lowered_count > 0, "terrain noise can raise and lower local land relief");
        ok &= require(land_elevation_difference_sum(smooth, noisy) > 1.0f, "terrain noise range has visible land significance");
    }

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

bool test_coastline_noise_is_local_to_signed_boundary() {
    constexpr std::array seeds{314159ull, 515151ull, 616161ull};
    constexpr float coastline_support = 0.20f;
    bool ok = require(
        procgen::GreaterRealmGeneratorSettings{}.coastline_noise_weight == 0.01f,
        "coastline detail defaults to Mapgen4's 0.01 strength"
    );
    std::size_t topology_changes = 0;

    for (const auto seed : seeds) {
        procgen::GreaterRealmGeneratorSettings settings;
        settings.seed = seed;
        settings.width = 128;
        settings.height = 96;
        settings.coastline_noise_weight = 0.0f;
        const auto unperturbed = procgen::generate_greater_realm(settings);

        settings.coastline_noise_weight = 0.01f;
        const auto default_detail = procgen::generate_greater_realm(settings);

        settings.coastline_noise_weight = 0.10f;
        const auto strong_detail = procgen::generate_greater_realm(settings);
        const auto repeated_strong = procgen::generate_greater_realm(settings);

        std::size_t supported_count = 0;
        std::size_t outside_count = 0;
        float default_delta_sum = 0.0f;
        float strong_delta_sum = 0.0f;
        float strong_max_delta = 0.0f;
        bool relief_constraints_unchanged = true;
        bool outside_signed_unchanged = true;
        bool outside_relief_unchanged = true;
        for (std::size_t i = 0; i < unperturbed.cells.size(); ++i) {
            const auto& base = unperturbed.cells[i];
            const auto& detailed = strong_detail.cells[i];
            const float base_signed = std::abs(base.landmass_elevation);
            const float default_delta = std::abs(
                default_detail.cells[i].landmass_elevation - unperturbed.cells[i].landmass_elevation
            );
            const float strong_delta = std::abs(
                strong_detail.cells[i].landmass_elevation - unperturbed.cells[i].landmass_elevation
            );
            relief_constraints_unchanged &= base.relief_constraint == detailed.relief_constraint;

            if (base_signed < coastline_support) {
                ++supported_count;
                default_delta_sum += default_delta;
                strong_delta_sum += strong_delta;
                strong_max_delta = std::max(strong_max_delta, strong_delta);
                topology_changes += base.is_water != detailed.is_water;
            } else {
                ++outside_count;
                outside_signed_unchanged &= base.landmass_elevation == detailed.landmass_elevation;
                outside_relief_unchanged &= base.elevation == detailed.elevation
                    && base.hill_relief == detailed.hill_relief
                    && base.mountain_relief == detailed.mountain_relief;
            }
        }

        ok &= require(maps_match(strong_detail, repeated_strong), "coastline noise response is deterministic");
        ok &= require(supported_count > 0, "test map contains cells inside coastline-detail support");
        ok &= require(outside_count > 0, "test map contains terrain outside coastline-detail support");
        ok &= require(default_delta_sum > 0.1f, "default coastline detail perturbs the signed coastline field");
        ok &= require(strong_delta_sum > default_delta_sum * 5.0f, "coastline detail retains a substantial control response");
        ok &= require(strong_max_delta <= 0.1751f, "Mapgen4's weighted coastline spectrum still bounds maximum perturbation");
        ok &= require(relief_constraints_unchanged, "coastline detail never changes the clean relief constraint");
        ok &= require(outside_signed_unchanged, "coastline detail leaves the signed field unchanged outside its support");
        ok &= require(outside_relief_unchanged, "coastline detail leaves inland relief unchanged outside its support");
    }

    ok &= require(topology_changes > 0, "strong coastline detail changes coastline topology");
    return ok;
}

bool test_coastline_noise_is_independent_from_relief_and_depth_controls() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 424242;
    settings.width = 96;
    settings.height = 72;
    settings.coastline_noise_weight = 0.0f;
    const auto baseline_without_detail = procgen::generate_greater_realm(settings);

    settings.coastline_noise_weight = 0.05f;
    const auto baseline_with_detail = procgen::generate_greater_realm(settings);

    auto changed_controls = settings;
    changed_controls.base_elevation_weight = 2.0f;
    changed_controls.mountain_weight = 1.3f;
    changed_controls.ridge_weight = 1.5f;
    changed_controls.valley_weight = 1.5f;
    changed_controls.terrain_noise_weight = 2.0f;
    changed_controls.ocean_depth_weight = 3.0f;
    changed_controls.coastline_noise_weight = 0.0f;
    const auto changed_without_detail = procgen::generate_greater_realm(changed_controls);

    changed_controls.coastline_noise_weight = 0.05f;
    const auto changed_with_detail = procgen::generate_greater_realm(changed_controls);

    bool ok = true;
    ok &= require(
        landmass_fields_match(baseline_without_detail, changed_without_detail),
        "inland relief and ocean depth controls do not change the unperturbed signed landmass field"
    );
    ok &= require(
        landmass_fields_match(baseline_with_detail, changed_with_detail),
        "inland relief and ocean depth controls do not change coastline-noise signed perturbation"
    );
    ok &= require(
        topology_matches(baseline_with_detail, changed_with_detail),
        "coastline detail topology remains independent from relief and depth controls"
    );
    ok &= require(
        elevation_difference_sum(baseline_with_detail, changed_with_detail) > 1.0f,
        "relief and depth controls can still change final elevation after coastline topology is fixed"
    );
    return ok;
}

bool test_sea_level_is_not_a_generation_input() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 13579;
    settings.width = 96;
    settings.height = 72;
    settings.sea_level = 0.10f;
    const auto low_setting = procgen::generate_greater_realm(settings);
    const auto low_image = procgen::build_greater_realm_debug_image(low_setting, settings.sea_level);

    settings.sea_level = 0.90f;
    const auto high_setting = procgen::generate_greater_realm(settings);
    const auto high_image = procgen::build_greater_realm_debug_image(high_setting, settings.sea_level);

    bool ok = true;
    ok &= require(maps_match(low_setting, high_setting), "sea level setting no longer changes generated terrain data");
    ok &= require(count_land(low_setting) == count_land(high_setting), "sea level setting no longer changes land area");
    ok &= require(low_image.rgba == high_image.rgba, "sea level setting no longer changes debug terrain shading");
    return ok;
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
    settings.mountain_threshold = procgen::NORMALIZED_WATERLINE;

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

bool test_continuous_terrain_colour_mapping() {
    procgen::GreaterRealmCell low;
    low.terrain_form = procgen::TerrainForm::Plains;
    low.elevation = 0.52f;

    auto high = low;
    high.elevation = 0.92f;

    auto highland = low;
    highland.terrain_form = procgen::TerrainForm::Highlands;

    const auto low_colour = procgen::greater_realm_debug_colour(low, 0.5f);
    const auto high_colour = procgen::greater_realm_debug_colour(high, 0.5f);
    const auto highland_colour = procgen::greater_realm_debug_colour(highland, 0.5f);
    const auto categorical_plain = procgen::greater_realm_debug_colour(
        low,
        0.5f,
        procgen::GreaterRealmDebugView::TerrainForms
    );
    const auto categorical_highland = procgen::greater_realm_debug_colour(
        highland,
        0.5f,
        procgen::GreaterRealmDebugView::TerrainForms
    );

    bool ok = true;
    ok &= require(low_colour != high_colour, "continuous terrain colour responds to elevation within one form");
    ok &= require(low_colour != highland_colour, "continuous terrain colour retains a restrained terrain-form tint");
    ok &= require(
        categorical_plain != categorical_highland,
        "categorical terrain-form colours remain available as a separate debug view"
    );
    return ok;
}

bool test_continuous_terrain_image_data() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 314159;
    settings.width = 64;
    settings.height = 48;

    const auto base_map = procgen::generate_greater_realm(settings);
    const auto first = procgen::build_greater_realm_debug_image(base_map, settings.sea_level);
    const auto repeated = procgen::build_greater_realm_debug_image(base_map, settings.sea_level);

    auto stronger_settings = settings;
    stronger_settings.mountain_weight = 1.5f;
    const auto stronger_map = procgen::generate_greater_realm(stronger_settings);
    const auto stronger = procgen::build_greater_realm_debug_image(
        stronger_map,
        stronger_settings.sea_level
    );

    bool ok = true;
    ok &= require(
        first.width == settings.width && first.height == settings.height && first.has_expected_byte_count(),
        "continuous terrain image preserves generated map dimensions"
    );
    ok &= require(first.rgba == repeated.rgba, "continuous terrain image output is deterministic");
    ok &= require(
        stronger.width == first.width && stronger.height == first.height,
        "terrain parameter changes preserve debug image dimensions"
    );
    ok &= require(stronger.rgba != first.rgba, "continuous terrain image responds to relief parameters");
    return ok;
}

bool test_debug_base_views() {
    procgen::GreaterRealmMap map;
    map.width = 2;
    map.height = 1;
    map.cells.resize(2);

    auto& low = map.cells[0];
    low.terrain_form = procgen::TerrainForm::Plains;
    low.elevation = 0.55f;
    low.landmass_elevation = -0.8f;
    low.hill_relief = 0.05f;
    low.mountain_relief = 0.05f;
    low.mountain_influence = 0.0f;
    low.slope = 0.0f;
    low.distance_to_coast = 0.0f;
    low.drainage_area = 0.0f;

    auto& high = map.cells[1];
    high.terrain_form = procgen::TerrainForm::Mountains;
    high.elevation = 0.95f;
    high.landmass_elevation = 0.8f;
    high.hill_relief = 0.20f;
    high.mountain_relief = 0.80f;
    high.mountain_influence = 1.0f;
    high.slope = 2.0f;
    high.distance_to_coast = 20.0f;
    high.drainage_area = 100.0f;

    procgen::GreaterRealmDebugOptions options;
    options.show_coastline = false;
    options.show_mountain_peaks = false;
    options.show_rivers = false;

    bool ok = true;
    for (std::uint8_t index = 0;
         index < static_cast<std::uint8_t>(procgen::GreaterRealmDebugView::Count);
         ++index) {
        options.view = static_cast<procgen::GreaterRealmDebugView>(index);
        const auto image = procgen::build_greater_realm_debug_image(map, 0.5f, options);
        ok &= require(image.has_expected_byte_count(), "every debug base view produces a complete image");
        ok &= require(
            image.rgba[0] != image.rgba[4]
                || image.rgba[1] != image.rgba[5]
                || image.rgba[2] != image.rgba[6],
            "every debug base view visualizes differing source values"
        );
        ok &= require(
            image.rgba[3] == 255 && image.rgba[7] == 255,
            "every debug base view remains opaque"
        );
    }
    return ok;
}

bool test_debug_overlay_options() {
    procgen::GreaterRealmMap map;
    map.width = 8;
    map.height = 8;
    map.cells.resize(64);
    for (auto& cell : map.cells) {
        cell.terrain_form = procgen::TerrainForm::Plains;
        cell.elevation = 0.6f;
    }

    constexpr std::uint32_t coastline_index = 1;
    constexpr std::uint32_t peak_index = 2;
    constexpr std::uint32_t river_index = 3;
    constexpr std::uint32_t drainage_index = 36;
    constexpr std::uint32_t drainage_destination = 37;
    map.cells[coastline_index].is_coastal = true;
    map.cells[drainage_index].downslope_index = drainage_destination;
    map.mountain_peaks.push_back({peak_index, 2, 0, 1.0f});
    map.rivers.push_back({river_index, 4, 20.0f, 1.0f});

    procgen::GreaterRealmDebugOptions none;
    none.show_coastline = false;
    none.show_mountain_peaks = false;
    none.show_rivers = false;
    none.show_drainage_directions = false;
    const auto base = procgen::build_greater_realm_debug_image(map, 0.5f, none);

    const auto pixel_differs = [&base](const procgen::DebugImage& image, std::size_t index) {
        const std::size_t pixel = index * 4;
        return image.rgba[pixel] != base.rgba[pixel]
            || image.rgba[pixel + 1] != base.rgba[pixel + 1]
            || image.rgba[pixel + 2] != base.rgba[pixel + 2];
    };

    bool ok = true;
    auto selected = none;
    selected.show_coastline = true;
    ok &= require(
        pixel_differs(procgen::build_greater_realm_debug_image(map, 0.5f, selected), coastline_index),
        "coastline overlay can be enabled independently"
    );

    selected = none;
    selected.show_mountain_peaks = true;
    ok &= require(
        pixel_differs(procgen::build_greater_realm_debug_image(map, 0.5f, selected), peak_index),
        "mountain peak overlay can be enabled independently"
    );

    selected = none;
    selected.show_rivers = true;
    ok &= require(
        pixel_differs(procgen::build_greater_realm_debug_image(map, 0.5f, selected), river_index),
        "river overlay can be enabled independently"
    );

    selected = none;
    selected.show_drainage_directions = true;
    ok &= require(
        pixel_differs(procgen::build_greater_realm_debug_image(map, 0.5f, selected), drainage_index),
        "drainage direction overlay can be enabled independently"
    );
    return ok;
}

bool test_debug_default_options_preserve_existing_image() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 314159;
    settings.width = 48;
    settings.height = 32;
    const auto map = procgen::generate_greater_realm(settings);

    const auto legacy = procgen::build_greater_realm_debug_image(map, settings.sea_level);
    const auto configured = procgen::build_greater_realm_debug_image(
        map,
        settings.sea_level,
        procgen::GreaterRealmDebugOptions{}
    );
    return require(
        legacy.width == configured.width
            && legacy.height == configured.height
            && legacy.rgba == configured.rgba,
        "default debug options preserve the existing terrain image byte for byte"
    );
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
        test_mountain_strength_changes_only_local_mountain_relief,
        test_land_relief_stages_are_inspectable_and_separated,
        test_relief_control_ranges_preserve_topology_and_monotonicity,
        test_mountain_blend_uses_local_positive_constraint,
        test_terrain_noise_changes_land_relief_only,
        test_terrain_noise_range_is_masked_to_land_relief,
        test_coastline_noise_is_local_to_signed_boundary,
        test_coastline_noise_is_independent_from_relief_and_depth_controls,
        test_sea_level_is_not_a_generation_input,
        test_ocean_depth_preserves_topology,
        test_island_bias_matches_mapgen4_contract,
        test_debug_visualization_data,
        test_debug_ocean_depth_shading,
        test_continuous_terrain_colour_mapping,
        test_continuous_terrain_image_data,
        test_debug_base_views,
        test_debug_overlay_options,
        test_debug_default_options_preserve_existing_image,
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
