#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace procgen {

using Seed = std::uint64_t;
inline constexpr std::uint32_t INVALID_CELL_INDEX = std::numeric_limits<std::uint32_t>::max();

class TerrainConstraintField;

enum class TerrainForm : std::uint8_t {
    Ocean,
    Plains,
    Hills,
    Highlands,
    Mountains
};

[[nodiscard]] const char* to_string(TerrainForm form) noexcept;
[[nodiscard]] bool is_water(TerrainForm form) noexcept;

struct GreaterRealmCell {
    std::int32_t x{0};
    std::int32_t y{0};
    float landmass_elevation{0.0f};
    float relief_constraint{0.0f};
    float hill_relief{0.0f};
    float mountain_relief{0.0f};
    float elevation{0.0f};
    bool is_water{false};
    bool is_ocean{false};
    bool is_coastal{false};
    float distance_to_coast{0.0f};
    float slope{0.0f};
    float mountain_distance{std::numeric_limits<float>::infinity()};
    float mountain_influence{0.0f};
    bool is_mountain_peak{false};
    float drainage_elevation{0.0f};
    float drainage_area{0.0f};
    std::uint32_t downslope_index{INVALID_CELL_INDEX};
    bool is_drainage_outlet{false};
    TerrainForm terrain_form{TerrainForm::Ocean};
};

struct GreaterRealmRiverSegment {
    std::uint32_t source_index{INVALID_CELL_INDEX};
    std::uint32_t destination_index{INVALID_CELL_INDEX};
    float drainage_area{0.0f};
    float width{0.0f};
};

struct GreaterRealmMountainPeak {
    std::uint32_t cell_index{INVALID_CELL_INDEX};
    std::int32_t x{0};
    std::int32_t y{0};
    float priority{0.0f};
};

inline constexpr float DEFAULT_MOUNTAIN_STRENGTH = 0.35f;
inline constexpr float NORMALIZED_WATERLINE = 0.5f;

struct GreaterRealmGeneratorSettings {
    Seed seed{1};
    std::uint32_t width{256};
    std::uint32_t height{256};
    float cell_size{1.0f};

    float sea_level{NORMALIZED_WATERLINE};

    float mountain_threshold{0.82f};
    float highland_threshold{0.68f};
    float hill_threshold{0.55f};

    float base_elevation_frequency{5.0f};
    float ridge_frequency{9.0f};
    float valley_frequency{6.0f};
    float terrain_noise_frequency{18.0f};
    float ocean_noise_frequency{8.0f};

    float island_bias{0.5f};
    float seed_terrain_variation{1.0f};
    float base_elevation_weight{1.0f};
    float mountain_weight{DEFAULT_MOUNTAIN_STRENGTH};
    float mountain_peak_spacing{28.0f};
    float mountain_peak_radius{36.0f};
    float mountain_peak_jaggedness{0.25f};
    float ridge_weight{0.25f};
    float valley_weight{0.25f};
    float coastline_noise_weight{0.01f};
    float terrain_noise_weight{0.12f};
    float ocean_depth_weight{1.0f};

    float river_min_drainage_area{500.0f};
    float river_width_scale{0.25f};
};

struct GreaterRealmTerrainCharacter {
    float ruggedness{0.5f};
    float base_relief_scale{1.0f};
    float mountain_relief_scale{1.0f};
    float mountain_coverage_scale{1.0f};
    float detail_scale{1.0f};
    float peak_spacing_scale{1.0f};
    float peak_radius_scale{1.0f};

    [[nodiscard]] bool operator==(const GreaterRealmTerrainCharacter&) const noexcept = default;
};

[[nodiscard]] GreaterRealmTerrainCharacter derive_greater_realm_terrain_character(
    const GreaterRealmGeneratorSettings& settings
) noexcept;

enum class GreaterRealmDirtyStage : std::uint32_t {
    None = 0,
    TerrainFields = 1u << 0,
    MountainPeaks = 1u << 1,
    Relief = 1u << 2,
    Classification = 1u << 3,
    Drainage = 1u << 4,
    RiverChannels = 1u << 5,
    DebugImage = 1u << 6,
    TextureUpload = 1u << 7
};

[[nodiscard]] constexpr GreaterRealmDirtyStage operator|(
    GreaterRealmDirtyStage left,
    GreaterRealmDirtyStage right
) noexcept {
    return static_cast<GreaterRealmDirtyStage>(
        static_cast<std::uint32_t>(left) | static_cast<std::uint32_t>(right)
    );
}

