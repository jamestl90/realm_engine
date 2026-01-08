#pragma once
#include <cstdint>

namespace rendering {

// All uniform buffer structs must be aligned for GPU memory layout
struct alignas(16) CameraData {
    float projection[16];  // 4x4 matrix, row-major for GPU
};

static_assert(sizeof(CameraData) % 16 == 0, "CameraData size must be 16-byte aligned");

} // namespace rendering
