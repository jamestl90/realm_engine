#include "../../include/procgen/Climate.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace procgen {
namespace {

constexpr float PI = 3.14159265358979323846f;

float clamp01(float value) noexcept {
    return std::clamp(value, 0.0f, 1.0f);
}

} // namespace

void generate_greater_realm_climate(
    GreaterRealmMap& map,
    const GreaterRealmGeneratorSettings& settings
) {
    if (!map.has_expected_cell_count()) {
        return;
    }

    const float angle = settings.wind_angle_degrees * PI / 180.0f;
    const float wind_x = std::cos(angle);
    const float wind_y = std::sin(angle);
    const std::int32_t step_x = std::abs(wind_x) >= 0.38268343f ? (wind_x >= 0.0f ? 1 : -1) : 0;
    const std::int32_t step_y = std::abs(wind_y) >= 0.38268343f ? (wind_y >= 0.0f ? 1 : -1) : 0;
    const float raininess = std::max(settings.raininess, 0.0f);
    const float rain_shadow = std::max(settings.rain_shadow, 0.0f);
    const float evaporation = clamp01(settings.evaporation);

    std::vector<std::uint32_t> wind_order(map.cells.size());
    std::iota(wind_order.begin(), wind_order.end(), 0u);
    std::stable_sort(wind_order.begin(), wind_order.end(), [&](std::uint32_t left, std::uint32_t right) {
        const auto& a = map.cells[left];
        const auto& b = map.cells[right];
        const float projection_a = static_cast<float>(a.x) * wind_x + static_cast<float>(a.y) * wind_y;
        const float projection_b = static_cast<float>(b.x) * wind_x + static_cast<float>(b.y) * wind_y;
        return projection_a == projection_b ? left < right : projection_a < projection_b;
    });

    for (const std::uint32_t index : wind_order) {
        auto& cell = map.cells[index];
        if (cell.is_water) {
            cell.humidity = clamp01(0.65f + evaporation * 0.35f);
            cell.rainfall = 0.0f;
            cell.moisture = 1.0f;
            continue;
        }

        const auto* upwind = map.cell(cell.x - step_x, cell.y - step_y);
        const float incoming_humidity = upwind ? upwind->humidity : 0.15f;
        const float upwind_elevation = upwind ? upwind->elevation : cell.elevation;
        const float elevation_rise = std::max(cell.elevation - upwind_elevation, 0.0f);
        const float orographic_rain = incoming_humidity * elevation_rise * rain_shadow * 2.0f;
        const float ambient_rain = incoming_humidity * 0.08f + 0.02f;

        cell.rainfall = clamp01((ambient_rain + orographic_rain) * raininess);
        cell.humidity = clamp01(
            incoming_humidity
            - cell.rainfall * (0.30f + 0.25f * rain_shadow)
            - evaporation * 0.015f
        );
        cell.moisture = clamp01(cell.rainfall * 2.0f + cell.humidity * 0.20f);
    }
}

} // namespace procgen
