#include "../../include/procgen/GreaterRealmDebug.hpp"
#include <algorithm>
#include <cmath>

namespace procgen {

namespace {

std::uint8_t mix_channel(std::uint8_t from, std::uint8_t to, float amount) noexcept {
    return static_cast<std::uint8_t>(
        static_cast<float>(from)
        + (static_cast<float>(to) - static_cast<float>(from)) * std::clamp(amount, 0.0f, 1.0f)
    );
}

DebugColour mix_colour(DebugColour from, DebugColour to, float amount) noexcept {
    return {
        mix_channel(from.r, to.r, amount),
        mix_channel(from.g, to.g, amount),
        mix_channel(from.b, to.b, amount),
        mix_channel(from.a, to.a, amount)
    };
}

DebugColour three_colour_gradient(
    float value,
    DebugColour low,
    DebugColour middle,
    DebugColour high
) noexcept {
    const float normalized = std::clamp(value, 0.0f, 1.0f);
    if (normalized <= 0.5f) {
        return mix_colour(low, middle, normalized * 2.0f);
    }
    return mix_colour(middle, high, (normalized - 0.5f) * 2.0f);
}

float scalar_maximum_for_view(const GreaterRealmMap& map, GreaterRealmDebugView view) noexcept {
    float maximum = 0.0f;
    for (const auto& cell : map.cells) {
        switch (view) {
            case GreaterRealmDebugView::Slope:
                maximum = std::max(maximum, cell.slope);
                break;
            case GreaterRealmDebugView::CoastDistance:
                maximum = std::max(maximum, cell.distance_to_coast);
                break;
            case GreaterRealmDebugView::DrainageFlow:
                maximum = std::max(maximum, cell.flow);
                break;
            default:
                return 1.0f;
        }
    }
    return std::max(maximum, 0.0001f);
}

void set_pixel(DebugImage& image, std::size_t cell_index, DebugColour colour) noexcept {
    const std::size_t pixel = cell_index * 4;
    if (pixel + 3 >= image.rgba.size()) {
        return;
    }
    image.rgba[pixel] = colour.r;
    image.rgba[pixel + 1] = colour.g;
    image.rgba[pixel + 2] = colour.b;
    image.rgba[pixel + 3] = colour.a;
}

} // namespace

std::size_t DebugImage::expected_byte_count() const noexcept {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
}

bool DebugImage::has_expected_byte_count() const noexcept {
    return rgba.size() == expected_byte_count();
}

TerrainFormCounts count_terrain_forms(const GreaterRealmMap& map) noexcept {
    TerrainFormCounts counts;

    for (const auto& cell : map.cells) {
        if (cell.is_coastal) {
            ++counts.coastal_land;
        }

        switch (cell.terrain_form) {
            case TerrainForm::Ocean:
                ++counts.ocean;
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

const char* to_string(GreaterRealmDebugView view) noexcept {
    switch (view) {
        case GreaterRealmDebugView::Terrain: return "Terrain";
        case GreaterRealmDebugView::Elevation: return "Elevation";
        case GreaterRealmDebugView::Landmass: return "Landmass";
        case GreaterRealmDebugView::MountainInfluence: return "Mountain influence";
        case GreaterRealmDebugView::Slope: return "Slope";
        case GreaterRealmDebugView::CoastDistance: return "Coast distance";
        case GreaterRealmDebugView::Humidity: return "Humidity";
        case GreaterRealmDebugView::Rainfall: return "Rainfall";
        case GreaterRealmDebugView::Moisture: return "Moisture";
        case GreaterRealmDebugView::DrainageFlow: return "Drainage flow";
        case GreaterRealmDebugView::Count: break;
    }
    return "Unknown";
}

DebugColour greater_realm_debug_colour(
    const GreaterRealmCell& cell,
    float sea_level,
    GreaterRealmDebugView view,
    float scalar_maximum
) noexcept {
    switch (view) {
        case GreaterRealmDebugView::Elevation:
            return three_colour_gradient(
                cell.elevation,
                {8, 16, 30, 255},
                {112, 126, 132, 255},
                {246, 246, 238, 255}
            );
        case GreaterRealmDebugView::Landmass: {
            const float signed_value = std::clamp(cell.landmass_elevation, -1.0f, 1.0f);
            if (signed_value <= 0.0f) {
                return mix_colour({4, 24, 68, 255}, {92, 190, 214, 255}, signed_value + 1.0f);
            }
            return mix_colour({116, 172, 92, 255}, {238, 236, 218, 255}, signed_value);
        }
        case GreaterRealmDebugView::MountainInfluence:
            return three_colour_gradient(
                cell.mountain_influence,
                {18, 24, 38, 255},
                {188, 86, 48, 255},
                {250, 232, 164, 255}
            );
        case GreaterRealmDebugView::Slope:
            return three_colour_gradient(
                cell.slope / std::max(scalar_maximum, 0.0001f),
                {20, 34, 42, 255},
                {212, 170, 58, 255},
                {248, 244, 224, 255}
            );
        case GreaterRealmDebugView::CoastDistance:
            return three_colour_gradient(
                cell.distance_to_coast / std::max(scalar_maximum, 0.0001f),
                {46, 190, 196, 255},
                {94, 112, 166, 255},
                {40, 24, 70, 255}
            );
        case GreaterRealmDebugView::Humidity:
            return three_colour_gradient(
                cell.humidity,
                {82, 54, 42, 255},
                {122, 160, 148, 255},
                {210, 242, 244, 255}
            );
        case GreaterRealmDebugView::Rainfall:
            return three_colour_gradient(
                cell.rainfall,
                {56, 46, 42, 255},
                {74, 138, 174, 255},
                {198, 232, 250, 255}
            );
        case GreaterRealmDebugView::Moisture:
            return three_colour_gradient(
                cell.moisture,
                {116, 72, 42, 255},
                {112, 156, 76, 255},
                {42, 112, 88, 255}
            );
        case GreaterRealmDebugView::DrainageFlow: {
            const float maximum = std::max(scalar_maximum, 0.0001f);
            const float normalized = std::log1p(std::max(cell.flow, 0.0f)) / std::log1p(maximum);
            return three_colour_gradient(
                normalized,
                {18, 20, 28, 255},
                {32, 126, 160, 255},
                {204, 242, 240, 255}
            );
        }
        case GreaterRealmDebugView::Terrain:
        case GreaterRealmDebugView::Count:
            break;
    }

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
    return build_greater_realm_debug_image(map, sea_level, GreaterRealmDebugOptions{});
}

DebugImage build_greater_realm_debug_image(
    const GreaterRealmMap& map,
    float sea_level,
    const GreaterRealmDebugOptions& options
) {
    DebugImage image;
    image.width = map.width;
    image.height = map.height;

    if (!map.has_expected_cell_count()) {
        return image;
    }

    const float scalar_maximum = scalar_maximum_for_view(map, options.view);
    image.rgba.resize(image.expected_byte_count());
    for (std::size_t index = 0; index < map.cells.size(); ++index) {
        DebugColour colour = greater_realm_debug_colour(
            map.cells[index],
            sea_level,
            options.view,
            scalar_maximum
        );
        if (options.show_coastline && map.cells[index].is_coastal) {
            constexpr float coastline_outline = 0.72f;
            colour.r = static_cast<std::uint8_t>(static_cast<float>(colour.r) * coastline_outline);
            colour.g = static_cast<std::uint8_t>(static_cast<float>(colour.g) * coastline_outline);
            colour.b = static_cast<std::uint8_t>(static_cast<float>(colour.b) * coastline_outline);
        }
        const std::size_t pixel = index * 4;
        image.rgba[pixel] = colour.r;
        image.rgba[pixel + 1] = colour.g;
        image.rgba[pixel + 2] = colour.b;
        image.rgba[pixel + 3] = colour.a;
    }

    if (options.show_drainage_directions && map.width > 0 && map.height > 0) {
        constexpr std::uint32_t sample_stride = 8;
        constexpr DebugColour drainage_colour{246, 198, 64, 255};
        for (std::uint32_t y = sample_stride / 2; y < map.height; y += sample_stride) {
            for (std::uint32_t x = sample_stride / 2; x < map.width; x += sample_stride) {
                const std::size_t index = map.index(x, y);
                const auto& cell = map.cells[index];
                if (cell.downslope_index == INVALID_CELL_INDEX || cell.downslope_index == index) {
                    continue;
                }
                set_pixel(image, index, drainage_colour);
                set_pixel(image, cell.downslope_index, drainage_colour);
            }
        }
    }

    if (options.show_rivers) {
        constexpr DebugColour river_colour{40, 156, 224, 255};
        for (const auto& river : map.rivers) {
            for (const std::uint32_t index : {river.source_index, river.destination_index}) {
                set_pixel(image, index, river_colour);
            }
        }
    }

    if (options.show_mountain_peaks) {
        constexpr DebugColour peak_colour{232, 62, 48, 255};
        for (const auto& peak : map.mountain_peaks) {
            set_pixel(image, peak.cell_index, peak_colour);
        }
    }

    return image;
}

} // namespace procgen
