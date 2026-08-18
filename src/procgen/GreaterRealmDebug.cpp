#include "../../include/procgen/GreaterRealmDebug.hpp"
#include <algorithm>
#include <array>
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

struct TerrainColourAnchor {
    float elevation;
    DebugColour colour;
};

constexpr std::array TERRAIN_COLOUR_ANCHORS{
    TerrainColourAnchor{0.50f, {52, 108, 70, 255}},
    TerrainColourAnchor{0.54f, {65, 132, 73, 255}},
    TerrainColourAnchor{0.59f, {96, 146, 72, 255}},
    TerrainColourAnchor{0.65f, {148, 154, 84, 255}},
    TerrainColourAnchor{0.75f, {160, 145, 112, 255}},
    TerrainColourAnchor{0.86f, {178, 177, 169, 255}},
    TerrainColourAnchor{1.00f, {230, 230, 220, 255}}
};

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

DebugColour ocean_depth_colour(const GreaterRealmCell& cell) noexcept {
    const float relative_depth = 1.0f - std::clamp(
        cell.elevation / NORMALIZED_WATERLINE,
        0.0f,
        1.0f
    );
    const float shallow = 1.0f - std::sqrt(relative_depth);
    return mix_colour({3, 18, 52, 255}, {66, 145, 196, 255}, shallow);
}

DebugColour inland_water_colour(const GreaterRealmCell& cell) noexcept {
    const float relative_depth = 1.0f - std::clamp(
        cell.elevation / NORMALIZED_WATERLINE,
        0.0f,
        1.0f
    );
    const float shallow = 1.0f - std::sqrt(relative_depth);
    return mix_colour({8, 48, 58, 255}, {82, 177, 160, 255}, shallow);
}

DebugColour terrain_form_colour(const GreaterRealmCell& cell) noexcept {
    const float land_range = 1.0f - NORMALIZED_WATERLINE;
    const float relative_height = std::clamp(
        (cell.elevation - NORMALIZED_WATERLINE) / land_range,
        0.0f,
        1.0f
    );
    const float shade = 0.62f + relative_height * 0.38f;
    const auto shaded = [shade](DebugColour colour) noexcept {
        colour.r = static_cast<std::uint8_t>(static_cast<float>(colour.r) * shade);
        colour.g = static_cast<std::uint8_t>(static_cast<float>(colour.g) * shade);
        colour.b = static_cast<std::uint8_t>(static_cast<float>(colour.b) * shade);
        return colour;
    };

    switch (cell.terrain_form) {
        case TerrainForm::Ocean: return ocean_depth_colour(cell);
        case TerrainForm::InlandWater: return inland_water_colour(cell);
        case TerrainForm::Plains: return shaded({78, 150, 82, 255});
        case TerrainForm::Hills: return shaded({112, 136, 74, 255});
        case TerrainForm::Highlands: return shaded({126, 112, 94, 255});
        case TerrainForm::Mountains: return shaded({192, 194, 188, 255});
    }
    return {255, 0, 255, 255};
}

DebugColour terrain_elevation_colour(float elevation) noexcept {
    const float clamped = std::clamp(elevation, NORMALIZED_WATERLINE, 1.0f);
    for (std::size_t index = 1; index < TERRAIN_COLOUR_ANCHORS.size(); ++index) {
        const auto& lower = TERRAIN_COLOUR_ANCHORS[index - 1];
        const auto& upper = TERRAIN_COLOUR_ANCHORS[index];
        if (clamped <= upper.elevation) {
            const float amount = (clamped - lower.elevation)
                / (upper.elevation - lower.elevation);
            return mix_colour(lower.colour, upper.colour, amount);
        }
    }
    return TERRAIN_COLOUR_ANCHORS.back().colour;
}

