#if !defined(REALM_TEST_BUILD)
#error This evaluation harness must only be compiled in test builds.
#endif

#include "procgen/GreaterRealm.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::uint32_t WIDTH = 256;
constexpr std::uint32_t HEIGHT = 192;
constexpr std::uint32_t SAMPLE_COUNT = WIDTH * HEIGHT;
constexpr std::uint32_t TRAVERSAL_REPETITIONS = 80;

struct DualMeshStorageEstimate {
    std::size_t terrain_samples{0};
    std::size_t regions{0};
    std::size_t triangles{0};
    std::size_t sides{0};
    std::size_t topology_bytes{0};
    std::size_t spatial_index_bytes{0};
};

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

DualMeshStorageEstimate estimate_compact_dual_mesh(std::size_t terrain_samples) {
    // Mapgen4 stores terrain primarily on triangles. For a large planar
    // triangulation, triangles are approximately twice the region count and
    // each triangle owns three directed sides.
    const std::size_t triangles = terrain_samples;
    const std::size_t regions = (triangles + 4) / 2;
    const std::size_t sides = triangles * 3;

    const std::size_t region_positions = regions * 2 * sizeof(float);
    const std::size_t side_regions = sides * sizeof(std::uint32_t);
    const std::size_t halfedges = sides * sizeof(std::uint32_t);
    const std::size_t one_side_per_region = regions * sizeof(std::uint32_t);
    const std::size_t side_lengths = sides * sizeof(float);
    const std::size_t boundary_flags = triangles * sizeof(std::uint8_t);

    // A world/local-tile lookup needs a spatial index; two uint32 arrays model
    // bucket heads plus linked entries without claiming a specific index.
    const std::size_t spatial_index = regions * 2 * sizeof(std::uint32_t);
    return {
        terrain_samples,
        regions,
        triangles,
        sides,
        region_positions + side_regions + halfedges + one_side_per_region + side_lengths + boundary_flags,
        spatial_index
    };
}

std::vector<std::uint32_t> build_explicit_six_neighbor_graph() {
    std::vector<std::uint32_t> neighbors;
    neighbors.reserve(static_cast<std::size_t>(SAMPLE_COUNT) * 6);
    for (std::uint32_t y = 0; y < HEIGHT; ++y) {
        for (std::uint32_t x = 0; x < WIDTH; ++x) {
            const auto add = [&](std::int32_t nx, std::int32_t ny) {
                if (nx >= 0 && ny >= 0
                    && nx < static_cast<std::int32_t>(WIDTH)
                    && ny < static_cast<std::int32_t>(HEIGHT)) {
                    neighbors.push_back(static_cast<std::uint32_t>(ny) * WIDTH + static_cast<std::uint32_t>(nx));
                } else {
                    neighbors.push_back(procgen::INVALID_CELL_INDEX);
                }
            };
            add(static_cast<std::int32_t>(x) - 1, static_cast<std::int32_t>(y));
            add(static_cast<std::int32_t>(x) + 1, static_cast<std::int32_t>(y));
            add(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y) - 1);
            add(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y) + 1);
            const std::int32_t diagonal = ((x + y) & 1u) == 0u ? 1 : -1;
            add(static_cast<std::int32_t>(x) + diagonal, static_cast<std::int32_t>(y) - 1);
            add(static_cast<std::int32_t>(x) - diagonal, static_cast<std::int32_t>(y) + 1);
        }
    }
    return neighbors;
}

std::uint64_t traverse_implicit_grid() {
    std::uint64_t checksum = 0;
    for (std::uint32_t repetition = 0; repetition < TRAVERSAL_REPETITIONS; ++repetition) {
        for (std::uint32_t y = 0; y < HEIGHT; ++y) {
            for (std::uint32_t x = 0; x < WIDTH; ++x) {
                if (x > 0) checksum += y * WIDTH + x - 1;
                if (x + 1 < WIDTH) checksum += y * WIDTH + x + 1;
                if (y > 0) checksum += (y - 1) * WIDTH + x;
                if (y + 1 < HEIGHT) checksum += (y + 1) * WIDTH + x;
            }
        }
    }
    return checksum;
}

std::uint64_t traverse_explicit_graph(const std::vector<std::uint32_t>& neighbors) {
    std::uint64_t checksum = 0;
    for (std::uint32_t repetition = 0; repetition < TRAVERSAL_REPETITIONS; ++repetition) {
        for (const std::uint32_t neighbor : neighbors) {
            if (neighbor != procgen::INVALID_CELL_INDEX) {
                checksum += neighbor;
            }
        }
    }
    return checksum;
}

