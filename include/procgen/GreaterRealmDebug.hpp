#pragma once

#include "GreaterRealm.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace procgen {

enum class GreaterRealmDebugView : std::uint8_t {
    Terrain,
    TerrainForms,
    Elevation,
    Landmass,
    HillRelief,
    MountainRelief,
    MountainInfluence,
    Slope,
    CoastDistance,
    CatchmentArea,
    Count
};

struct GreaterRealmDebugOptions {
    GreaterRealmDebugView view{GreaterRealmDebugView::Terrain};
    bool show_coastline{true};
    bool show_mountain_peaks{true};
    bool show_rivers{true};
    bool show_drainage_directions{false};
};

struct TerrainFormCounts {
    std::size_t ocean{0};
    std::size_t inland_water{0};
    std::size_t coastal_land{0};
    std::size_t plains{0};
    std::size_t hills{0};
    std::size_t highlands{0};
    std::size_t mountains{0};
};

struct DebugColour {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{255};

    [[nodiscard]] bool operator==(const DebugColour&) const noexcept = default;
};

struct DebugImage {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::vector<std::uint8_t> rgba;

    [[nodiscard]] std::size_t expected_byte_count() const noexcept;
    [[nodiscard]] bool has_expected_byte_count() const noexcept;
};

[[nodiscard]] TerrainFormCounts count_terrain_forms(const GreaterRealmMap& map) noexcept;
[[nodiscard]] const char* to_string(GreaterRealmDebugView view) noexcept;
[[nodiscard]] DebugColour greater_realm_debug_colour(
    const GreaterRealmCell& cell,
    float sea_level,
    GreaterRealmDebugView view = GreaterRealmDebugView::Terrain,
    float scalar_maximum = 1.0f
) noexcept;
[[nodiscard]] DebugImage build_greater_realm_debug_image(
    const GreaterRealmMap& map,
    float sea_level
);
[[nodiscard]] DebugImage build_greater_realm_debug_image(
    const GreaterRealmMap& map,
    float sea_level,
    const GreaterRealmDebugOptions& options
);

} // namespace procgen
