#pragma once

#include "../../include/procgen/GreaterRealm.hpp"
#include <functional>
#include <memory>

namespace ui {
class TextBlock;
class UIElement;
}

namespace game {

class GreaterRealmDebugPanel {
public:
    using RegenerateCallback = std::function<void()>;

    [[nodiscard]] std::unique_ptr<ui::UIElement> build(
        procgen::GreaterRealmGeneratorSettings& settings,
        const procgen::GreaterRealmMap& map,
        RegenerateCallback on_regenerate
    );

    void update(const procgen::GreaterRealmMap& map);

private:
    void regenerate();

    procgen::GreaterRealmGeneratorSettings* m_settings{nullptr};
    RegenerateCallback m_on_regenerate;
    ui::TextBlock* m_seed_text{nullptr};
    ui::TextBlock* m_sea_text{nullptr};
    ui::TextBlock* m_land_shape_text{nullptr};
    ui::TextBlock* m_island_bias_text{nullptr};
    ui::TextBlock* m_coastline_noise_text{nullptr};
    ui::TextBlock* m_base_elevation_text{nullptr};
    ui::TextBlock* m_mountain_text{nullptr};
    ui::TextBlock* m_ridge_text{nullptr};
    ui::TextBlock* m_valley_text{nullptr};
    ui::TextBlock* m_noise_text{nullptr};
    ui::TextBlock* m_ocean_depth_text{nullptr};
    ui::TextBlock* m_coverage_text{nullptr};
    ui::TextBlock* m_terrain_text{nullptr};
};

} // namespace game
