#include "procgen/GreaterRealm.hpp"
#include "procgen/TerrainConstraints.hpp"
#include <cstddef>
#include <iostream>

#if !defined(REALM_TEST_BUILD)
#error "GreaterRealmRegenerationTests.cpp must only be compiled for test builds"
#endif

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool maps_match(const procgen::GreaterRealmMap& left, const procgen::GreaterRealmMap& right) {
    if (left.seed != right.seed
        || left.width != right.width
        || left.height != right.height
        || left.cell_size != right.cell_size
        || left.cells.size() != right.cells.size()
        || left.drainage_order != right.drainage_order
        || left.rivers.size() != right.rivers.size()
        || left.mountain_peaks.size() != right.mountain_peaks.size()) {
        return false;
    }

    for (std::size_t index = 0; index < left.cells.size(); ++index) {
        const auto& a = left.cells[index];
        const auto& b = right.cells[index];
        if (a.x != b.x
            || a.y != b.y
            || a.landmass_elevation != b.landmass_elevation
            || a.relief_constraint != b.relief_constraint
            || a.hill_relief != b.hill_relief
            || a.mountain_relief != b.mountain_relief
            || a.elevation != b.elevation
            || a.is_water != b.is_water
            || a.is_ocean != b.is_ocean
            || a.is_coastal != b.is_coastal
            || a.distance_to_coast != b.distance_to_coast
            || a.slope != b.slope
            || a.mountain_distance != b.mountain_distance
            || a.mountain_influence != b.mountain_influence
            || a.is_mountain_peak != b.is_mountain_peak
            || a.drainage_elevation != b.drainage_elevation
            || a.drainage_area != b.drainage_area
            || a.downslope_index != b.downslope_index
            || a.is_drainage_outlet != b.is_drainage_outlet
            || a.terrain_form != b.terrain_form) {
            return false;
        }
    }

    for (std::size_t index = 0; index < left.rivers.size(); ++index) {
        const auto& a = left.rivers[index];
        const auto& b = right.rivers[index];
        if (a.source_index != b.source_index
            || a.destination_index != b.destination_index
            || a.drainage_area != b.drainage_area
            || a.width != b.width) {
            return false;
        }
    }

    for (std::size_t index = 0; index < left.mountain_peaks.size(); ++index) {
        const auto& a = left.mountain_peaks[index];
        const auto& b = right.mountain_peaks[index];
        if (a.cell_index != b.cell_index
            || a.x != b.x
            || a.y != b.y
            || a.priority != b.priority) {
            return false;
        }
    }

    return true;
}

constexpr procgen::GreaterRealmDirtyStage FULL_REGENERATION =
    procgen::GreaterRealmDirtyStage::TerrainFields
    | procgen::GreaterRealmDirtyStage::MountainPeaks
    | procgen::GreaterRealmDirtyStage::Relief
    | procgen::GreaterRealmDirtyStage::Classification
    | procgen::GreaterRealmDirtyStage::Drainage
    | procgen::GreaterRealmDirtyStage::RiverChannels
    | procgen::GreaterRealmDirtyStage::DebugImage
    | procgen::GreaterRealmDirtyStage::TextureUpload;

constexpr procgen::GreaterRealmDirtyStage RELIEF_REGENERATION =
    procgen::GreaterRealmDirtyStage::Relief
    | procgen::GreaterRealmDirtyStage::Classification
    | procgen::GreaterRealmDirtyStage::Drainage
    | procgen::GreaterRealmDirtyStage::RiverChannels
    | procgen::GreaterRealmDirtyStage::DebugImage
    | procgen::GreaterRealmDirtyStage::TextureUpload;

constexpr procgen::GreaterRealmDirtyStage PEAK_REGENERATION =
    procgen::GreaterRealmDirtyStage::MountainPeaks
    | RELIEF_REGENERATION;

constexpr procgen::GreaterRealmDirtyStage CLASSIFICATION_REGENERATION =
    procgen::GreaterRealmDirtyStage::Classification
    | procgen::GreaterRealmDirtyStage::DebugImage
    | procgen::GreaterRealmDirtyStage::TextureUpload;

constexpr procgen::GreaterRealmDirtyStage DRAINAGE_REGENERATION =
    procgen::GreaterRealmDirtyStage::Drainage
    | procgen::GreaterRealmDirtyStage::RiverChannels
    | procgen::GreaterRealmDirtyStage::DebugImage
    | procgen::GreaterRealmDirtyStage::TextureUpload;

constexpr procgen::GreaterRealmDirtyStage CHANNEL_REGENERATION =
    procgen::GreaterRealmDirtyStage::RiverChannels
    | procgen::GreaterRealmDirtyStage::DebugImage
    | procgen::GreaterRealmDirtyStage::TextureUpload;

bool matches_clean_generation(
    const procgen::GreaterRealmMap& partial,
    const procgen::GreaterRealmGeneratorSettings& settings,
    const procgen::TerrainConstraintField& constraints
) {
    return maps_match(partial, procgen::generate_greater_realm(settings, constraints));
}

