#include "game/GreaterRealmDebugPanel.hpp"
#include "ui/Button.hpp"
#include "ui/InputSurface.hpp"
#include "ui/Primitives.hpp"
#include "ui/Slider.hpp"
#include <algorithm>
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
    map.cells[0].terrain_form = procgen::TerrainForm::Ocean;
    map.cells[0].is_water = true;
    map.cells[0].is_ocean = true;
    map.cells[1].terrain_form = procgen::TerrainForm::InlandWater;
    map.cells[1].is_water = true;
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

void collect_text_blocks(ui::UIElement* element, std::vector<ui::TextBlock*>& text_blocks) {
    if (!element) {
        return;
    }
    if (auto* text = dynamic_cast<ui::TextBlock*>(element)) {
        text_blocks.push_back(text);
    }
    for (const auto& child : element->children()) {
        collect_text_blocks(child.get(), text_blocks);
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
    procgen::TemperatureNormalSummary temperatureSummary;
    temperatureSummary.minimum = 0.25f;
    temperatureSummary.maximum = 0.75f;
    temperatureSummary.mean = 0.50f;
    temperatureSummary.sample_count = map.cells.size();
    procgen::PrecipitationNormalSummary precipitationSummary;
    precipitationSummary.minimum = 0.20f;
    precipitationSummary.maximum = 0.80f;
    precipitationSummary.mean = 0.45f;
    precipitationSummary.sample_count = map.cells.size();

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
        temperatureSummary,
        precipitationSummary,
        [&regenerations](bool) { ++regenerations; },
        [](procgen::TerrainConstraintTool) {},
        [&brushChanges](procgen::TerrainConstraintBrushSettings) { ++brushChanges; },
        []() {},
        []() {},
        [&presentationChanges]() { ++presentationChanges; }
    );

    std::vector<ui::Slider*> sliders;
    collect_sliders(root.get(), sliders);
    bool ok = require(sliders.size() >= 16, "debug panel builds reusable sliders for presentation, generator, and brush settings");
    ok &= require(nearly_equal(sliders[2]->value(), 1.0f), "seed-variation slider defaults to full deterministic character");
    ok &= require(nearly_equal(sliders[2]->minimum(), 0.0f) && nearly_equal(sliders[2]->maximum(), 1.0f), "seed-variation slider exposes neutral through full character");
    ok &= require(nearly_equal(sliders[3]->value(), 0.01f), "coast-detail slider starts at Mapgen4's default");
    ok &= require(nearly_equal(sliders[3]->minimum(), 0.0f), "coast-detail slider starts at zero");
    ok &= require(nearly_equal(sliders[3]->maximum(), 0.10f), "coast-detail slider uses Mapgen4's upper bound");

    std::vector<ui::Button*> buttons;
    collect_buttons(root.get(), buttons);
    ok &= require(buttons.size() >= 2, "debug panel builds Flat and 3D presentation buttons");

    std::vector<ui::TextBlock*> text_blocks;
    collect_text_blocks(root.get(), text_blocks);
    const bool has_water_coverage = std::any_of(
        text_blocks.begin(),
        text_blocks.end(),
        [](const ui::TextBlock* text) {
            return text->text() == "Land 50.00%  Ocean 25.00%  Inland 25.00%";
        }
    );
    ok &= require(has_water_coverage, "coverage summary separates ocean and inland water from land");
    const bool has_temperature_summary = std::any_of(
        text_blocks.begin(),
        text_blocks.end(),
        [](const ui::TextBlock* text) {
            return text->text() == "Temp 50.00% (25.00-75.00)  Precip 45.00% (20.00-80.00)";
        }
    );
    ok &= require(has_temperature_summary, "temperature summary reports fixed-scale mean and range");

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
