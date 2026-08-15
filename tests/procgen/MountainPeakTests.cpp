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

namespace {

struct PeakScenario {
    procgen::Seed seed{1};
    std::uint32_t width{0};
    std::uint32_t height{0};
    float spacing{0.0f};
};

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool peak_fields_match(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    if (a.mountain_peaks.size() != b.mountain_peaks.size() || a.cells.size() != b.cells.size()) {
        return false;
    }
    for (std::size_t index = 0; index < a.mountain_peaks.size(); ++index) {
        const auto& left = a.mountain_peaks[index];
        const auto& right = b.mountain_peaks[index];
        if (left.cell_index != right.cell_index
            || left.x != right.x
            || left.y != right.y
            || left.priority != right.priority) {
            return false;
        }
    }
    for (std::size_t index = 0; index < a.cells.size(); ++index) {
        if (a.cells[index].mountain_distance != b.cells[index].mountain_distance
            || a.cells[index].mountain_influence != b.cells[index].mountain_influence
            || a.cells[index].is_mountain_peak != b.cells[index].is_mountain_peak) {
            return false;
        }
    }
    return true;
}

bool same_peak_locations(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    if (a.mountain_peaks.size() != b.mountain_peaks.size()) {
        return false;
    }
    for (std::size_t index = 0; index < a.mountain_peaks.size(); ++index) {
        if (a.mountain_peaks[index].cell_index != b.mountain_peaks[index].cell_index) {
            return false;
        }
    }
    return true;
}

bool peak_metadata_and_spacing_valid(const procgen::GreaterRealmMap& map, float spacing) {
    for (std::size_t left = 0; left < map.mountain_peaks.size(); ++left) {
        const auto& a = map.mountain_peaks[left];
        if (a.cell_index >= map.cells.size()) {
            return false;
        }

        const auto& cell = map.cells[a.cell_index];
        if (a.x != cell.x
            || a.y != cell.y
            || cell.is_water
            || !cell.is_mountain_peak
            || cell.mountain_distance != 0.0f
            || cell.mountain_influence != 1.0f) {
            return false;
        }

        for (std::size_t right = left + 1; right < map.mountain_peaks.size(); ++right) {
            const auto& b = map.mountain_peaks[right];
            const float dx = static_cast<float>(a.x - b.x);
            const float dy = static_cast<float>(a.y - b.y);
            if (std::sqrt(dx * dx + dy * dy) < spacing) {
                return false;
            }
        }
    }
    return true;
}

bool peak_distribution_has_span(const procgen::GreaterRealmMap& map) {
    if (map.mountain_peaks.size() < 3) {
        return true;
    }

    auto min_x = map.mountain_peaks.front().x;
    auto max_x = map.mountain_peaks.front().x;
    auto min_y = map.mountain_peaks.front().y;
    auto max_y = map.mountain_peaks.front().y;
    for (const auto& peak : map.mountain_peaks) {
        min_x = std::min(min_x, peak.x);
        max_x = std::max(max_x, peak.x);
        min_y = std::min(min_y, peak.y);
        max_y = std::max(max_y, peak.y);
    }

    const auto x_span = static_cast<std::uint32_t>(max_x - min_x);
    const auto y_span = static_cast<std::uint32_t>(max_y - min_y);
    return x_span > map.width / 6 || y_span > map.height / 6;
}

bool has_peak_at(const procgen::GreaterRealmMap& map, std::uint32_t cell_index) {
    return std::any_of(
        map.mountain_peaks.begin(),
        map.mountain_peaks.end(),
        [cell_index](const procgen::GreaterRealmMountainPeak& peak) {
            return peak.cell_index == cell_index;
        }
    );
}

float normalized_x(const procgen::GreaterRealmMap& map, std::uint32_t cell_index) {
    const auto& cell = map.cells[cell_index];
    return map.width > 1 ? static_cast<float>(cell.x) / static_cast<float>(map.width - 1) : 0.0f;
}

float normalized_y(const procgen::GreaterRealmMap& map, std::uint32_t cell_index) {
    const auto& cell = map.cells[cell_index];
    return map.height > 1 ? static_cast<float>(cell.y) / static_cast<float>(map.height - 1) : 0.0f;
}

bool distant_peak_locations_stable(
    const procgen::GreaterRealmMap& baseline,
    const procgen::GreaterRealmMap& edited,
    float center_x,
    float center_y,
    float radius
) {
    for (const auto& peak : baseline.mountain_peaks) {
        const float dx = normalized_x(baseline, peak.cell_index) - center_x;
        const float dy = normalized_y(baseline, peak.cell_index) - center_y;
        if (std::sqrt(dx * dx + dy * dy) <= radius) {
            continue;
        }
        if (!has_peak_at(edited, peak.cell_index)) {
            return false;
        }
    }

    for (const auto& peak : edited.mountain_peaks) {
        const float dx = normalized_x(edited, peak.cell_index) - center_x;
        const float dy = normalized_y(edited, peak.cell_index) - center_y;
        if (std::sqrt(dx * dx + dy * dy) <= radius) {
            continue;
        }
        if (!has_peak_at(baseline, peak.cell_index)) {
            return false;
        }
    }
    return true;
}

float mountain_distance_difference(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    float total = 0.0f;
    for (std::size_t index = 0; index < std::min(a.cells.size(), b.cells.size()); ++index) {
        total += std::abs(a.cells[index].mountain_distance - b.cells[index].mountain_distance);
    }
    return total;
}

float mountain_influence_sum(const procgen::GreaterRealmMap& map) {
    float total = 0.0f;
    for (const auto& cell : map.cells) {
        total += cell.mountain_influence;
    }
    return total;
}

float elevation_difference(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    float total = 0.0f;
    for (std::size_t index = 0; index < std::min(a.cells.size(), b.cells.size()); ++index) {
        total += std::abs(a.cells[index].elevation - b.cells[index].elevation);
    }
    return total;
}

bool topology_matches(const procgen::GreaterRealmMap& a, const procgen::GreaterRealmMap& b) {
    if (a.cells.size() != b.cells.size()) {
        return false;
    }
    for (std::size_t index = 0; index < a.cells.size(); ++index) {
        if (a.cells[index].is_water != b.cells[index].is_water
            || a.cells[index].is_ocean != b.cells[index].is_ocean
            || a.cells[index].landmass_elevation != b.cells[index].landmass_elevation) {
            return false;
        }
    }
    return true;
}

bool test_peak_selection_is_deterministic_and_spaced() {
    constexpr std::array scenarios{
        PeakScenario{424242, 128, 96, 8.0f},
        PeakScenario{314159, 96, 72, 28.0f},
        PeakScenario{8675309, 128, 96, 56.0f}
    };

    bool ok = true;
    std::size_t scenarios_with_multiple_peaks = 0;
    std::size_t distributed_scenarios = 0;
    for (const auto& scenario : scenarios) {
        procgen::GreaterRealmGeneratorSettings settings;
        settings.seed = scenario.seed;
        settings.width = scenario.width;
        settings.height = scenario.height;
        settings.mountain_peak_spacing = scenario.spacing;
        const auto first = procgen::generate_greater_realm(settings);
        const auto second = procgen::generate_greater_realm(settings);

        if (first.mountain_peaks.size() > 1) {
            ++scenarios_with_multiple_peaks;
        }
        if (first.mountain_peaks.size() >= 3 && peak_distribution_has_span(first)) {
            ++distributed_scenarios;
        }

        ok &= require(!first.mountain_peaks.empty(), "representative map selects explicit mountain peaks");
        ok &= require(peak_metadata_and_spacing_valid(first, settings.mountain_peak_spacing), "peak metadata identifies spaced inland zero-distance cells");
        ok &= require(peak_fields_match(first, second), "peak identities and distance fields are deterministic");
    }

    ok &= require(scenarios_with_multiple_peaks >= 2, "lower/default/upper spacing scenarios include multi-peak distributions");
    ok &= require(distributed_scenarios >= 1, "representative peak distribution spans the island instead of collapsing into a sampling band");
    return ok;
}

bool test_distance_field_descends_to_a_peak() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 515151;
    settings.width = 96;
    settings.height = 72;
    settings.mountain_peak_spacing = 20.0f;
    settings.mountain_peak_jaggedness = 0.75f;
    const auto map = procgen::generate_greater_realm(settings);

