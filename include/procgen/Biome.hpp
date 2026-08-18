#pragma once

#include "Climate.hpp"
#include "GreaterRealm.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace procgen {

using BiomeId = std::uint32_t;
inline constexpr BiomeId INVALID_BIOME_ID = std::numeric_limits<BiomeId>::max();
inline constexpr std::uint32_t GREATER_REALM_BIOME_VERSION = 1;

struct BiomeValueRange {
    float minimum{0.0f};
    float maximum{1.0f};

    [[nodiscard]] bool contains(float value) const noexcept {
        return value >= minimum && value <= maximum;
    }
};

enum class BiomeWaterClass : std::uint8_t {
    Any,
    Land,
    AnyWater,
    Ocean,
    InlandWater
};

struct GreaterRealmBiomeRule {
    BiomeId biome_id{INVALID_BIOME_ID};
    std::int32_t priority{0};
    std::optional<TerrainForm> terrain_form;
    BiomeWaterClass water_class{BiomeWaterClass::Any};
    std::optional<BiomeValueRange> elevation;
    std::optional<BiomeValueRange> slope;
    std::optional<BiomeValueRange> coast_distance;
    std::optional<BiomeValueRange> temperature_normal;
    std::optional<BiomeValueRange> precipitation_normal;
};

struct GreaterRealmBiomeRuleSet {
    std::uint32_t version{1};
    std::uint64_t identity{1};
    std::optional<BiomeId> fallback_biome_id;
    std::vector<GreaterRealmBiomeRule> rules;
};

enum class BiomeRuleValidationError : std::uint8_t {
    None,
    InvalidRuleSetVersion,
    InvalidBiomeId,
    DuplicateBiomeId,
    InvalidTerrainForm,
    InvalidWaterClass,
    InvalidRange
};

struct BiomeRuleValidationResult {
    BiomeRuleValidationError error{BiomeRuleValidationError::None};
    std::size_t rule_index{0};

    [[nodiscard]] bool valid() const noexcept {
        return error == BiomeRuleValidationError::None;
    }
};

struct GreaterRealmBiomeCell {
    BiomeId biome_id{INVALID_BIOME_ID};
};

struct GreaterRealmBiomeMap {
    std::uint32_t version{GREATER_REALM_BIOME_VERSION};
    Seed source_seed{1};
    std::uint32_t source_width{0};
    std::uint32_t source_height{0};
    float source_cell_size{1.0f};
    std::uint64_t source_terrain_fingerprint{0};
    std::uint64_t source_climate_fingerprint{0};
    std::uint64_t source_rule_set_fingerprint{0};
    std::vector<GreaterRealmBiomeCell> cells;

    [[nodiscard]] std::size_t expected_cell_count() const noexcept;
    [[nodiscard]] bool has_expected_cell_count() const noexcept;
    [[nodiscard]] bool source_maps_match(
        const GreaterRealmMap& terrain,
        const GreaterRealmClimateMap& climate
    ) const noexcept;
    [[nodiscard]] bool sources_match(
        const GreaterRealmMap& terrain,
        const GreaterRealmClimateMap& climate,
        const GreaterRealmBiomeRuleSet& rules
    ) const noexcept;
};

enum class GreaterRealmBiomeDirtyStage : std::uint8_t {
    None,
    Assignment
};

struct GreaterRealmBiomeRegenerationResult {
    GreaterRealmBiomeDirtyStage rebuilt_stages{GreaterRealmBiomeDirtyStage::None};

    [[nodiscard]] bool rebuilt_assignment() const noexcept {
        return rebuilt_stages == GreaterRealmBiomeDirtyStage::Assignment;
    }
};

class GreaterRealmBiomeGenerationCache {
public:
    void invalidate() noexcept;
    void reset() noexcept;

    [[nodiscard]] GreaterRealmBiomeRegenerationResult regenerate(
        GreaterRealmBiomeMap& biomes,
        const GreaterRealmMap& terrain,
        const GreaterRealmClimateMap& climate,
        const GreaterRealmBiomeRuleSet& rules
    );

private:
    bool m_pending_assignment{true};
    bool m_initialized{false};
};

[[nodiscard]] BiomeRuleValidationResult validate_greater_realm_biome_rules(
    const GreaterRealmBiomeRuleSet& rules
) noexcept;
[[nodiscard]] std::uint64_t greater_realm_climate_fingerprint(
    const GreaterRealmClimateMap& climate
) noexcept;
[[nodiscard]] std::uint64_t greater_realm_biome_rule_set_fingerprint(
    const GreaterRealmBiomeRuleSet& rules
) noexcept;
[[nodiscard]] GreaterRealmBiomeMap generate_greater_realm_biomes(
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateMap& climate,
    const GreaterRealmBiomeRuleSet& rules
);

} // namespace procgen
