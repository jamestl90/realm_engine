#pragma once

#include "Texture.hpp"
#include <cstdint>

namespace rendering {

// Sprite flip flags
enum class SpriteFlip : std::uint8_t {
    None = 0,
    Horizontal = 1 << 0,
    Vertical = 1 << 1,
    Both = Horizontal | Vertical
};

inline SpriteFlip operator|(SpriteFlip a, SpriteFlip b) noexcept {
    return static_cast<SpriteFlip>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

inline SpriteFlip operator&(SpriteFlip a, SpriteFlip b) noexcept {
    return static_cast<SpriteFlip>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

// Sprite component - pure data for SoA storage
struct Sprite {
    TextureID texture_id{INVALID_TEXTURE_ID};
    std::uint16_t region_index{0}; // Index into atlas regions
    std::uint8_t layer{0};         // Render layer (0-255)
    SpriteFlip flip{SpriteFlip::None};
    float rotation{0.0f};          // Radians
    float scale_x{1.0f};
    float scale_y{1.0f};
    std::uint8_t r{255}, g{255}, b{255}, a{255}; // Tint color
};

// Transform component - position in world space
struct Transform {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f}; // Depth for sorting
};

// Animation state component
struct SpriteAnimation {
    std::uint16_t frame_start{0};
    std::uint16_t frame_count{0};
    std::uint16_t current_frame{0};
    float frame_duration{0.1f};  // Seconds per frame
    float elapsed{0.0f};
    bool looping{true};
    bool playing{true};
};

} // namespace rendering
