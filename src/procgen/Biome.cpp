#include "../../include/procgen/Biome.hpp"
#include "../../include/procgen/detail/GenerationUtility.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <unordered_set>

namespace procgen {
namespace {

using detail::mix_hash;

[[nodiscard]] bool valid_terrain_form(TerrainForm form) noexcept {
    switch (form) {
        case TerrainForm::Ocean:
        case TerrainForm::InlandWater:
        case TerrainForm::Plains:
        case TerrainForm::Hills:
        case TerrainForm::Highlands:
        case TerrainForm::Mountains:
            return true;
    }
    return false;
}

[[nodiscard]] bool valid_water_class(BiomeWaterClass water_class) noexcept {
    switch (water_class) {
        case BiomeWaterClass::Any:
        case BiomeWaterClass::Land:
        case BiomeWaterClass::AnyWater:
        case BiomeWaterClass::Ocean:
        case BiomeWaterClass::InlandWater:
            return true;
    }
    return false;
}

[[nodiscard]] bool valid_range(
    const std::optional<BiomeValueRange>& range,
    float allowed_minimum,
    float allowed_maximum
) noexcept {
    return !range || (
        std::isfinite(range->minimum)
        && std::isfinite(range->maximum)
        && range->minimum >= allowed_minimum
        && range->maximum <= allowed_maximum
        && range->minimum <= range->maximum
    );
}

[[nodiscard]] bool water_class_matches(
    const GreaterRealmCell& cell,
    BiomeWaterClass water_class
) noexcept {
    switch (water_class) {
        case BiomeWaterClass::Any: return true;
        case BiomeWaterClass::Land: return !cell.is_water;
        case BiomeWaterClass::AnyWater: return cell.is_water;
        case BiomeWaterClass::Ocean: return cell.is_water && cell.is_ocean;
        case BiomeWaterClass::InlandWater: return cell.is_water && !cell.is_ocean;
    }
    return false;
}

[[nodiscard]] bool range_matches(
    const std::optional<BiomeValueRange>& range,
    float value
) noexcept {
    return !range || range->contains(value);
}

[[nodiscard]] bool rule_matches(
    const GreaterRealmBiomeRule& rule,
    const GreaterRealmCell& terrain,
    const GreaterRealmClimateCell& climate
) noexcept {
    return (!rule.terrain_form || *rule.terrain_form == terrain.terrain_form)
        && water_class_matches(terrain, rule.water_class)
        && range_matches(rule.elevation, terrain.elevation)
        && range_matches(rule.slope, terrain.slope)
        && range_matches(rule.coast_distance, terrain.distance_to_coast)
        && range_matches(rule.temperature_normal, climate.temperature_normal)
        && range_matches(rule.precipitation_normal, climate.precipitation_normal);
}

void hash_optional_range(
    std::uint64_t& hash,
    const std::optional<BiomeValueRange>& range
) noexcept {
    hash = mix_hash(hash, range ? 1u : 0u);
    if (range) {
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(range->minimum));
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(range->maximum));
    }
}

} // namespace

std::size_t GreaterRealmBiomeMap::expected_cell_count() const noexcept {
    return static_cast<std::size_t>(source_width) * static_cast<std::size_t>(source_height);
}

bool GreaterRealmBiomeMap::has_expected_cell_count() const noexcept {
    return cells.size() == expected_cell_count();
}

bool GreaterRealmBiomeMap::source_maps_match(
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateMap& climate
) const noexcept {
    return version == GREATER_REALM_BIOME_VERSION
        && source_seed == terrain.seed
        && source_width == terrain.width
        && source_height == terrain.height
        && source_cell_size == terrain.cell_size
        && has_expected_cell_count()
        && climate.source_matches(terrain)
        && source_terrain_fingerprint == greater_realm_climate_source_fingerprint(terrain)
        && source_climate_fingerprint == greater_realm_climate_fingerprint(climate);
}

bool GreaterRealmBiomeMap::sources_match(
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateMap& climate,
    const GreaterRealmBiomeRuleSet& rules
) const noexcept {
    return source_maps_match(terrain, climate)
        && validate_greater_realm_biome_rules(rules).valid()
        && source_rule_set_fingerprint == greater_realm_biome_rule_set_fingerprint(rules);
}

BiomeRuleValidationResult validate_greater_realm_biome_rules(
    const GreaterRealmBiomeRuleSet& rules
) noexcept {
    if (rules.version == 0) {
        return {BiomeRuleValidationError::InvalidRuleSetVersion, 0};
    }
    if (rules.fallback_biome_id && *rules.fallback_biome_id == INVALID_BIOME_ID) {
        return {BiomeRuleValidationError::InvalidBiomeId, rules.rules.size()};
    }

    std::unordered_set<BiomeId> ids;
    ids.reserve(rules.rules.size());
    for (std::size_t index = 0; index < rules.rules.size(); ++index) {
        const auto& rule = rules.rules[index];
        if (rule.biome_id == INVALID_BIOME_ID) {
            return {BiomeRuleValidationError::InvalidBiomeId, index};
        }
        if (!ids.insert(rule.biome_id).second) {
            return {BiomeRuleValidationError::DuplicateBiomeId, index};
        }
        if (rule.terrain_form && !valid_terrain_form(*rule.terrain_form)) {
            return {BiomeRuleValidationError::InvalidTerrainForm, index};
        }
        if (!valid_water_class(rule.water_class)) {
            return {BiomeRuleValidationError::InvalidWaterClass, index};
        }
        if (!valid_range(rule.elevation, 0.0f, 1.0f)
            || !valid_range(rule.slope, 0.0f, std::numeric_limits<float>::max())
            || !valid_range(rule.coast_distance, 0.0f, std::numeric_limits<float>::max())
            || !valid_range(rule.temperature_normal, 0.0f, 1.0f)
            || !valid_range(rule.precipitation_normal, 0.0f, 1.0f)) {
            return {BiomeRuleValidationError::InvalidRange, index};
        }
    }
    return {};
}

