#pragma once

#include "../../include/procgen/GreaterRealmDebug.hpp"
#include "../../include/world/Weather.hpp"
#include <cstdint>

namespace game {

enum class GreaterRealmInspectionView : std::uint8_t {
    Terrain,
    TerrainForms,
    Elevation,
    Landmass,
    HillRelief,
    MountainRelief,
    MountainInfluence,
    Slope,
    CoastDistance,
    CatchmentArea,
    AnnualTemperature,
    AnnualPrecipitation,
    Biome,
    SeasonalTemperature,
    SeasonalPrecipitation,
    Count
};

struct GreaterRealmInspectionSettings {
    GreaterRealmInspectionView view{GreaterRealmInspectionView::Terrain};
    float year_fraction{0.0f};
};

[[nodiscard]] const char* to_string(GreaterRealmInspectionView view) noexcept;
[[nodiscard]] bool procgen_debug_view_for(
    GreaterRealmInspectionView inspection_view,
    procgen::GreaterRealmDebugView& debug_view
) noexcept;
[[nodiscard]] procgen::DebugImage build_greater_realm_climate_weather_inspection_image(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const world::SeasonalTemperatureMap& seasonal_temperature,
    const world::SeasonalPrecipitationMap& seasonal_precipitation,
    GreaterRealmInspectionView view,
    const procgen::GreaterRealmDebugOptions& overlay_options
);

} // namespace game
