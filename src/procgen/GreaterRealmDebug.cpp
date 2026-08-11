#include "../../include/procgen/GreaterRealmDebug.hpp"
#include <algorithm>
#include <cmath>

namespace procgen {

std::size_t DebugImage::expected_byte_count() const noexcept {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
}

bool DebugImage::has_expected_byte_count() const noexcept {
    return rgba.size() == expected_byte_count();
}

TerrainFormCounts count_terrain_forms(const GreaterRealmMap& map) noexcept {
    TerrainFormCounts counts;

    for (const auto& cell : map.cells) {
        switch (cell.terrain_form) {
            case TerrainForm::Ocean:
                ++counts.ocean;
                break;
            case TerrainForm::Coast:
                ++counts.coast;
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

DebugColour greater_realm_debug_colour(const GreaterRealmCell& cell, float sea_level) noexcept {
    if (cell.terrain_form == TerrainForm::Ocean) {
        const float safe_sea_level = sea_level > 0.01f ? sea_level : 0.01f;
        const float relative_depth = 1.0f - std::clamp(cell.elevation / safe_sea_level, 0.0f, 1.0f);
        const float shallow = 1.0f - std::sqrt(relative_depth);
        const auto mix = [shallow](std::uint8_t deep, std::uint8_t coast) -> std::uint8_t {
            return static_cast<std::uint8_t>(
                static_cast<float>(deep)
                + (static_cast<float>(coast) - static_cast<float>(deep)) * shallow
            );
        };
        return {mix(3, 66), mix(18, 145), mix(52, 196), 255};
    }

    const float land_range = sea_level < 0.99f ? 1.0f - sea_level : 0.01f;
    const float relative_land_height = std::clamp((cell.elevation - sea_level) / land_range, 0.0f, 1.0f);
    const float shade = 0.62f + relative_land_height * 0.38f;
    const auto scale = [shade](std::uint8_t value) -> std::uint8_t {
        return static_cast<std::uint8_t>(static_cast<float>(value) * shade);
    };

    switch (cell.terrain_form) {
        case TerrainForm::Ocean:
            break;
        case TerrainForm::Coast:
            return {210, 190, 126, 255};
        case TerrainForm::Plains:
            return {scale(78), scale(150), scale(82), 255};
        case TerrainForm::Hills:
            return {scale(112), scale(136), scale(74), 255};
        case TerrainForm::Highlands:
            return {scale(126), scale(112), scale(94), 255};
        case TerrainForm::Mountains:
            return {scale(192), scale(194), scale(188), 255};
    }

    return {255, 0, 255, 255};
}

DebugImage build_greater_realm_debug_image(const GreaterRealmMap& map, float sea_level) {
    DebugImage image;
    image.width = map.width;
    image.height = map.height;

    if (!map.has_expected_cell_count()) {
        return image;
    }

    image.rgba.resize(image.expected_byte_count());
    for (std::size_t index = 0; index < map.cells.size(); ++index) {
        const DebugColour colour = greater_realm_debug_colour(map.cells[index], sea_level);
        const std::size_t pixel = index * 4;
        image.rgba[pixel] = colour.r;
        image.rgba[pixel + 1] = colour.g;
        image.rgba[pixel + 2] = colour.b;
        image.rgba[pixel + 3] = colour.a;
    }

    return image;
}

} // namespace procgen
