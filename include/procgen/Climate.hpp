#pragma once

#include "GreaterRealm.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace procgen {

inline constexpr std::uint32_t GREATER_REALM_CLIMATE_VERSION = 1;

struct GreaterRealmClimateSettings {
    float north_edge_latitude_degrees{60.0f};
    float south_edge_latitude_degrees{-60.0f};
    float elevation_cooling{0.35f};
    float maritime_moderation{0.20f};
    float maritime_influence_distance{16.0f};
    float temperature_variation{0.08f};
    float temperature_variation_frequency{2.5f};

    [[nodiscard]] bool operator==(const GreaterRealmClimateSettings&) const noexcept = default;
};

struct GreaterRealmClimateCell {
    float temperature_normal{0.5f};
};

struct GreaterRealmClimateMap {
    std::uint32_t version{GREATER_REALM_CLIMATE_VERSION};
    Seed source_seed{1};
    std::uint32_t source_width{0};
    std::uint32_t source_height{0};
    float source_cell_size{1.0f};
    std::uint64_t source_terrain_fingerprint{0};
    std::vector<GreaterRealmClimateCell> cells;

    [[nodiscard]] std::size_t expected_cell_count() const noexcept;
    [[nodiscard]] bool has_expected_cell_count() const noexcept;
    [[nodiscard]] bool source_matches(const GreaterRealmMap& map) const noexcept;
};

struct TemperatureNormalSummary {
    float minimum{0.0f};
    float maximum{0.0f};
    float mean{0.0f};
    std::size_t sample_count{0};
};

enum class GreaterRealmClimateDirtyStage : std::uint8_t {
    None = 0,
    Temperature = 1u << 0
};

[[nodiscard]] constexpr GreaterRealmClimateDirtyStage operator|(
    GreaterRealmClimateDirtyStage left,
    GreaterRealmClimateDirtyStage right
) noexcept {
    return static_cast<GreaterRealmClimateDirtyStage>(
        static_cast<std::uint8_t>(left) | static_cast<std::uint8_t>(right)
    );
}

[[nodiscard]] constexpr GreaterRealmClimateDirtyStage operator&(
    GreaterRealmClimateDirtyStage left,
    GreaterRealmClimateDirtyStage right
) noexcept {
    return static_cast<GreaterRealmClimateDirtyStage>(
        static_cast<std::uint8_t>(left) & static_cast<std::uint8_t>(right)
    );
}

[[nodiscard]] constexpr bool has_climate_dirty_stage(
    GreaterRealmClimateDirtyStage stages,
    GreaterRealmClimateDirtyStage stage
) noexcept {
    return (stages & stage) != GreaterRealmClimateDirtyStage::None;
}

struct GreaterRealmClimateRegenerationResult {
    GreaterRealmClimateDirtyStage rebuilt_stages{GreaterRealmClimateDirtyStage::None};

    [[nodiscard]] bool rebuilt(GreaterRealmClimateDirtyStage stage) const noexcept {
        return has_climate_dirty_stage(rebuilt_stages, stage);
    }
};

class GreaterRealmClimateGenerationCache {
public:
    void invalidate() noexcept;
    void reset() noexcept;

    [[nodiscard]] bool initialized() const noexcept { return m_initialized; }
    [[nodiscard]] GreaterRealmClimateRegenerationResult regenerate(
        GreaterRealmClimateMap& climate,
        const GreaterRealmMap& terrain,
        const GreaterRealmClimateSettings& settings
    );

private:
    GreaterRealmClimateSettings m_settings;
    bool m_pending_temperature{true};
    bool m_initialized{false};
};

[[nodiscard]] GreaterRealmClimateSettings clamp_greater_realm_climate_settings(
    const GreaterRealmClimateSettings& settings
) noexcept;
[[nodiscard]] float greater_realm_latitude_for_row(
    const GreaterRealmClimateSettings& settings,
    std::uint32_t row,
    std::uint32_t height
) noexcept;
[[nodiscard]] std::uint64_t greater_realm_climate_source_fingerprint(
    const GreaterRealmMap& map
) noexcept;
[[nodiscard]] GreaterRealmClimateMap generate_greater_realm_climate(
    const GreaterRealmMap& terrain,
    const GreaterRealmClimateSettings& settings = {}
);
[[nodiscard]] TemperatureNormalSummary summarize_temperature_normals(
    const GreaterRealmClimateMap& climate
) noexcept;

} // namespace procgen
