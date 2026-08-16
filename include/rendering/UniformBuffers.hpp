#pragma once
#include <cstdint>

namespace rendering {

// All uniform buffer structs must be aligned for GPU memory layout
struct alignas(16) CameraData {
    float projection[16];  // 4x4 matrix, row-major for GPU
};

static_assert(sizeof(CameraData) % 16 == 0, "CameraData size must be 16-byte aligned");

struct alignas(16) TerrainViewData {
    float view_projection[16];
    float terrain_parameters[4]; // x: elevation scale
};

struct alignas(16) TerrainLightData {
    float direction_and_ambient[4];
};

static_assert(sizeof(TerrainViewData) % 16 == 0, "TerrainViewData size must be 16-byte aligned");
static_assert(sizeof(TerrainLightData) % 16 == 0, "TerrainLightData size must be 16-byte aligned");

} // namespace rendering