DebugColour continuous_terrain_colour(const GreaterRealmCell& cell) noexcept {
    if (cell.terrain_form == TerrainForm::Ocean) {
        return ocean_depth_colour(cell);
    }
    if (cell.terrain_form == TerrainForm::InlandWater) {
        return inland_water_colour(cell);
    }

    const DebugColour elevation_colour = terrain_elevation_colour(cell.elevation);
    constexpr float form_tint_strength = 0.08f;
    return mix_colour(elevation_colour, terrain_form_colour(cell), form_tint_strength);
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
            case GreaterRealmDebugView::CatchmentArea:
                maximum = std::max(maximum, cell.drainage_area);
                break;
            case GreaterRealmDebugView::HillRelief:
            case GreaterRealmDebugView::MountainRelief:
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

DebugColour biome_debug_colour(
    BiomeId biome_id,
    const std::vector<BiomeDebugColour>* biome_colours
) noexcept {
    if (biome_colours) {
        const auto found = std::find_if(
            biome_colours->begin(),
            biome_colours->end(),
            [biome_id](const BiomeDebugColour& entry) { return entry.biome_id == biome_id; }
        );
        if (found != biome_colours->end()) {
            return found->colour;
        }
    }
    if (biome_id == INVALID_BIOME_ID) {
        return {72, 72, 76, 255};
    }
    constexpr std::array<DebugColour, 8> FALLBACK_PALETTE{{
        {88, 126, 118, 255}, {154, 118, 82, 255}, {92, 112, 156, 255},
        {142, 92, 112, 255}, {112, 142, 82, 255}, {86, 138, 158, 255},
        {158, 142, 88, 255}, {118, 98, 148, 255}
    }};
    return FALLBACK_PALETTE[biome_id % FALLBACK_PALETTE.size()];
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
            case TerrainForm::InlandWater:
                ++counts.inland_water;
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
        case GreaterRealmDebugView::TerrainForms: return "Terrain forms";
        case GreaterRealmDebugView::Elevation: return "Elevation";
        case GreaterRealmDebugView::Landmass: return "Landmass";
        case GreaterRealmDebugView::HillRelief: return "Hill relief";
        case GreaterRealmDebugView::MountainRelief: return "Mountain relief";
        case GreaterRealmDebugView::MountainInfluence: return "Mountain influence";
        case GreaterRealmDebugView::Slope: return "Slope";
        case GreaterRealmDebugView::CoastDistance: return "Coast distance";
        case GreaterRealmDebugView::CatchmentArea: return "Catchment area";
        case GreaterRealmDebugView::TemperatureNormal: return "Temperature normal";
        case GreaterRealmDebugView::PrecipitationNormal: return "Precipitation normal";
        case GreaterRealmDebugView::Biome: return "Biome";
        case GreaterRealmDebugView::Count: break;
    }
    return "Unknown";
}

DebugColour greater_realm_debug_colour(
    const GreaterRealmCell& cell,
    float,
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
        case GreaterRealmDebugView::HillRelief:
            return three_colour_gradient(
                cell.hill_relief,
                {24, 36, 44, 255},
                {92, 156, 102, 255},
                {224, 216, 146, 255}
            );
        case GreaterRealmDebugView::MountainRelief:
            return three_colour_gradient(
                cell.mountain_relief,
                {24, 30, 42, 255},
                {138, 116, 94, 255},
                {244, 244, 232, 255}
            );
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
        case GreaterRealmDebugView::CatchmentArea: {
            const float maximum = std::max(scalar_maximum, 0.0001f);
            const float normalized = std::log1p(std::max(cell.drainage_area, 0.0f)) / std::log1p(maximum);
            return three_colour_gradient(
                normalized,
                {18, 20, 28, 255},
                {32, 126, 160, 255},
                {204, 242, 240, 255}
            );
        }
        case GreaterRealmDebugView::TerrainForms:
            return terrain_form_colour(cell);
        case GreaterRealmDebugView::Terrain:
            return continuous_terrain_colour(cell);
        case GreaterRealmDebugView::TemperatureNormal:
        case GreaterRealmDebugView::PrecipitationNormal:
        case GreaterRealmDebugView::Biome:
        case GreaterRealmDebugView::Count:
            break;
    }
    return {255, 0, 255, 255};
}

