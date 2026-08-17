#pragma once

#include "procgen/Climate.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace world {

inline constexpr std::uint32_t SEASONAL_TEMPERATURE_VERSION = 1;
inline constexpr std::uint32_t SEASONAL_PRECIPITATION_VERSION = 1;

struct SeasonalTemperatureSettings {
    procgen::Seed profile_seed{1};
    std::uint64_t profile_identity{1};
    float north_edge_latitude_degrees{60.0f};
    float south_edge_latitude_degrees{-60.0f};
    float base_amplitude{0.10f};
    float latitude_amplitude{0.16f};
    float elevation_amplitude{0.04f};
    float maritime_damping{0.35f};
    float maritime_influence_distance{16.0f};
    float northern_peak_year_fraction{0.50f};
    float southern_peak_year_fraction{0.0f};
    float regional_phase_variation{0.0f};
    float regional_amplitude_variation{0.0f};
    float regional_variation_frequency{1.5f};

    [[nodiscard]] bool operator==(const SeasonalTemperatureSettings&) const noexcept = default;
};

struct SeasonalTemperatureCell {
    float annual_temperature_normal{0.5f};
    float seasonal_offset{0.0f};
    float seasonal_temperature_normal{0.5f};
};

struct SeasonalTemperatureMap {
    std::uint32_t version{SEASONAL_TEMPERATURE_VERSION};
    procgen::Seed source_seed{1};
    std::uint32_t source_width{0};
    std::uint32_t source_height{0};
    float source_cell_size{1.0f};
    std::uint64_t source_terrain_fingerprint{0};
    std::uint64_t source_temperature_fingerprint{0};
    std::uint64_t settings_fingerprint{0};
    float year_fraction{0.0f};
    std::vector<SeasonalTemperatureCell> cells;

    [[nodiscard]] std::size_t expected_cell_count() const noexcept;
    [[nodiscard]] bool has_expected_cell_count() const noexcept;
    [[nodiscard]] bool source_matches(
        const procgen::GreaterRealmMap& terrain,
        const procgen::GreaterRealmClimateMap& climate,
        const SeasonalTemperatureSettings& settings,
        float requested_year_fraction
    ) const noexcept;
};

struct SeasonalTemperatureEvaluationResult {
    bool rebuilt{false};
};

struct SeasonalPrecipitationSettings {
    procgen::Seed profile_seed{1};
    std::uint64_t profile_identity{1};
    float north_edge_latitude_degrees{60.0f};
    float south_edge_latitude_degrees{-60.0f};
    float base_amplitude{0.25f};
    float latitude_amplitude{0.15f};
    float inland_damping{0.10f};
    float northern_wet_peak_year_fraction{0.0f};
    float southern_wet_peak_year_fraction{0.50f};
    float regional_phase_variation{0.0f};
    float regional_amplitude_variation{0.0f};
    float regional_variation_frequency{1.25f};
    float minimum_multiplier{0.25f};
    float maximum_multiplier{1.75f};

    [[nodiscard]] bool operator==(const SeasonalPrecipitationSettings&) const noexcept = default;
};

struct SeasonalPrecipitationCell {
    float annual_precipitation_normal{0.0f};
    float seasonal_multiplier{1.0f};
    float seasonal_precipitation_normal{0.0f};
};

struct SeasonalPrecipitationMap {
    std::uint32_t version{SEASONAL_PRECIPITATION_VERSION};
    procgen::Seed source_seed{1};
    std::uint32_t source_width{0};
    std::uint32_t source_height{0};
    float source_cell_size{1.0f};
    std::uint64_t source_terrain_fingerprint{0};
    std::uint64_t source_precipitation_fingerprint{0};
    std::uint64_t settings_fingerprint{0};
    float year_fraction{0.0f};
    std::vector<SeasonalPrecipitationCell> cells;

    [[nodiscard]] std::size_t expected_cell_count() const noexcept;
    [[nodiscard]] bool has_expected_cell_count() const noexcept;
    [[nodiscard]] bool source_matches(
        const procgen::GreaterRealmMap& terrain,
        const procgen::GreaterRealmClimateMap& climate,
        const SeasonalPrecipitationSettings& settings,
        float requested_year_fraction
    ) const noexcept;
};

struct SeasonalPrecipitationEvaluationResult {
    bool rebuilt{false};
};

class SeasonalTemperatureEvaluationCache {
public:
    void invalidate() noexcept;
    void reset() noexcept;

    [[nodiscard]] bool initialized() const noexcept { return m_initialized; }
    [[nodiscard]] SeasonalTemperatureEvaluationResult regenerate(
        SeasonalTemperatureMap& seasonal_temperature,
        const procgen::GreaterRealmMap& terrain,
        const procgen::GreaterRealmClimateMap& climate,
        const SeasonalTemperatureSettings& settings,
        float year_fraction
    );

private:
    SeasonalTemperatureSettings m_settings;
    float m_year_fraction{0.0f};
    bool m_pending{true};
    bool m_initialized{false};
};

class SeasonalPrecipitationEvaluationCache {
public:
    void invalidate() noexcept;
    void reset() noexcept;

    [[nodiscard]] bool initialized() const noexcept { return m_initialized; }
    [[nodiscard]] SeasonalPrecipitationEvaluationResult regenerate(
        SeasonalPrecipitationMap& seasonal_precipitation,
        const procgen::GreaterRealmMap& terrain,
        const procgen::GreaterRealmClimateMap& climate,
        const SeasonalPrecipitationSettings& settings,
        float year_fraction
    );

private:
    SeasonalPrecipitationSettings m_settings;
    float m_year_fraction{0.0f};
    bool m_pending{true};
    bool m_initialized{false};
};

[[nodiscard]] SeasonalTemperatureSettings clamp_seasonal_temperature_settings(
    const SeasonalTemperatureSettings& settings
) noexcept;
[[nodiscard]] float normalize_year_fraction(float year_fraction) noexcept;
[[nodiscard]] float seasonal_temperature_latitude_for_row(
    const SeasonalTemperatureSettings& settings,
    std::uint32_t row,
    std::uint32_t height
) noexcept;
[[nodiscard]] std::uint64_t seasonal_temperature_settings_fingerprint(
    const SeasonalTemperatureSettings& settings
) noexcept;
[[nodiscard]] std::uint64_t annual_temperature_fingerprint(
    const procgen::GreaterRealmClimateMap& climate
) noexcept;
[[nodiscard]] SeasonalPrecipitationSettings clamp_seasonal_precipitation_settings(
    const SeasonalPrecipitationSettings& settings
) noexcept;
[[nodiscard]] float seasonal_precipitation_latitude_for_row(
    const SeasonalPrecipitationSettings& settings,
    std::uint32_t row,
    std::uint32_t height
) noexcept;
[[nodiscard]] std::uint64_t seasonal_precipitation_settings_fingerprint(
    const SeasonalPrecipitationSettings& settings
) noexcept;
[[nodiscard]] std::uint64_t annual_precipitation_fingerprint(
    const procgen::GreaterRealmClimateMap& climate
) noexcept;
[[nodiscard]] SeasonalTemperatureMap evaluate_seasonal_temperature(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalTemperatureSettings& settings,
    float year_fraction
);
[[nodiscard]] SeasonalPrecipitationMap evaluate_seasonal_precipitation(
    const procgen::GreaterRealmMap& terrain,
    const procgen::GreaterRealmClimateMap& climate,
    const SeasonalPrecipitationSettings& settings,
    float year_fraction
);

} // namespace world
