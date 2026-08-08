#include "../../include/procgen/GreaterRealm.hpp"

namespace procgen {

const char* to_string(TerrainForm form) noexcept {
    switch (form) {
        case TerrainForm::Ocean:
            return "ocean";
        case TerrainForm::Coast:
            return "coast";
        case TerrainForm::Plains:
            return "plains";
        case TerrainForm::Hills:
            return "hills";
        case TerrainForm::Highlands:
            return "highlands";
        case TerrainForm::Mountains:
            return "mountains";
    }

    return "unknown";
}

bool is_water(TerrainForm form) noexcept {
    return form == TerrainForm::Ocean;
}

std::size_t GreaterRealmMap::expected_cell_count() const noexcept {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

bool GreaterRealmMap::has_expected_cell_count() const noexcept {
    return cells.size() == expected_cell_count();
}

bool GreaterRealmMap::contains(std::int32_t x, std::int32_t y) const noexcept {
    return x >= 0
        && y >= 0
        && static_cast<std::uint32_t>(x) < width
        && static_cast<std::uint32_t>(y) < height;
}

std::size_t GreaterRealmMap::index(std::uint32_t x, std::uint32_t y) const noexcept {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
}

GreaterRealmCell* GreaterRealmMap::cell(std::int32_t x, std::int32_t y) noexcept {
    if (!contains(x, y) || !has_expected_cell_count()) {
        return nullptr;
    }

    return &cells[index(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y))];
}

const GreaterRealmCell* GreaterRealmMap::cell(std::int32_t x, std::int32_t y) const noexcept {
    if (!contains(x, y) || !has_expected_cell_count()) {
        return nullptr;
    }

    return &cells[index(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y))];
}

} // namespace procgen
