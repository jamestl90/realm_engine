#include "GreaterRealmClimateWeatherInspection.hpp"

namespace game {

const char* to_string(GreaterRealmInspectionView view) noexcept {
    switch (view) {
        case GreaterRealmInspectionView::Terrain: return "Terrain";
        case GreaterRealmInspectionView::TerrainForms: return "Terrain forms";
        case GreaterRealmInspectionView::Elevation: return "Elevation";
        case GreaterRealmInspectionView::Landmass: return "Landmass";
        case GreaterRealmInspectionView::HillRelief: return "Hill relief";
        case GreaterRealmInspectionView::MountainRelief: return "Mountain relief";
        case GreaterRealmInspectionView::MountainInfluence: return "Mountain influence";
        case GreaterRealmInspectionView::Slope: return "Slope";
        case GreaterRealmInspectionView::CoastDistance: return "Coast distance";
        case GreaterRealmInspectionView::CatchmentArea: return "Catchment area";
        case GreaterRealmInspectionView::AnnualTemperature: return "Annual temperature";
        case GreaterRealmInspectionView::AnnualPrecipitation: return "Annual precipitation";
        case GreaterRealmInspectionView::Biome: return "Biome";
        case GreaterRealmInspectionView::SeasonalTemperature: return "Seasonal temperature";
        case GreaterRealmInspectionView::SeasonalPrecipitation: return "Seasonal precipitation";
        case GreaterRealmInspectionView::TemperatureAnomaly: return "Temperature anomaly";
        case GreaterRealmInspectionView::Pressure: return "Pressure";
        case GreaterRealmInspectionView::RuntimeWind: return "Runtime wind";
        case GreaterRealmInspectionView::Humidity: return "Humidity";
        case GreaterRealmInspectionView::CloudCover: return "Cloud cover";
        case GreaterRealmInspectionView::ActivePrecipitation: return "Active precipitation";
        case GreaterRealmInspectionView::ExperiencedTemperature: return "Experienced temperature";
        case GreaterRealmInspectionView::ExperiencedPrecipitation:
            return "Experienced precipitation";
        case GreaterRealmInspectionView::Count: break;
    }
    return "Unknown";
}

bool procgen_debug_view_for(
    GreaterRealmInspectionView inspection_view,
    procgen::GreaterRealmDebugView& debug_view
) noexcept {
    switch (inspection_view) {
        case GreaterRealmInspectionView::Terrain:
            debug_view = procgen::GreaterRealmDebugView::Terrain;
            return true;
        case GreaterRealmInspectionView::TerrainForms:
            debug_view = procgen::GreaterRealmDebugView::TerrainForms;
            return true;
        case GreaterRealmInspectionView::Elevation:
            debug_view = procgen::GreaterRealmDebugView::Elevation;
            return true;
        case GreaterRealmInspectionView::Landmass:
            debug_view = procgen::GreaterRealmDebugView::Landmass;
            return true;
        case GreaterRealmInspectionView::HillRelief:
            debug_view = procgen::GreaterRealmDebugView::HillRelief;
            return true;
        case GreaterRealmInspectionView::MountainRelief:
            debug_view = procgen::GreaterRealmDebugView::MountainRelief;
            return true;
        case GreaterRealmInspectionView::MountainInfluence:
            debug_view = procgen::GreaterRealmDebugView::MountainInfluence;
            return true;
        case GreaterRealmInspectionView::Slope:
            debug_view = procgen::GreaterRealmDebugView::Slope;
            return true;
        case GreaterRealmInspectionView::CoastDistance:
            debug_view = procgen::GreaterRealmDebugView::CoastDistance;
            return true;
        case GreaterRealmInspectionView::CatchmentArea:
            debug_view = procgen::GreaterRealmDebugView::CatchmentArea;
            return true;
        case GreaterRealmInspectionView::AnnualTemperature:
            debug_view = procgen::GreaterRealmDebugView::TemperatureNormal;
            return true;
        case GreaterRealmInspectionView::AnnualPrecipitation:
            debug_view = procgen::GreaterRealmDebugView::PrecipitationNormal;
            return true;
        case GreaterRealmInspectionView::Biome:
            debug_view = procgen::GreaterRealmDebugView::Biome;
            return true;
        default:
            return false;
    }
}

} // namespace game
