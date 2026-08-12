#include "GreaterRealmDebugPanel.hpp"
#include "../../include/procgen/GreaterRealmDebug.hpp"
#include "../../include/ui/Button.hpp"
#include "../../include/ui/Layout.hpp"
#include "../../include/ui/Primitives.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

namespace game {
namespace {

std::string format_float(float value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

std::string setting_text(const char* label, float value) {
    std::ostringstream stream;
    stream << label << ": " << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

std::string seed_text(procgen::Seed seed) {
    std::ostringstream stream;
    stream << "Seed: " << seed;
    return stream.str();
}

std::string coverage_text(const procgen::GreaterRealmMap& map, const procgen::TerrainFormCounts& counts) {
    const auto total = static_cast<float>(map.cells.size());
    const float land_percent = total > 0.0f ? (total - static_cast<float>(counts.ocean)) * 100.0f / total : 0.0f;
    const float ocean_percent = total > 0.0f ? static_cast<float>(counts.ocean) * 100.0f / total : 0.0f;

    std::ostringstream stream;
    stream << "Land " << format_float(land_percent) << "%  Ocean " << format_float(ocean_percent) << "%";
    return stream.str();
}

std::string terrain_text(const procgen::TerrainFormCounts& counts) {
    std::ostringstream stream;
    stream << "Coastal " << counts.coastal_land << "  Mountains " << counts.mountains;
    return stream.str();
}

std::string hydrology_text(const procgen::GreaterRealmMap& map) {
    std::ostringstream stream;
    stream << "Peaks " << map.mountain_peaks.size() << "  Rivers " << map.rivers.size()
           << "  Drainage " << map.drainage_order.size();
    return stream.str();
}

std::unique_ptr<ui::TextBlock> make_text(const std::string& text, ui::TextBlock** out = nullptr) {
    auto element = std::make_unique<ui::TextBlock>(text);
    element->setFontSize(14.0f);
    element->setColour(ui::Colour{20, 24, 30, 255});
    if (out) {
        *out = element.get();
    }
    return element;
}

std::unique_ptr<ui::Button> make_debug_button(
    const std::string& text,
    ui::Button::ClickCallback callback,
    float min_width = 38.0f
) {
    auto button = std::make_unique<ui::Button>(text);
    button->setBackgroundColour(ui::Colour{54, 88, 128, 255});
    button->setHoverColour(ui::Colour{72, 108, 152, 255});
    button->setPressedColour(ui::Colour{38, 68, 102, 255});
    button->setTextColour(ui::Colour::white());
    button->setBorderColour(ui::Colour{28, 52, 78, 255});
    button->setBorderThickness(1.0f);
    button->setFontSize(14.0f);
    button->setPadding(ui::Thickness(8.0f, 4.0f));
    button->setOnClick(std::move(callback));

    ui::SizeConstraints constraints;
    constraints.min_width = min_width;
    constraints.min_height = 30.0f;
    button->setSizeConstraints(constraints);
    return button;
}

std::unique_ptr<ui::StackPanel> make_control_row(
    std::unique_ptr<ui::TextBlock> label,
    ui::Button::ClickCallback decrease,
    ui::Button::ClickCallback increase
) {
    auto row = std::make_unique<ui::StackPanel>(ui::Orientation::Horizontal);
    row->setSpacing(4.0f);

    ui::SizeConstraints label_constraints;
    label_constraints.preferred_width = 210.0f;
    label_constraints.min_width = 210.0f;
    label->setSizeConstraints(label_constraints);

    row->addChild(std::move(label));
    row->addChild(make_debug_button("-", std::move(decrease)));
    row->addChild(make_debug_button("+", std::move(increase)));
    return row;
}

} // namespace

std::unique_ptr<ui::UIElement> GreaterRealmDebugPanel::build(
    procgen::GreaterRealmGeneratorSettings& settings,
    const procgen::GreaterRealmMap& map,
    RegenerateCallback on_regenerate,
    PaintConstraintCallback on_paint_constraint,
    ClearConstraintsCallback on_clear_constraints
) {
    m_settings = &settings;
    m_on_regenerate = std::move(on_regenerate);
    m_on_paint_constraint = std::move(on_paint_constraint);
    m_on_clear_constraints = std::move(on_clear_constraints);

    auto root = std::make_unique<ui::StackPanel>(ui::Orientation::Vertical);
    root->setPadding(ui::Thickness(10.0f));
    root->setSpacing(5.0f);
    root->setBackgroundColour(ui::Colour{238, 242, 238, 240});

    ui::SizeConstraints root_constraints;
    root_constraints.preferred_width = 430.0f;
    root_constraints.min_width = 430.0f;
    root->setSizeConstraints(root_constraints);

    auto title = make_text("Greater Realm Debug");
    title->setFontSize(20.0f);
    title->setColour(ui::Colour{10, 18, 24, 255});
    root->addChild(std::move(title));

    const auto adjust = [this](
        float procgen::GreaterRealmGeneratorSettings::* member,
        float amount,
        float minimum,
        float maximum
    ) {
        return [this, member, amount, minimum, maximum]() {
            if (!m_settings) {
                return;
            }
            auto& value = m_settings->*member;
            value = std::clamp(value + amount, minimum, maximum);
            regenerate();
        };
    };

    root->addChild(make_control_row(
        make_text(seed_text(settings.seed), &m_seed_text),
        [this]() {
            if (m_settings) {
                if (m_settings->seed > 0) {
                    --m_settings->seed;
                }
                regenerate();
            }
        },
        [this]() {
            if (m_settings) {
                ++m_settings->seed;
                regenerate();
            }
        }
    ));

    const auto add_setting_row = [&root, &adjust](
        const char* label,
        float value,
        ui::TextBlock** text,
        float procgen::GreaterRealmGeneratorSettings::* member,
        float step,
        float minimum,
        float maximum
    ) {
        root->addChild(make_control_row(
            make_text(setting_text(label, value), text),
            adjust(member, -step, minimum, maximum),
            adjust(member, step, minimum, maximum)
        ));
    };

    add_setting_row("Island bias", settings.island_bias, &m_island_bias_text, &procgen::GreaterRealmGeneratorSettings::island_bias, 0.05f, 0.0f, 1.0f);
    add_setting_row("Coast detail", settings.coastline_noise_weight, &m_coastline_noise_text, &procgen::GreaterRealmGeneratorSettings::coastline_noise_weight, 0.01f, 0.0f, 0.40f);
    add_setting_row("Base relief", settings.base_elevation_weight, &m_base_elevation_text, &procgen::GreaterRealmGeneratorSettings::base_elevation_weight, 0.05f, 0.0f, 2.0f);
    add_setting_row("Sea", settings.sea_level, &m_sea_text, &procgen::GreaterRealmGeneratorSettings::sea_level, 0.02f, 0.10f, 0.90f);
    add_setting_row("Mountain strength", settings.mountain_weight, &m_mountain_text, &procgen::GreaterRealmGeneratorSettings::mountain_weight, 0.05f, 0.0f, 1.5f);
    add_setting_row("Peak spacing", settings.mountain_peak_spacing, &m_peak_spacing_text, &procgen::GreaterRealmGeneratorSettings::mountain_peak_spacing, 4.0f, 8.0f, 80.0f);
    add_setting_row("Peak radius", settings.mountain_peak_radius, &m_peak_radius_text, &procgen::GreaterRealmGeneratorSettings::mountain_peak_radius, 4.0f, 4.0f, 100.0f);
    add_setting_row("Peak jaggedness", settings.mountain_peak_jaggedness, &m_peak_jaggedness_text, &procgen::GreaterRealmGeneratorSettings::mountain_peak_jaggedness, 0.10f, 0.0f, 1.0f);
    add_setting_row("Ridge", settings.ridge_weight, &m_ridge_text, &procgen::GreaterRealmGeneratorSettings::ridge_weight, 0.05f, 0.0f, 1.5f);
    add_setting_row("Valley", settings.valley_weight, &m_valley_text, &procgen::GreaterRealmGeneratorSettings::valley_weight, 0.05f, 0.0f, 1.5f);
    add_setting_row("Terrain noise", settings.terrain_noise_weight, &m_noise_text, &procgen::GreaterRealmGeneratorSettings::terrain_noise_weight, 0.10f, 0.0f, 2.0f);
    add_setting_row("Ocean depth", settings.ocean_depth_weight, &m_ocean_depth_text, &procgen::GreaterRealmGeneratorSettings::ocean_depth_weight, 0.25f, 0.0f, 3.0f);
    add_setting_row("Wind angle", settings.wind_angle_degrees, &m_wind_angle_text, &procgen::GreaterRealmGeneratorSettings::wind_angle_degrees, 15.0f, 0.0f, 360.0f);
    add_setting_row("Raininess", settings.raininess, &m_raininess_text, &procgen::GreaterRealmGeneratorSettings::raininess, 0.10f, 0.0f, 2.0f);
    add_setting_row("Rain shadow", settings.rain_shadow, &m_rain_shadow_text, &procgen::GreaterRealmGeneratorSettings::rain_shadow, 0.10f, 0.0f, 2.0f);
    add_setting_row("Evaporation", settings.evaporation, &m_evaporation_text, &procgen::GreaterRealmGeneratorSettings::evaporation, 0.10f, 0.0f, 1.0f);
    add_setting_row("River flow", settings.river_flow_scale, &m_river_flow_text, &procgen::GreaterRealmGeneratorSettings::river_flow_scale, 0.05f, 0.0f, 1.0f);
    add_setting_row("River threshold", settings.river_min_flow, &m_river_threshold_text, &procgen::GreaterRealmGeneratorSettings::river_min_flow, 2.0f, 0.0f, 100.0f);

    const auto add_constraint_coordinate = [this, &root](
        const char* label,
        float* value,
        ui::TextBlock** text
    ) {
        root->addChild(make_control_row(
            make_text(setting_text(label, *value), text),
            [value, text, label]() {
                *value = std::clamp(*value - 0.05f, 0.0f, 1.0f);
                if (*text) {
                    (*text)->setText(setting_text(label, *value));
                }
            },
            [value, text, label]() {
                *value = std::clamp(*value + 0.05f, 0.0f, 1.0f);
                if (*text) {
                    (*text)->setText(setting_text(label, *value));
                }
            }
        ));
    };
    add_constraint_coordinate("Constraint X", &m_constraint_x, &m_constraint_x_text);
    add_constraint_coordinate("Constraint Y", &m_constraint_y, &m_constraint_y_text);

    const auto paint = [this](procgen::TerrainConstraintTool tool) {
        return [this, tool]() {
            if (m_on_paint_constraint) {
                m_on_paint_constraint(tool, m_constraint_x, m_constraint_y);
            }
        };
    };
    auto constraint_row_one = std::make_unique<ui::StackPanel>(ui::Orientation::Horizontal);
    constraint_row_one->setSpacing(6.0f);
    constraint_row_one->addChild(make_debug_button("Ocean", paint(procgen::TerrainConstraintTool::Ocean), 96.0f));
    constraint_row_one->addChild(make_debug_button("Shallow", paint(procgen::TerrainConstraintTool::ShallowWater), 96.0f));
    constraint_row_one->addChild(make_debug_button("Valley", paint(procgen::TerrainConstraintTool::Valley), 96.0f));
    constraint_row_one->addChild(make_debug_button("Mountain", paint(procgen::TerrainConstraintTool::Mountain), 96.0f));
    root->addChild(std::move(constraint_row_one));

    auto constraint_row_two = std::make_unique<ui::StackPanel>(ui::Orientation::Horizontal);
    constraint_row_two->setSpacing(6.0f);
    constraint_row_two->addChild(make_debug_button("Clear constraints", [this]() {
        if (m_on_clear_constraints) {
            m_on_clear_constraints();
        }
    }, 180.0f));
    root->addChild(std::move(constraint_row_two));

    auto button_row = std::make_unique<ui::StackPanel>(ui::Orientation::Horizontal);
    button_row->setSpacing(6.0f);
    button_row->addChild(make_debug_button("Regenerate", [this]() { regenerate(); }));
    button_row->addChild(make_debug_button("Random Seed", [this]() {
        if (m_settings) {
            m_settings->seed += 101;
            regenerate();
        }
    }));
    root->addChild(std::move(button_row));

    ui::SizeConstraints summary_constraints;
    summary_constraints.preferred_width = 410.0f;
    summary_constraints.min_width = 410.0f;

    auto coverage = make_text("", &m_coverage_text);
    coverage->setColour(ui::Colour{38, 44, 48, 255});
    coverage->setSizeConstraints(summary_constraints);
    root->addChild(std::move(coverage));

    auto terrain = make_text("", &m_terrain_text);
    terrain->setColour(ui::Colour{38, 44, 48, 255});
    terrain->setSizeConstraints(summary_constraints);
    root->addChild(std::move(terrain));

    auto hydrology = make_text("", &m_hydrology_text);
    hydrology->setColour(ui::Colour{38, 44, 48, 255});
    hydrology->setSizeConstraints(summary_constraints);
    root->addChild(std::move(hydrology));

    update(map);
    return root;
}

void GreaterRealmDebugPanel::update(const procgen::GreaterRealmMap& map) {
    if (!m_settings) {
        return;
    }

    if (m_seed_text) m_seed_text->setText(seed_text(m_settings->seed));
    if (m_sea_text) m_sea_text->setText(setting_text("Sea", m_settings->sea_level));
    if (m_island_bias_text) m_island_bias_text->setText(setting_text("Island bias", m_settings->island_bias));
    if (m_coastline_noise_text) m_coastline_noise_text->setText(setting_text("Coast detail", m_settings->coastline_noise_weight));
    if (m_base_elevation_text) m_base_elevation_text->setText(setting_text("Base relief", m_settings->base_elevation_weight));
    if (m_mountain_text) m_mountain_text->setText(setting_text("Mountain strength", m_settings->mountain_weight));
    if (m_peak_spacing_text) m_peak_spacing_text->setText(setting_text("Peak spacing", m_settings->mountain_peak_spacing));
    if (m_peak_radius_text) m_peak_radius_text->setText(setting_text("Peak radius", m_settings->mountain_peak_radius));
    if (m_peak_jaggedness_text) m_peak_jaggedness_text->setText(setting_text("Peak jaggedness", m_settings->mountain_peak_jaggedness));
    if (m_ridge_text) m_ridge_text->setText(setting_text("Ridge", m_settings->ridge_weight));
    if (m_valley_text) m_valley_text->setText(setting_text("Valley", m_settings->valley_weight));
    if (m_noise_text) m_noise_text->setText(setting_text("Terrain noise", m_settings->terrain_noise_weight));
    if (m_ocean_depth_text) m_ocean_depth_text->setText(setting_text("Ocean depth", m_settings->ocean_depth_weight));
    if (m_wind_angle_text) m_wind_angle_text->setText(setting_text("Wind angle", m_settings->wind_angle_degrees));
    if (m_raininess_text) m_raininess_text->setText(setting_text("Raininess", m_settings->raininess));
    if (m_rain_shadow_text) m_rain_shadow_text->setText(setting_text("Rain shadow", m_settings->rain_shadow));
    if (m_evaporation_text) m_evaporation_text->setText(setting_text("Evaporation", m_settings->evaporation));
    if (m_river_flow_text) m_river_flow_text->setText(setting_text("River flow", m_settings->river_flow_scale));
    if (m_river_threshold_text) m_river_threshold_text->setText(setting_text("River threshold", m_settings->river_min_flow));
    if (m_constraint_x_text) m_constraint_x_text->setText(setting_text("Constraint X", m_constraint_x));
    if (m_constraint_y_text) m_constraint_y_text->setText(setting_text("Constraint Y", m_constraint_y));

    if (m_coverage_text || m_terrain_text || m_hydrology_text) {
        const auto counts = procgen::count_terrain_forms(map);
        if (m_coverage_text) m_coverage_text->setText(coverage_text(map, counts));
        if (m_terrain_text) m_terrain_text->setText(terrain_text(counts));
        if (m_hydrology_text) m_hydrology_text->setText(hydrology_text(map));
    }
}

void GreaterRealmDebugPanel::regenerate() {
    if (m_on_regenerate) {
        m_on_regenerate();
    }
}

} // namespace game
