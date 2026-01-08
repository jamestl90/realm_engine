#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

namespace rendering {

// Texture handle
using TextureID = std::uint32_t;
constexpr TextureID INVALID_TEXTURE_ID = 0;

// Texture region for sprite atlases
struct TextureRegion {
    float u0{0.0f}, v0{0.0f}; // Top-left UV
    float u1{1.0f}, v1{1.0f}; // Bottom-right UV
    std::uint16_t width{0};
    std::uint16_t height{0};
};

// Texture resource
struct Texture {
    SDL_GPUTexture* gpu_texture{nullptr};
    std::uint16_t width{0};
    std::uint16_t height{0};
    TextureID id{INVALID_TEXTURE_ID};
};

} // namespace rendering
