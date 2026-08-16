#pragma once

#include "../../include/procgen/Biome.hpp"
#include "../../include/procgen/GreaterRealmDebug.hpp"
#include <vector>

namespace game {

enum GreaterRealmDebugBiomeId : procgen::BiomeId {
    OceanBiome = 1,
    InlandWaterBiome = 2,
    AlpineBiome = 3,
    PolarBiome = 4,
    RainforestBiome = 5,
    DesertBiome = 6,
    ForestBiome = 7,
    TundraBiome = 8,
    GrasslandBiome = 9
};

[[nodiscard]] procgen::GreaterRealmBiomeRuleSet make_greater_realm_debug_biome_rules();
[[nodiscard]] std::vector<procgen::BiomeDebugColour> make_greater_realm_debug_biome_colours();

} // namespace game
