#include "GreaterRealmDebugBiomes.hpp"

namespace game {

procgen::GreaterRealmBiomeRuleSet make_greater_realm_debug_biome_rules() {
    procgen::GreaterRealmBiomeRuleSet rules;
    rules.identity = 2;
    rules.fallback_biome_id = GrasslandBiome;

    procgen::GreaterRealmBiomeRule ocean;
    ocean.biome_id = OceanBiome;
    ocean.priority = 100;
    ocean.water_class = procgen::BiomeWaterClass::Ocean;
    rules.rules.push_back(ocean);

    procgen::GreaterRealmBiomeRule inland_water;
    inland_water.biome_id = InlandWaterBiome;
    inland_water.priority = 100;
    inland_water.water_class = procgen::BiomeWaterClass::InlandWater;
    rules.rules.push_back(inland_water);

    procgen::GreaterRealmBiomeRule alpine;
    alpine.biome_id = AlpineBiome;
    alpine.priority = 90;
    alpine.water_class = procgen::BiomeWaterClass::Land;
    alpine.elevation = procgen::BiomeValueRange{0.70f, 1.0f};
    rules.rules.push_back(alpine);

    procgen::GreaterRealmBiomeRule polar;
    polar.biome_id = PolarBiome;
    polar.priority = 80;
    polar.water_class = procgen::BiomeWaterClass::Land;
    polar.temperature_normal = procgen::BiomeValueRange{0.0f, 0.38f};
    rules.rules.push_back(polar);

    procgen::GreaterRealmBiomeRule tundra;
    tundra.biome_id = TundraBiome;
    tundra.priority = 70;
    tundra.water_class = procgen::BiomeWaterClass::Land;
    tundra.temperature_normal = procgen::BiomeValueRange{0.38f, 0.52f};
    rules.rules.push_back(tundra);

    procgen::GreaterRealmBiomeRule rainforest;
    rainforest.biome_id = RainforestBiome;
    rainforest.priority = 65;
    rainforest.water_class = procgen::BiomeWaterClass::Land;
    rainforest.temperature_normal = procgen::BiomeValueRange{0.65f, 1.0f};
    rainforest.precipitation_normal = procgen::BiomeValueRange{0.28f, 1.0f};
    rules.rules.push_back(rainforest);

    procgen::GreaterRealmBiomeRule desert;
    desert.biome_id = DesertBiome;
    desert.priority = 60;
    desert.water_class = procgen::BiomeWaterClass::Land;
    desert.temperature_normal = procgen::BiomeValueRange{0.55f, 1.0f};
    desert.precipitation_normal = procgen::BiomeValueRange{0.0f, 0.04f};
    rules.rules.push_back(desert);

    procgen::GreaterRealmBiomeRule forest;
    forest.biome_id = ForestBiome;
    forest.priority = 50;
    forest.water_class = procgen::BiomeWaterClass::Land;
    forest.precipitation_normal = procgen::BiomeValueRange{0.12f, 1.0f};
    rules.rules.push_back(forest);
    return rules;
}

std::vector<procgen::BiomeDebugColour> make_greater_realm_debug_biome_colours() {
    return {
        {OceanBiome, {28, 82, 154, 255}},
        {InlandWaterBiome, {38, 142, 154, 255}},
        {AlpineBiome, {196, 202, 204, 255}},
        {PolarBiome, {224, 236, 238, 255}},
        {RainforestBiome, {30, 104, 66, 255}},
        {DesertBiome, {194, 148, 76, 255}},
        {ForestBiome, {54, 126, 70, 255}},
        {TundraBiome, {126, 144, 122, 255}},
        {GrasslandBiome, {112, 166, 76, 255}}
    };
}

} // namespace game