bool test_partial_regeneration_matches_clean_generation() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 8675309;
    settings.width = 112;
    settings.height = 84;
    procgen::TerrainConstraintField constraints(32, 24);
    procgen::GreaterRealmGenerationCache cache;
    procgen::GreaterRealmMap map;

    auto result = cache.regenerate(map, settings, constraints);
    bool ok = require(
        result.rebuilt_stages == FULL_REGENERATION,
        "initial generation rebuilds every dependent stage"
    );
    ok &= require(
        matches_clean_generation(map, settings, constraints),
        "initial cached generation matches a clean generation"
    );

    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == procgen::GreaterRealmDirtyStage::None,
        "unchanged settings perform no regeneration work"
    );
    cache.invalidate(procgen::GreaterRealmDirtyStage::DebugImage);
    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == (
            procgen::GreaterRealmDirtyStage::DebugImage
            | procgen::GreaterRealmDirtyStage::TextureUpload
        ),
        "debug-image invalidation requires only image rebuild and texture upload"
    );

    cache.invalidate(procgen::GreaterRealmDirtyStage::TextureUpload);
    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == procgen::GreaterRealmDirtyStage::TextureUpload,
        "texture-only invalidation does not rebuild the debug image"
    );

    settings.mountain_peak_radius += 8.0f;
    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == PEAK_REGENERATION,
        "peak-only settings reuse terrain fields"
    );
    ok &= require(
        matches_clean_generation(map, settings, constraints),
        "peak-only regeneration matches a clean generation"
    );

    settings.base_elevation_weight += 0.35f;
    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == RELIEF_REGENERATION,
        "relief-only settings reuse terrain fields and mountain peaks"
    );
    ok &= require(
        matches_clean_generation(map, settings, constraints),
        "relief-only regeneration matches a clean generation"
    );

    settings.highland_threshold += 0.03f;
    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == CLASSIFICATION_REGENERATION,
        "terrain-form thresholds skip drainage"
    );
    ok &= require(
        matches_clean_generation(map, settings, constraints),
        "classification-only regeneration matches a clean generation"
    );

    cache.invalidate(procgen::GreaterRealmDirtyStage::Drainage);
    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == DRAINAGE_REGENERATION,
        "explicit drainage invalidation reuses terrain and classification"
    );
    ok &= require(
        matches_clean_generation(map, settings, constraints),
        "drainage-only regeneration matches a clean generation"
    );

    settings.river_min_drainage_area -= 100.0f;
    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == CHANNEL_REGENERATION,
        "channel settings skip conditioned drainage"
    );
    ok &= require(
        matches_clean_generation(map, settings, constraints),
        "channel-only regeneration matches a clean generation"
    );

    constraints.paint(
        procgen::TerrainConstraintTool::Mountain,
        0.52f,
        0.46f,
        0.12f,
        0.75f
    );
    cache.invalidate(procgen::GreaterRealmDirtyStage::TerrainFields);
    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == FULL_REGENERATION,
        "authored-constraint changes rebuild the full map pipeline"
    );
    ok &= require(
        matches_clean_generation(map, settings, constraints),
        "painted-constraint regeneration matches a clean generation"
    );

    settings.sea_level += 0.05f;
    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == FULL_REGENERATION,
        "legacy sea-level setting remains a full-regeneration input"
    );

    cache.invalidate();
    result = cache.regenerate(map, settings, constraints);
    ok &= require(
        result.rebuilt_stages == FULL_REGENERATION,
        "forced regeneration rebuilds the complete pipeline"
    );
    return ok;
}

bool test_representative_256x192_timing_coverage() {
    procgen::GreaterRealmGeneratorSettings settings;
    settings.seed = 424242;
    settings.width = 256;
    settings.height = 192;
    procgen::TerrainConstraintField constraints(64, 48);
    procgen::GreaterRealmGenerationCache cache;
    procgen::GreaterRealmMap map;

    const auto full = cache.regenerate(map, settings, constraints);
    bool ok = require(
        full.rebuilt_stages == FULL_REGENERATION,
        "timed full path rebuilds all stages"
    );
    ok &= require(full.timings.total_ms > 0.0, "full path records total time");
    ok &= require(full.timings.terrain_fields_ms > 0.0, "full path records terrain-field time");
    ok &= require(full.timings.drainage_ms > 0.0, "full path records drainage time");

    settings.mountain_weight += 0.1f;
    const auto relief = cache.regenerate(map, settings, constraints);
    ok &= require(
        relief.rebuilt_stages == RELIEF_REGENERATION,
        "timed relief path has the expected dependencies"
    );
    ok &= require(relief.timings.total_ms > 0.0, "relief path records total time");
    ok &= require(relief.timings.terrain_fields_ms == 0.0, "relief path skips terrain fields");
    ok &= require(relief.timings.mountain_peaks_ms == 0.0, "relief path skips peak generation");
    ok &= require(relief.timings.relief_ms > 0.0, "relief path records relief time");

    settings.river_min_drainage_area += 100.0f;
    const auto channels = cache.regenerate(map, settings, constraints);
    ok &= require(
        channels.rebuilt_stages == CHANNEL_REGENERATION,
        "timed channel path has the expected dependencies"
    );
    ok &= require(channels.timings.total_ms > 0.0, "channel path records total time");
    ok &= require(channels.timings.terrain_fields_ms == 0.0, "channel path skips terrain fields");
    ok &= require(channels.timings.drainage_ms == 0.0, "channel path skips drainage");
    ok &= require(channels.timings.river_channels_ms > 0.0, "channel path records channel time");

    std::cout
        << "256x192 regeneration timings: full=" << full.timings.total_ms
        << "ms relief=" << relief.timings.total_ms
        << "ms channels=" << channels.timings.total_ms
        << "ms\n";
    return ok;
}

} // namespace

int main() {
    bool ok = true;
    ok &= test_partial_regeneration_matches_clean_generation();
    ok &= test_representative_256x192_timing_coverage();
    if (!ok) {
        return 1;
    }

    std::cout << "Greater realm staged regeneration tests passed.\n";
    return 0;
}

