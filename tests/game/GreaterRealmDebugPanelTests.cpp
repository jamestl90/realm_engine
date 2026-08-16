#include "game/GreaterRealmDebugPanel.hpp"
#include "ui/Button.hpp"
#include "ui/InputSurface.hpp"
#include "ui/Slider.hpp"
#include <cmath>
#include <iostream>
#include <vector>

#if !defined(REALM_TEST_BUILD)
#error "GreaterRealmDebugPanelTests.cpp must only be compiled for test builds"
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

procgen::GreaterRealmMap make_smoke_map() {
    procgen::GreaterRealmMap map;
    map.seed = 1;
    map.width = 2;
    map.height = 2;
    map.cells.resize(4);
    for (std::uint32_t y = 0; y < map.height; ++y) {
        for (std::uint32_t x = 0; x < map.width; ++x) {
            auto& cell = map.cells[static_cast<std::size_t>(y) * map.width + x];
            cell.x = static_cast<std::int32_t>(x);
            cell.y = static_cast<std::int32_t>(y);
            cell.terrain_form = procgen::TerrainForm::Plains;
        }
    }
    return map;
}

void collect_sliders(ui::UIElement* element, std::vector<ui::Slider*>& sliders) {
    if (!element) {
        return;
    }
    if (auto* slider = dynamic_cast<ui::Slider*>(element)) {
        sliders.push_back(slider);
    }
    for (const auto& child : element->children()) {
        collect_sliders(child.get(), sliders);
    }
}

void collect_buttons(ui::UIElement* element, std::vector<ui::Button*>& buttons) {
    if (!element) {
        return;
    }
    if (auto* button = dynamic_cast<ui::Button*>(element)) {
        buttons.push_back(button);
    }
    for (const auto& child : element->children()) {
        collect_buttons(child.get(), buttons);
    }
}

ui::MouseEventArgs left_mouse_event(ui::Slider& slider, float localX) {
    ui::MouseEventArgs args;
    args.x = slider.bounds().x + localX;
    args.y = slider.bounds().y + slider.bounds().height * 0.5f;
    args.button = ui::MouseButton::Left;
    return args;
}

void click_button(ui::Button& button) {
    button.setBounds(ui::Rect{0.0f, 0.0f, 120.0f, 30.0f});
    auto* surface = dynamic_cast<ui::InputSurface*>(&button);
    surface->onMouseEnter();

    ui::MouseEventArgs down;
    down.x = 10.0f;
    down.y = 10.0f;
    down.button = ui::MouseButton::Left;
    surface->onMouseDown(down);

    ui::MouseEventArgs up = down;
    surface->onMouseUp(up);
}

void click_slider_at(ui::Slider& slider, float localX) {
    slider.setBounds(ui::Rect{0.0f, 0.0f, 110.0f, 26.0f});
    auto* surface = dynamic_cast<ui::InputSurface*>(&slider);
    auto down = left_mouse_event(slider, localX);
    surface->onMouseDown(down);
    auto up = left_mouse_event(slider, localX);
    surface->onMouseUp(up);
}

bool test_panel_sliders_update_settings_and_callbacks() {
    procgen::GreaterRealmGeneratorSettings settings;
    procgen::GreaterRealmDebugOptions debugOptions;
    game::GreaterRealmPresentationSettings presentation;
    procgen::TerrainConstraintBrushSettings brushSettings;
    const auto map = make_smoke_map();

    int regenerations = 0;
    int presentationChanges = 0;
    int brushChanges = 0;
    game::GreaterRealmDebugPanel panel;
    auto root = panel.build(
        settings,
        debugOptions,
        presentation,
        brushSettings,
        map,
        [&regenerations](bool) { ++regenerations; },
        [](procgen::TerrainConstraintTool) {},
        [&brushChanges](procgen::TerrainConstraintBrushSettings) { ++brushChanges; },
        []() {},
        []() {},
        [&presentationChanges]() { ++presentationChanges; }
    );

    std::vector<ui::Slider*> sliders;
    collect_sliders(root.get(), sliders);
    bool ok = require(sliders.size() >= 15, "debug panel builds reusable sliders for presentation, generator, and brush settings");
    ok &= require(nearly_equal(sliders[2]->value(), 0.01f), "coast-detail slider starts at Mapgen4's default");
    ok &= require(nearly_equal(sliders[2]->minimum(), 0.0f), "coast-detail slider starts at zero");
    ok &= require(nearly_equal(sliders[2]->maximum(), 0.10f), "coast-detail slider uses Mapgen4's upper bound");

    std::vector<ui::Button*> buttons;
    collect_buttons(root.get(), buttons);
    ok &= require(buttons.size() >= 2, "debug panel builds Flat and 3D presentation buttons");

    click_button(*buttons[1]);
    ok &= require(
        presentation.mode == game::GreaterRealmPresentationMode::Tilted3D,
        "3D button updates presentation mode"
    );
    ok &= require(presentationChanges == 1, "3D button emits presentation callback");

    click_slider_at(*sliders[0], 110.0f);
    ok &= require(presentation.elevation_scale == 250.0f, "elevation slider updates presentation scale");
    ok &= require(presentationChanges == 2, "elevation slider emits presentation callback");
    ok &= require(regenerations == 0, "elevation slider does not regenerate procedural map");

    click_slider_at(*sliders[1], 110.0f);
    ok &= require(settings.island_bias == 1.0f, "first generator slider updates island bias");
    ok &= require(regenerations == 1, "generator slider emits regenerate callback");

    const int brushChangesBeforeSliders = brushChanges;
    click_slider_at(*sliders[sliders.size() - 2], 105.0f);
    ok &= require(
        nearly_equal(brushSettings.normalized_radius, procgen::MAX_TERRAIN_CONSTRAINT_BRUSH_RADIUS),
        "brush-size slider updates radius setting"
    );
    click_slider_at(*sliders[sliders.size() - 1], 5.0f);
    ok &= require(brushSettings.strength == 0.0f, "brush-strength slider updates strength setting");
    ok &= require(brushChanges >= brushChangesBeforeSliders + 2, "brush sliders emit brush-setting callbacks");
    ok &= require(regenerations == 1, "brush sliders do not regenerate until painting");
    return ok;
}

} // namespace

int main() {
    if (!test_panel_sliders_update_settings_and_callbacks()) {
        return 1;
    }

    std::cout << "Greater realm debug panel slider tests passed.\n";
    return 0;
}
