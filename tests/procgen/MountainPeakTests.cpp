#if !defined(REALM_TEST_BUILD)
#error This test module must only be compiled in test builds.
#endif

#include "procgen/GreaterRealm.hpp"
#include "procgen/GreaterRealmDebug.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace {

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
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 424242;
    settings.width = 128;
    settings.height = 96;
    settings.mountain_peak_spacing = 18.0f;
    const auto first = procgen::generate_greater_realm(settings);
    const auto second = procgen::generate_greater_realm(settings);

    bool metadata_valid = true;
    bool spacing_valid = true;
    for (std::size_t left = 0; left < first.mountain_peaks.size(); ++left) {
        const auto& a = first.mountain_peaks[left];
        const auto& cell = first.cells[a.cell_index];
        metadata_valid &= a.cell_index < first.cells.size();
        metadata_valid &= a.x == cell.x && a.y == cell.y;
        metadata_valid &= !cell.is_water && cell.is_mountain_peak;
        metadata_valid &= cell.mountain_distance == 0.0f && cell.mountain_influence == 1.0f;
        for (std::size_t right = left + 1; right < first.mountain_peaks.size(); ++right) {
            const auto& b = first.mountain_peaks[right];
            const float dx = static_cast<float>(a.x - b.x);
            const float dy = static_cast<float>(a.y - b.y);
            spacing_valid &= std::sqrt(dx * dx + dy * dy) >= settings.mountain_peak_spacing;
        }
    }

    bool ok = true;
    ok &= require(first.mountain_peaks.size() > 1, "test map selects multiple explicit mountain peaks");
    ok &= require(metadata_valid, "peak metadata identifies inland zero-distance cells");
    ok &= require(spacing_valid, "explicit mountain peaks respect minimum spacing");
    ok &= require(peak_fields_match(first, second), "peak identities and distance fields are deterministic");
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

    bool ok = true;
    ok &= require(sparse.mountain_peaks.size() < dense.mountain_peaks.size(), "larger peak spacing produces fewer peaks");
    ok &= require(same_peak_locations(dense, jagged), "jaggedness changes propagation without moving selected peaks");
    ok &= require(mountain_distance_difference(dense, jagged) > 1.0f, "jaggedness changes the mountain distance field");
    ok &= require(same_peak_locations(dense, broad), "peak radius changes falloff without moving selected peaks");
    ok &= require(mountain_influence_sum(broad) > mountain_influence_sum(dense), "larger peak radius broadens mountain influence");
    ok &= require(elevation_difference(dense, broad) > 1.0f, "peak radius changes final terrain relief");
    ok &= require(topology_matches(dense, sparse) && topology_matches(dense, jagged) && topology_matches(dense, broad), "peak parameters do not change landmass topology");
    return ok;
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