    bool valid = !map.mountain_peaks.empty();
    for (const auto& cell : map.cells) {
        valid &= std::isfinite(cell.mountain_distance);
        valid &= cell.mountain_influence >= 0.0f && cell.mountain_influence <= 1.0f;
        if (cell.is_mountain_peak) {
            continue;
        }

        bool has_lower_neighbor = false;
        for (std::int32_t y = cell.y - 1; y <= cell.y + 1 && !has_lower_neighbor; ++y) {
            for (std::int32_t x = cell.x - 1; x <= cell.x + 1; ++x) {
                if (x == cell.x && y == cell.y) {
                    continue;
                }
                const auto* neighbor = map.cell(x, y);
                if (neighbor && neighbor->mountain_distance < cell.mountain_distance) {
                    has_lower_neighbor = true;
                    break;
                }
            }
        }
        valid &= has_lower_neighbor;
    }

    return require(valid, "every non-peak cell has a descending distance path toward a peak");
}

bool test_peak_parameters_control_count_shape_and_relief() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 616161;
    settings.width = 128;
    settings.height = 96;
    settings.mountain_peak_spacing = 14.0f;
    settings.mountain_peak_radius = 14.0f;
    settings.mountain_peak_jaggedness = 0.0f;
    const auto dense = procgen::generate_greater_realm(settings);

    settings.mountain_peak_spacing = 48.0f;
    const auto sparse = procgen::generate_greater_realm(settings);

    settings.mountain_peak_spacing = 14.0f;
    settings.mountain_peak_jaggedness = 1.0f;
    const auto jagged = procgen::generate_greater_realm(settings);

    settings.mountain_peak_jaggedness = 0.0f;
    settings.mountain_peak_radius = 48.0f;
    const auto broad = procgen::generate_greater_realm(settings);

    settings.mountain_peak_radius = 14.0f;
    settings.mountain_weight = 1.0f;
    const auto stronger = procgen::generate_greater_realm(settings);

    settings.mountain_weight = procgen::DEFAULT_MOUNTAIN_STRENGTH;
    settings.base_elevation_weight = 0.25f;
    settings.ridge_weight = 1.0f;
    settings.valley_weight = 0.0f;
    settings.terrain_noise_weight = 1.0f;
    const auto relief_controls = procgen::generate_greater_realm(settings);

    bool ok = true;
    ok &= require(sparse.mountain_peaks.size() < dense.mountain_peaks.size(), "larger peak spacing produces fewer peaks");
    ok &= require(same_peak_locations(dense, jagged), "jaggedness changes propagation without moving selected peaks");
    ok &= require(mountain_distance_difference(dense, jagged) > 1.0f, "jaggedness changes the mountain distance field");
    ok &= require(same_peak_locations(dense, broad), "peak radius changes falloff without moving selected peaks");
    ok &= require(same_peak_locations(dense, stronger), "mountain strength changes relief without moving selected peaks");
    ok &= require(same_peak_locations(dense, relief_controls), "non-spacing relief controls do not move selected peaks");
    ok &= require(mountain_influence_sum(broad) > mountain_influence_sum(dense), "larger peak radius broadens mountain influence");
    ok &= require(elevation_difference(dense, broad) > 1.0f, "peak radius changes final terrain relief");
    ok &= require(topology_matches(dense, sparse) && topology_matches(dense, jagged) && topology_matches(dense, broad) && topology_matches(dense, stronger) && topology_matches(dense, relief_controls), "peak parameters do not change landmass topology");
    return ok;
}

