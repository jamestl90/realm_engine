#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace procgen {

using Seed = std::uint64_t;

enum class TerrainForm : std::uint8_t {
    Ocean,
    Coast,
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
    float elevation{0.0f};
    bool is_water{false};
    bool is_ocean{false};
    float distance_to_coast{0.0f};
    float slope{0.0f};
    TerrainForm terrain_form{TerrainForm::Ocean};
};

struct GreaterRealmGeneratorSettings {
    Seed seed{1};
    std::uint32_t width{256};
    std::uint32_t height{256};
    float cell_size{1.0f};

    float sea_level{0.5f};
    float coast_distance{3.0f};

    float mountain_threshold{0.82f};
    float highland_threshold{0.68f};
    float hill_threshold{0.55f};

    float land_shape_frequency{2.0f};
    float base_elevation_frequency{5.0f};
    float mountain_frequency{3.0f};
    float ridge_frequency{9.0f};
    float valley_frequency{6.0f};
    float terrain_noise_frequency{18.0f};

    float land_shape_weight{1.0f};
    float base_elevation_weight{1.0f};
    float mountain_weight{0.35f};
    float ridge_weight{0.25f};
    float valley_weight{0.25f};
    float terrain_noise_weight{0.12f};
};

struct GreaterRealmMap {
    Seed seed{1};
    std::uint32_t width{0};
    std::uint32_t height{0};
    float cell_size{1.0f};
    std::vector<GreaterRealmCell> cells;

    [[nodiscard]] bool empty() const noexcept { return cells.empty(); }
    [[nodiscard]] std::size_t expected_cell_count() const noexcept;
    [[nodiscard]] bool has_expected_cell_count() const noexcept;
    [[nodiscard]] bool contains(std::int32_t x, std::int32_t y) const noexcept;
    [[nodiscard]] std::size_t index(std::uint32_t x, std::uint32_t y) const noexcept;

    [[nodiscard]] GreaterRealmCell* cell(std::int32_t x, std::int32_t y) noexcept;
    [[nodiscard]] const GreaterRealmCell* cell(std::int32_t x, std::int32_t y) const noexcept;
};

[[nodiscard]] GreaterRealmMap generate_greater_realm(const GreaterRealmGeneratorSettings& settings);

} // namespace procgen