std::uint64_t greater_realm_climate_fingerprint(
    const GreaterRealmClimateMap& climate
) noexcept {
    std::uint64_t hash = mix_hash(climate.version, climate.source_seed);
    hash = mix_hash(hash, climate.source_width);
    hash = mix_hash(hash, climate.source_height);
    hash = mix_hash(hash, std::bit_cast<std::uint32_t>(climate.source_cell_size));
    hash = mix_hash(hash, climate.source_terrain_fingerprint);
    hash = mix_hash(
        hash,
        std::bit_cast<std::uint32_t>(climate.precipitation_character.wetness_scale)
    );
    hash = mix_hash(
        hash,
        std::bit_cast<std::uint32_t>(climate.precipitation_character.retention_scale)
    );
    hash = mix_hash(
        hash,
        std::bit_cast<std::uint32_t>(climate.wind_character.global_rotation_degrees)
    );
    hash = mix_hash(
        hash,
        std::bit_cast<std::uint32_t>(climate.wind_character.latitude_shift_degrees)
    );
    hash = mix_hash(
        hash,
        std::bit_cast<std::uint32_t>(climate.wind_character.north_angle_offset_degrees)
    );
    hash = mix_hash(
        hash,
        std::bit_cast<std::uint32_t>(climate.wind_character.south_angle_offset_degrees)
    );
    hash = mix_hash(
        hash,
        std::bit_cast<std::uint32_t>(climate.wind_character.regional_strength_scale)
    );
    for (const auto& cell : climate.cells) {
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(cell.temperature_normal));
        hash = mix_hash(hash, std::bit_cast<std::uint32_t>(cell.precipitation_normal));
    }
    return hash;
}

std::uint64_t greater_realm_biome_rule_set_fingerprint(
    const GreaterRealmBiomeRuleSet& rules
) noexcept {
    std::uint64_t hash = mix_hash(rules.version, rules.identity);
    hash = mix_hash(hash, rules.fallback_biome_id ? 1u : 0u);
    if (rules.fallback_biome_id) {
        hash = mix_hash(hash, *rules.fallback_biome_id);
    }
    for (const auto& rule : rules.rules) {
        hash = mix_hash(hash, rule.biome_id);
        hash = mix_hash(hash, static_cast<std::uint32_t>(rule.priority));
        hash = mix_hash(hash, rule.terrain_form ? 1u : 0u);
        if (rule.terrain_form) {
            hash = mix_hash(hash, static_cast<std::uint8_t>(*rule.terrain_form));
        }
        hash = mix_hash(hash, static_cast<std::uint8_t>(rule.water_class));
        hash_optional_range(hash, rule.elevation);
        hash_optional_range(hash, rule.slope);
        hash_optional_range(hash, rule.coast_distance);
        hash_optional_range(hash, rule.temperature_normal);
        hash_optional_range(hash, rule.precipitation_normal);
    }
    return hash;
}

GreaterRealmBiomeMap generate_greater_realm_biomes(
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateMap& climate,
    const GreaterRealmBiomeRuleSet& rules
) {
    GreaterRealmBiomeMap biomes;
    biomes.source_seed = terrain.seed;
    biomes.source_width = terrain.width;
    biomes.source_height = terrain.height;
    biomes.source_cell_size = terrain.cell_size;
    biomes.source_terrain_fingerprint = greater_realm_climate_source_fingerprint(terrain);
    biomes.source_climate_fingerprint = greater_realm_climate_fingerprint(climate);
    biomes.source_rule_set_fingerprint = greater_realm_biome_rule_set_fingerprint(rules);
    if (!terrain.has_expected_cell_count()
        || !climate.source_matches(terrain)
        || !validate_greater_realm_biome_rules(rules).valid()) {
        return biomes;
    }

    biomes.cells.resize(terrain.cells.size());
    for (std::size_t cell_index = 0; cell_index < terrain.cells.size(); ++cell_index) {
        BiomeId selected = rules.fallback_biome_id.value_or(INVALID_BIOME_ID);
        std::int32_t selected_priority = std::numeric_limits<std::int32_t>::min();
        bool selected_rule = false;
        for (const auto& rule : rules.rules) {
            if ((!selected_rule || rule.priority > selected_priority)
                && rule_matches(rule, terrain.cells[cell_index], climate.cells[cell_index])) {
                selected = rule.biome_id;
                selected_priority = rule.priority;
                selected_rule = true;
            }
        }
        biomes.cells[cell_index].biome_id = selected;
    }
    return biomes;
}

void GreaterRealmBiomeGenerationCache::invalidate() noexcept {
    m_pending_assignment = true;
}

void GreaterRealmBiomeGenerationCache::reset() noexcept {
    m_pending_assignment = true;
    m_initialized = false;
}

GreaterRealmBiomeRegenerationResult GreaterRealmBiomeGenerationCache::regenerate(
    GreaterRealmBiomeMap& biomes,
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateMap& climate,
    const GreaterRealmBiomeRuleSet& rules
) {
    if (!m_pending_assignment
        && m_initialized
        && biomes.sources_match(terrain, climate, rules)) {
        return {};
    }

    biomes = generate_greater_realm_biomes(terrain, climate, rules);
    m_pending_assignment = false;
    m_initialized = biomes.sources_match(terrain, climate, rules);
    return {GreaterRealmBiomeDirtyStage::Assignment};
}

} // namespace procgen