bool test_authored_constraints_do_not_relocate_fixed_peak_sites() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 314159;
    settings.width = 96;
    settings.height = 72;
    settings.coastline_noise_weight = 0.0f;
    const auto baseline = procgen::generate_greater_realm(settings);
    if (baseline.mountain_peaks.empty()) {
        return require(false, "baseline map exposes a peak for authored-constraint stability");
    }

    const std::uint32_t edited_peak_index = baseline.mountain_peaks.front().cell_index;
    const float peak_x = normalized_x(baseline, edited_peak_index);
    const float peak_y = normalized_y(baseline, edited_peak_index);
    constexpr float brush_radius = 0.08f;

    procgen::TerrainConstraintField ocean_constraints(settings.width, settings.height);
    ocean_constraints.paint(procgen::TerrainConstraintTool::Ocean, peak_x, peak_y, brush_radius);
    const auto ocean_edit = procgen::generate_greater_realm(settings, ocean_constraints);

    auto restored_constraints = ocean_constraints;
    restored_constraints.paint(procgen::TerrainConstraintTool::Mountain, peak_x, peak_y, brush_radius);
    const auto restored_edit = procgen::generate_greater_realm(settings, restored_constraints);

    bool ok = true;
    ok &= require(ocean_edit.cells[edited_peak_index].is_water, "ocean brush can make an existing fixed peak site dormant");
    ok &= require(!has_peak_at(ocean_edit, edited_peak_index), "dormant water peak site is not exported as an active peak");
    ok &= require(!restored_edit.cells[edited_peak_index].is_water, "mountain brush can restore land at the fixed peak site");
    ok &= require(has_peak_at(restored_edit, edited_peak_index), "restored fixed peak site becomes active again");
    ok &= require(
        distant_peak_locations_stable(baseline, ocean_edit, peak_x, peak_y, brush_radius + 0.04f),
        "ocean brush does not relocate distant active peak sites"
    );
    return ok && require(
        distant_peak_locations_stable(baseline, restored_edit, peak_x, peak_y, brush_radius + 0.04f),
        "mountain brush does not relocate distant active peak sites"
    );
}

