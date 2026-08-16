#include "procgen/GreaterRealmDebug.hpp"

namespace procgen {

TerrainFormCounts count_terrain_forms(const GreaterRealmMap& map) noexcept {
    TerrainFormCounts counts;
    for (const auto& cell : map.cells) {
        if (cell.is_coastal) {
            ++counts.coastal_land;
        }
        switch (cell.terrain_form) {
            case TerrainForm::Ocean:
                ++counts.ocean;
                break;
            case TerrainForm::Plains:
                ++counts.plains;
                break;
            case TerrainForm::Hills:
                ++counts.hills;
                break;
            case TerrainForm::Highlands:
                ++counts.highlands;
                break;
            case TerrainForm::Mountains:
                ++counts.mountains;
                break;
        }
    }
    return counts;
}

const char* to_string(GreaterRealmDebugView view) noexcept {
    switch (view) {
        case GreaterRealmDebugView::Terrain: return "Terrain";
        case GreaterRealmDebugView::TerrainForms: return "Terrain forms";
        case GreaterRealmDebugView::Elevation: return "Elevation";
        case GreaterRealmDebugView::Landmass: return "Landmass";
        case GreaterRealmDebugView::HillRelief: return "Hill relief";
        case GreaterRealmDebugView::MountainRelief: return "Mountain relief";
        case GreaterRealmDebugView::MountainInfluence: return "Mountain influence";
        case GreaterRealmDebugView::Slope: return "Slope";
        case GreaterRealmDebugView::CoastDistance: return "Coast distance";
        case GreaterRealmDebugView::CatchmentArea: return "Catchment area";
        case GreaterRealmDebugView::Count: break;
    }
    return "Unknown";
}

} // namespace procgen
