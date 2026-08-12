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
    float elevation{0.0f};
    bool is_water{false};
    bool is_ocean{false};
    bool is_coastal{false};
    float distance_to_coast{0.0f};
    float slope{0.0f};
    float humidity{0.0f};
    float rainfall{0.0f};
    float moisture{0.0f};
    float drainage_elevation{0.0f};
    float flow{0.0f};
    std::uint32_t downslope_index{INVALID_CELL_INDEX};
    bool is_drainage_outlet{false};
    TerrainForm terrain_form{TerrainForm::Ocean};
};

struct GreaterRealmRiverSegment {
    std::uint32_t source_index{INVALID_CELL_INDEX};
    std::uint32_t destination_index{INVALID_CELL_INDEX};
    float flow{0.0f};
    float width{0.0f};
};

struct GreaterRealmGeneratorSettings {
    Seed seed{1};
    std::uint32_t width{256};
    std::uint32_t height{256};
    float cell_size{1.0f};

    float sea_level{0.5f};

    float mountain_threshold{0.82f};
    float highland_threshold{0.68f};
    float hill_threshold{0.55f};

    float base_elevation_frequency{5.0f};
    float mountain_frequency{3.0f};
    float ridge_frequency{9.0f};
    float valley_frequency{6.0f};
    float coastline_noise_frequency{14.0f};
    float terrain_noise_frequency{18.0f};
    float ocean_noise_frequency{8.0f};

    float island_bias{0.5f};
    float base_elevation_weight{1.0f};
    float mountain_weight{0.35f};
    float ridge_weight{0.25f};
    float valley_weight{0.25f};
    float coastline_noise_weight{0.08f};
    float terrain_noise_weight{0.12f};
    float ocean_depth_weight{1.0f};

    float wind_angle_degrees{0.0f};
    float raininess{0.9f};
    float rain_shadow{0.5f};
    float evaporation{0.5f};

    float river_flow_scale{0.2f};
    float river_min_flow{12.0f};
    float river_width_scale{0.5f};
};

struct GreaterRealmMap {
    Seed seed{1};
    std::uint32_t width{0};
    std::uint32_t height{0};
    float cell_size{1.0f};
    std::vector<GreaterRealmCell> cells;
    std::vector<std::uint32_t> drainage_order;
    std::vector<GreaterRealmRiverSegment> rivers;

    [[nodiscard]] bool empty() const noexcept { return cells.empty(); }
    [[nodiscard]] std::size_t expected_cell_count() const noexcept;
    [[nodiscard]] bool has_expected_cell_count() const noexcept;
    [[nodiscard]] bool contains(std::int32_t x, std::int32_t y) const noexcept;
    [[nodiscard]] std::size_t index(std::uint32_t x, std::uint32_t y) const noexcept;

    [[nodiscard]] GreaterRealmCell* cell(std::int32_t x, std::int32_t y) noexcept;
    [[nodiscard]] const GreaterRealmCell* cell(std::int32_t x, std::int32_t y) const noexcept;
};

[[nodiscard]] GreaterRealmMap generate_greater_realm(const GreaterRealmGeneratorSettings& settings);
[[nodiscard]] GreaterRealmMap generate_greater_realm(
    const GreaterRealmGeneratorSettings& settings,
    const TerrainConstraintField& constraints
);

} // namespace procgen