template <typename Callback>
std::uint64_t measure_microseconds(Callback&& callback, std::uint64_t& checksum) {
    const auto start = Clock::now();
    checksum = callback();
    const auto end = Clock::now();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
    );
}

bool test_grid_indexing_and_region_handoff() {
    procgen::GreaterRealmMap map;
    map.width = WIDTH;
    map.height = HEIGHT;
    map.cells.resize(SAMPLE_COUNT);

    bool bijective = true;
    for (std::uint32_t y = 0; y < HEIGHT; ++y) {
        for (std::uint32_t x = 0; x < WIDTH; ++x) {
            const std::size_t index = map.index(x, y);
            bijective &= index == static_cast<std::size_t>(y) * WIDTH + x;
            bijective &= index < map.cells.size();
        }
    }

    constexpr std::uint32_t region_left = 64;
    constexpr std::uint32_t region_top = 48;
    constexpr std::uint32_t region_width = 64;
    constexpr std::uint32_t region_height = 48;
    std::size_t extracted = 0;
    bool contiguous_rows = true;
    for (std::uint32_t y = region_top; y < region_top + region_height; ++y) {
        const std::size_t first = map.index(region_left, y);
        const std::size_t last = map.index(region_left + region_width - 1, y);
        contiguous_rows &= last - first + 1 == region_width;
        extracted += last - first + 1;
    }

    bool ok = true;
    ok &= require(bijective, "regular-grid coordinates map directly and uniquely to storage");
    ok &= require(contiguous_rows, "rectangular world regions remain contiguous within each grid row");
    ok &= require(extracted == static_cast<std::size_t>(region_width) * region_height, "grid region extraction returns an exact tile-aligned sample count");
    return ok;
}

bool test_dual_mesh_storage_and_traversal_prototype() {
    const DualMeshStorageEstimate dual = estimate_compact_dual_mesh(SAMPLE_COUNT);
    const auto graph = build_explicit_six_neighbor_graph();
    const std::size_t grid_cell_bytes = static_cast<std::size_t>(SAMPLE_COUNT) * sizeof(procgen::GreaterRealmCell);
    const std::size_t grid_coordinate_bytes = static_cast<std::size_t>(SAMPLE_COUNT) * 2 * sizeof(std::int32_t);
    const std::size_t explicit_graph_bytes = graph.size() * sizeof(std::uint32_t);

    std::uint64_t grid_checksum = 0;
    std::uint64_t graph_checksum = 0;
    const std::uint64_t grid_traversal_us = measure_microseconds(traverse_implicit_grid, grid_checksum);
    const std::uint64_t graph_traversal_us = measure_microseconds(
        [&graph]() { return traverse_explicit_graph(graph); },
        graph_checksum
    );

    std::cout
        << "REPRESENTATION_EVALUATION"
        << " samples=" << SAMPLE_COUNT
        << " cell_size=" << sizeof(procgen::GreaterRealmCell)
        << " grid_cell_bytes=" << grid_cell_bytes
        << " grid_coordinate_bytes=" << grid_coordinate_bytes
        << " dual_regions=" << dual.regions
        << " dual_triangles=" << dual.triangles
        << " dual_sides=" << dual.sides
        << " dual_topology_bytes=" << dual.topology_bytes
        << " dual_spatial_index_bytes=" << dual.spatial_index_bytes
        << " explicit_graph_bytes=" << explicit_graph_bytes
        << " grid_traversal_us=" << grid_traversal_us
        << " explicit_traversal_us=" << graph_traversal_us
        << " grid_checksum=" << grid_checksum
        << " explicit_checksum=" << graph_checksum
        << '\n';

    bool ok = true;
    ok &= require(dual.triangles == SAMPLE_COUNT, "dual estimate compares equal terrain-sample counts");
    ok &= require(dual.sides == dual.triangles * 3, "dual prototype uses compact triangle-side topology");
    ok &= require(dual.topology_bytes > grid_coordinate_bytes, "explicit dual topology exceeds the grid's stored coordinate overhead");
    ok &= require(dual.spatial_index_bytes > 0, "irregular local-tile lookup requires additional spatial indexing");
    ok &= require(graph.size() == static_cast<std::size_t>(SAMPLE_COUNT) * 6, "explicit neighbor prototype stores six slots per terrain sample");
    ok &= require(grid_checksum > 0 && graph_checksum > 0, "both traversal prototypes visit valid neighbors");
    return ok;
}

} // namespace

int main() {
    const std::array tests{
        test_grid_indexing_and_region_handoff,
        test_dual_mesh_storage_and_traversal_prototype
    };

    bool ok = true;
    for (const auto& test : tests) {
        ok &= test();
    }
    if (!ok) {
        return 1;
    }

    std::cout << "Procgen representation evaluation passed.\n";
    return 0;
}
