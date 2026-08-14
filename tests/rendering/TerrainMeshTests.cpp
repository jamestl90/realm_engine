#include "rendering/TerrainMesh.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#if !defined(REALM_TEST_BUILD)
#error "TerrainMeshTests.cpp must only be compiled for test builds"
#endif

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool nearly_equal(float left, float right) {
    return std::abs(left - right) < 0.0001f;
}

bool test_mesh_shape_positions_and_colours() {
    const std::vector<float> elevations{
        0.0f, 1.0f, 2.0f,
        0.0f, 1.0f, 2.0f
    };
    std::vector<std::uint8_t> rgba(elevations.size() * 4, 255);
    rgba[0] = 12;
    rgba[1] = 34;
    rgba[2] = 56;

    const auto mesh = rendering::build_heightfield_mesh(3, 2, 2.0f, elevations, rgba);
    bool ok = require(mesh.has_expected_shape(), "mesh has one vertex per cell and two triangles per quad");
    ok &= require(mesh.vertices.size() == 6, "3x2 grid produces six vertices");
    ok &= require(mesh.indices.size() == 12, "3x2 grid produces twelve indices");
    ok &= require(nearly_equal(mesh.extent_x, 4.0f), "mesh records horizontal extent");
    ok &= require(nearly_equal(mesh.extent_y, 2.0f), "mesh records vertical extent");
    ok &= require(nearly_equal(mesh.vertices.front().x, -2.0f), "mesh is horizontally centred");
    ok &= require(nearly_equal(mesh.vertices.front().y, -1.0f), "mesh is vertically centred");
    ok &= require(mesh.vertices.front().r == 12
        && mesh.vertices.front().g == 34
        && mesh.vertices.front().b == 56,
        "mesh transfers RGBA debug colours");
    ok &= require(nearly_equal(mesh.minimum_elevation, 0.0f), "mesh records minimum elevation");
    return ok && require(nearly_equal(mesh.maximum_elevation, 2.0f), "mesh records maximum elevation");
}

bool test_gradients_use_central_and_edge_differences() {
    const std::vector<float> elevations{
        0.0f, 2.0f, 4.0f,
        0.0f, 2.0f, 4.0f,
        0.0f, 2.0f, 4.0f
    };
    const auto mesh = rendering::build_heightfield_mesh(3, 3, 2.0f, elevations);

    bool ok = require(mesh.has_expected_shape(), "gradient test mesh is valid");
    for (const auto& vertex : mesh.vertices) {
        ok &= require(nearly_equal(vertex.gradient_x, 1.0f), "linear x ramp has stable x gradient");
        ok &= require(nearly_equal(vertex.gradient_y, 0.0f), "linear x ramp has zero y gradient");
    }
    return ok;
}

bool test_invalid_inputs_are_rejected() {
    const std::vector<float> too_few{0.0f, 1.0f, 2.0f};
    bool ok = require(
        rendering::build_heightfield_mesh(2, 2, 1.0f, too_few).empty(),
        "mismatched elevation count is rejected"
    );

    const std::vector<float> invalid{
        0.0f, 1.0f,
        2.0f, std::numeric_limits<float>::quiet_NaN()
    };
    ok &= require(
        rendering::build_heightfield_mesh(2, 2, 1.0f, invalid).empty(),
        "non-finite elevation is rejected"
    );
    return ok && require(
        rendering::build_heightfield_mesh(1, 2, 1.0f, too_few).empty(),
        "a grid without drawable quads is rejected"
    );
}

} // namespace

int main() {
    bool ok = true;
    ok &= test_mesh_shape_positions_and_colours();
    ok &= test_gradients_use_central_and_edge_differences();
    ok &= test_invalid_inputs_are_rejected();

    if (!ok) {
        return 1;
    }

    std::cout << "Terrain mesh tests passed.\n";
    return 0;
}