void apply_greater_realm_debug_overlays(
    DebugImage& image,
    const GreaterRealmMap& map,
    const GreaterRealmDebugOptions& options
) noexcept {
    if (!map.has_expected_cell_count()
        || image.width != map.width
        || image.height != map.height
        || !image.has_expected_byte_count()) {
        return;
    }

    if (options.show_coastline) {
        constexpr float coastline_outline = 0.72f;
        for (std::size_t index = 0; index < map.cells.size(); ++index) {
            if (!map.cells[index].is_coastal) {
                continue;
            }
            const std::size_t pixel = index * 4;
            image.rgba[pixel] = static_cast<std::uint8_t>(
                static_cast<float>(image.rgba[pixel]) * coastline_outline
            );
            image.rgba[pixel + 1] = static_cast<std::uint8_t>(
                static_cast<float>(image.rgba[pixel + 1]) * coastline_outline
            );
            image.rgba[pixel + 2] = static_cast<std::uint8_t>(
                static_cast<float>(image.rgba[pixel + 2]) * coastline_outline
            );
        }
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
}

static DebugImage build_greater_realm_debug_image_impl(
    const GreaterRealmMap& map,
    const GreaterRealmClimateMap* climate,
    const GreaterRealmBiomeMap* biomes,
    const std::vector<BiomeDebugColour>* biome_colours,
    float sea_level,
    const GreaterRealmDebugOptions& options
);

DebugImage build_greater_realm_debug_image(const GreaterRealmMap& map, float sea_level) {
    return build_greater_realm_debug_image(map, sea_level, GreaterRealmDebugOptions{});
}

DebugImage build_greater_realm_debug_image(
    const GreaterRealmMap& map,
    float sea_level,
    const GreaterRealmDebugOptions& options
) {
    return build_greater_realm_debug_image_impl(
        map, nullptr, nullptr, nullptr, sea_level, options
    );
}

DebugImage build_greater_realm_debug_image(
    const GreaterRealmMap& map,
    const GreaterRealmClimateMap& climate,
    float sea_level,
    const GreaterRealmDebugOptions& options
) {
    return build_greater_realm_debug_image_impl(
        map, &climate, nullptr, nullptr, sea_level, options
    );
}

DebugImage build_greater_realm_debug_image(
    const GreaterRealmMap& map,
    const GreaterRealmClimateMap& climate,
    const GreaterRealmBiomeMap& biomes,
    const std::vector<BiomeDebugColour>& biome_colours,
    float sea_level,
    const GreaterRealmDebugOptions& options
) {
    return build_greater_realm_debug_image_impl(
        map, &climate, &biomes, &biome_colours, sea_level, options
    );
}

static DebugImage build_greater_realm_debug_image_impl(
    const GreaterRealmMap& map,
    const GreaterRealmClimateMap* climate,
    const GreaterRealmBiomeMap* biomes,
    const std::vector<BiomeDebugColour>* biome_colours,
    float sea_level,
    const GreaterRealmDebugOptions& options
) {
    DebugImage image;
    image.width = map.width;
    image.height = map.height;

    const bool temperature_view = options.view == GreaterRealmDebugView::TemperatureNormal;
    const bool precipitation_view = options.view == GreaterRealmDebugView::PrecipitationNormal;
    const bool biome_view = options.view == GreaterRealmDebugView::Biome;
    if (!map.has_expected_cell_count()
        || ((temperature_view || precipitation_view)
            && (!climate || !climate->source_matches(map)))
        || (biome_view
            && (!climate || !biomes || !biomes->source_maps_match(map, *climate)))) {
        return image;
    }

    const float scalar_maximum = scalar_maximum_for_view(map, options.view);
    image.rgba.resize(image.expected_byte_count());
    for (std::size_t index = 0; index < map.cells.size(); ++index) {
        DebugColour colour;
        if (temperature_view) {
            colour = three_colour_gradient(
                climate->cells[index].temperature_normal,
                {38, 82, 148, 255},
                {104, 172, 120, 255},
                {222, 82, 48, 255}
            );
        } else if (precipitation_view) {
            colour = three_colour_gradient(
                climate->cells[index].precipitation_normal,
                {196, 158, 78, 255},
                {72, 158, 104, 255},
                {46, 102, 178, 255}
            );
        } else if (biome_view) {
            colour = biome_debug_colour(biomes->cells[index].biome_id, biome_colours);
        } else {
            colour = greater_realm_debug_colour(
                map.cells[index],
                sea_level,
                options.view,
                scalar_maximum
            );
        }
        const std::size_t pixel = index * 4;
        image.rgba[pixel] = colour.r;
        image.rgba[pixel + 1] = colour.g;
        image.rgba[pixel + 2] = colour.b;
        image.rgba[pixel + 3] = colour.a;
    }

    apply_greater_realm_debug_overlays(image, map, options);

    return image;
}

} // namespace procgen