bool test_peak_markers_reach_debug_image() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 717171;
    settings.width = 96;
    settings.height = 72;
    const auto map = procgen::generate_greater_realm(settings);
    const auto image = procgen::build_greater_realm_debug_image(map, settings.sea_level);

    bool ok = true;
    ok &= require(!map.mountain_peaks.empty(), "debug map exports mountain peaks");
    ok &= require(image.has_expected_byte_count(), "peak debug image has valid storage");
    if (!map.mountain_peaks.empty() && image.has_expected_byte_count()) {
        const std::size_t pixel = static_cast<std::size_t>(map.mountain_peaks.front().cell_index) * 4;
        ok &= require(
            image.rgba[pixel] == 232 && image.rgba[pixel + 1] == 62 && image.rgba[pixel + 2] == 48,
            "debug image marks explicit peak cells"
        );
    }
    return ok;
}

} // namespace

int main() {
    const std::array tests{
        test_peak_selection_is_deterministic_and_spaced,
        test_distance_field_descends_to_a_peak,
        test_peak_parameters_control_count_shape_and_relief,
        test_authored_constraints_do_not_relocate_fixed_peak_sites,
        test_peak_markers_reach_debug_image
    };

    bool ok = true;
    for (const auto& test : tests) {
        ok &= test();
    }
    if (!ok) {
        return 1;
    }

    std::cout << "Mountain peak distance-field tests passed.\n";
    return 0;
}