[[nodiscard]] constexpr GreaterRealmDirtyStage operator&(
    GreaterRealmDirtyStage left,
    GreaterRealmDirtyStage right
) noexcept {
    return static_cast<GreaterRealmDirtyStage>(
        static_cast<std::uint32_t>(left) & static_cast<std::uint32_t>(right)
    );
}

constexpr GreaterRealmDirtyStage& operator|=(
    GreaterRealmDirtyStage& left,
    GreaterRealmDirtyStage right
) noexcept {
    left = left | right;
    return left;
}

[[nodiscard]] constexpr bool has_dirty_stage(
    GreaterRealmDirtyStage stages,
    GreaterRealmDirtyStage stage
) noexcept {
    return (stages & stage) != GreaterRealmDirtyStage::None;
}

struct GreaterRealmMap {
    Seed seed{1};
    std::uint32_t width{0};
    std::uint32_t height{0};
    float cell_size{1.0f};
    GreaterRealmTerrainCharacter terrain_character;
    std::vector<GreaterRealmCell> cells;
    std::vector<std::uint32_t> drainage_order;
    std::vector<GreaterRealmRiverSegment> rivers;
    std::vector<GreaterRealmMountainPeak> mountain_peaks;

    [[nodiscard]] bool empty() const noexcept { return cells.empty(); }
    [[nodiscard]] std::size_t expected_cell_count() const noexcept;
    [[nodiscard]] bool has_expected_cell_count() const noexcept;
    [[nodiscard]] bool contains(std::int32_t x, std::int32_t y) const noexcept;
    [[nodiscard]] std::size_t index(std::uint32_t x, std::uint32_t y) const noexcept;

    [[nodiscard]] GreaterRealmCell* cell(std::int32_t x, std::int32_t y) noexcept;
    [[nodiscard]] const GreaterRealmCell* cell(std::int32_t x, std::int32_t y) const noexcept;
};

struct GreaterRealmRegenerationTimings {
    double terrain_fields_ms{0.0};
    double mountain_peaks_ms{0.0};
    double relief_ms{0.0};
    double classification_ms{0.0};
    double drainage_ms{0.0};
    double river_channels_ms{0.0};
    double total_ms{0.0};
};

struct GreaterRealmRegenerationResult {
    GreaterRealmDirtyStage rebuilt_stages{GreaterRealmDirtyStage::None};
    GreaterRealmRegenerationTimings timings;

    [[nodiscard]] bool rebuilt(GreaterRealmDirtyStage stage) const noexcept {
        return has_dirty_stage(rebuilt_stages, stage);
    }
};

class GreaterRealmGenerationCache {
public:
    struct TerrainLayers {
        float base_elevation{0.0f};
        float ridge{0.0f};
        float valley{0.0f};
        float terrain_noise{0.0f};
        float ocean_noise{0.0f};
    };

    void invalidate(
        GreaterRealmDirtyStage stage = GreaterRealmDirtyStage::TerrainFields
    ) noexcept;
    void reset() noexcept;

    [[nodiscard]] bool initialized() const noexcept { return m_initialized; }
    [[nodiscard]] GreaterRealmRegenerationResult regenerate(
        GreaterRealmMap& map,
        const GreaterRealmGeneratorSettings& settings
    );
    [[nodiscard]] GreaterRealmRegenerationResult regenerate(
        GreaterRealmMap& map,
        const GreaterRealmGeneratorSettings& settings,
        const TerrainConstraintField& constraints
    );

private:
    [[nodiscard]] GreaterRealmRegenerationResult regenerate_impl(
        GreaterRealmMap& map,
        const GreaterRealmGeneratorSettings& settings,
        const TerrainConstraintField* constraints
    );

    GreaterRealmGeneratorSettings m_settings;
    std::vector<TerrainLayers> m_layers;
    GreaterRealmDirtyStage m_pending_stages{GreaterRealmDirtyStage::TerrainFields};
    const TerrainConstraintField* m_constraints{nullptr};
    bool m_initialized{false};
};

[[nodiscard]] GreaterRealmMap generate_greater_realm(const GreaterRealmGeneratorSettings& settings);
[[nodiscard]] GreaterRealmMap generate_greater_realm(
    const GreaterRealmGeneratorSettings& settings,
    const TerrainConstraintField& constraints
);

} // namespace procgen
