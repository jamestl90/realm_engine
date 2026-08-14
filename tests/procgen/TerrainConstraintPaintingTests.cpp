#if !defined(REALM_TEST_BUILD)
#error This test module must only be compiled in test builds.
#endif

#include "procgen/TerrainConstraintPainting.hpp"
#include "procgen/TerrainConstraints.hpp"
#include <array>
#include <cmath>
#include <iostream>

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

bool test_preview_coordinate_conversion() {
    const procgen::TerrainPreviewBounds bounds{100.0f, 50.0f, 400.0f, 200.0f};
    const auto top_left = procgen::terrain_constraint_paint_sample(
        bounds,
        procgen::TerrainConstraintTool::Ocean,
        100.0f,
        50.0f
    );
    const auto center = procgen::terrain_constraint_paint_sample(
        bounds,
        procgen::TerrainConstraintTool::Valley,
        300.0f,
        150.0f
    );
    const auto bottom_right = procgen::terrain_constraint_paint_sample(
        bounds,
        procgen::TerrainConstraintTool::Mountain,
        500.0f,
        250.0f
    );

    bool ok = true;
    ok &= require(bounds.is_valid(), "positive finite preview bounds are valid");
    ok &= require(top_left && top_left->normalized_x == 0.0f && top_left->normalized_y == 0.0f, "preview top-left maps to normalized origin");
    ok &= require(center && nearly_equal(center->normalized_x, 0.5f) && nearly_equal(center->normalized_y, 0.5f), "preview center maps to normalized center");
    ok &= require(bottom_right && bottom_right->normalized_x == 1.0f && bottom_right->normalized_y == 1.0f, "preview bottom-right maps to normalized maximum");
    ok &= require(!procgen::terrain_constraint_paint_sample(bounds, procgen::TerrainConstraintTool::Mountain, 99.0f, 150.0f), "points outside the preview do not produce samples");
    ok &= require(!procgen::TerrainPreviewBounds{0.0f, 0.0f, 0.0f, 10.0f}.is_valid(), "zero-width preview bounds are invalid");
    return ok;
}

bool test_paint_session_lifecycle_and_blocking() {
    const procgen::TerrainPreviewBounds bounds{10.0f, 20.0f, 100.0f, 50.0f};
    procgen::TerrainConstraintPaintSession session;
    session.select_tool(procgen::TerrainConstraintTool::ShallowWater);

    bool ok = true;
    ok &= require(!session.pointer_down(bounds, 60.0f, 45.0f, true), "UI-blocked pointer down does not paint");
    ok &= require(!session.is_painting(), "UI-blocked pointer down does not begin a drag");
    ok &= require(!session.pointer_down(bounds, 5.0f, 45.0f, false), "pointer down outside the preview does not paint");

    const auto first = session.pointer_down(bounds, 60.0f, 45.0f, false);
    ok &= require(first && first->tool == procgen::TerrainConstraintTool::ShallowWater, "pointer down begins painting with the selected tool");
    ok &= require(session.is_painting(), "valid pointer down begins a paint drag");
    ok &= require(!session.pointer_move(bounds, 60.0f, 45.0f, false), "duplicate pointer position is suppressed");

    const auto moved = session.pointer_move(bounds, 85.0f, 57.5f, false);
    ok &= require(moved && nearly_equal(moved->normalized_x, 0.75f) && nearly_equal(moved->normalized_y, 0.75f), "drag motion emits normalized paint samples");
    ok &= require(!session.pointer_move(bounds, 85.0f, 57.5f, true), "UI-blocked drag motion does not paint");
    ok &= require(!session.pointer_move(bounds, 200.0f, 57.5f, false), "drag motion outside the preview does not paint");
    ok &= require(session.is_painting(), "leaving or crossing UI does not lose the active drag before release");

    session.pointer_up();
    ok &= require(!session.is_painting(), "pointer up ends the paint drag");
    ok &= require(!session.pointer_move(bounds, 70.0f, 50.0f, false), "motion after release does not paint");
    return ok;
}

bool test_samples_apply_to_constraint_field() {
    procgen::TerrainConstraintField field(32, 24);
    procgen::TerrainConstraintPaintSession session;
    session.select_tool(procgen::TerrainConstraintTool::Mountain);
    const procgen::TerrainPreviewBounds bounds{0.0f, 0.0f, 320.0f, 240.0f};
    const auto sample = session.pointer_down(bounds, 160.0f, 120.0f, false);
    if (!sample) {
        return require(false, "valid paint session emits a sample for constraint integration");
    }

    field.paint(sample->tool, sample->normalized_x, sample->normalized_y, 0.12f);
    const auto center = field.sample(0.5f, 0.5f);

    bool ok = true;
    ok &= require(center.influence > 0.0f, "paint sample applies brush influence to the constraint field");
    ok &= require(center.elevation > 0.0f, "selected mountain tool applies positive constrained elevation");
    return ok;
}

} // namespace

int main() {
    const std::array tests{
        test_preview_coordinate_conversion,
        test_paint_session_lifecycle_and_blocking,
        test_samples_apply_to_constraint_field
    };

    bool ok = true;
    for (const auto& test : tests) {
        ok &= test();
    }
    if (!ok) {
        return 1;
    }

    std::cout << "Terrain constraint painting tests passed.\n";
    return 0;
}
