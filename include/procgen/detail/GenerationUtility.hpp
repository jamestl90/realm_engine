#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace procgen::detail {

inline constexpr float SQRT_TWO = 1.41421356237f;

inline constexpr std::array<std::array<std::int32_t, 2>, 8> EIGHT_WAY_NEIGHBORS{{
    {{-1, -1}}, {{0, -1}}, {{1, -1}}, {{-1, 0}},
    {{1, 0}}, {{-1, 1}}, {{0, 1}}, {{1, 1}}
}};

[[nodiscard]] inline float clamp01(float value) noexcept {
    return std::clamp(value, 0.0f, 1.0f);
}

[[nodiscard]] inline float lerp(float from, float to, float amount) noexcept {
    return from + (to - from) * amount;
}

[[nodiscard]] inline float smoothstep01(float value) noexcept {
    const float clamped = clamp01(value);
    return clamped * clamped * (3.0f - 2.0f * clamped);
}

[[nodiscard]] inline float smoothstep(float edge0, float edge1, float value) noexcept {
    if (edge0 == edge1) {
        return value < edge0 ? 0.0f : 1.0f;
    }

    return smoothstep01((value - edge0) / (edge1 - edge0));
}

[[nodiscard]] inline std::uint64_t mix_hash(
    std::uint64_t hash,
    std::uint64_t value
) noexcept {
    hash ^= value + 0x9e3779b97f4a7c15ull + (hash << 6) + (hash >> 2);
    hash ^= hash >> 30;
    hash *= 0xbf58476d1ce4e5b9ull;
    hash ^= hash >> 27;
    hash *= 0x94d049bb133111ebull;
    return hash ^ (hash >> 31);
}

[[nodiscard]] inline std::uint64_t hash_grid_coordinate(
    std::uint64_t seed,
    std::uint32_t x,
    std::uint32_t y,
    std::uint64_t salt
) noexcept {
    std::uint64_t value = seed ^ salt;
    value ^= static_cast<std::uint64_t>(x)
        + 0x9e3779b97f4a7c15ull
        + (value << 6)
        + (value >> 2);
    value ^= static_cast<std::uint64_t>(y)
        + 0xbf58476d1ce4e5b9ull
        + (value << 6)
        + (value >> 2);
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ull;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebull;
    value ^= value >> 31;
    return value;
}

[[nodiscard]] inline std::uint64_t hash_grid_coordinate(
    std::uint64_t seed,
    std::int32_t x,
    std::int32_t y,
    std::uint64_t salt
) noexcept {
    return hash_grid_coordinate(
        seed,
        static_cast<std::uint32_t>(x),
        static_cast<std::uint32_t>(y),
        salt
    );
}

[[nodiscard]] inline float random01_from_hash(std::uint64_t hash) noexcept {
    return static_cast<float>((hash >> 40) & 0xffffffu) / static_cast<float>(0xffffffu);
}

[[nodiscard]] inline float grid_random01(
    std::uint64_t seed,
    std::uint32_t x,
    std::uint32_t y,
    std::uint64_t salt
) noexcept {
    return random01_from_hash(hash_grid_coordinate(seed, x, y, salt));
}

[[nodiscard]] inline float grid_random01(
    std::uint64_t seed,
    std::int32_t x,
    std::int32_t y,
    std::uint64_t salt
) noexcept {
    return random01_from_hash(hash_grid_coordinate(seed, x, y, salt));
}

[[nodiscard]] inline std::uint64_t mixed_hash_grid_coordinate(
    std::uint64_t seed,
    std::int32_t x,
    std::int32_t y,
    std::uint64_t salt
) noexcept {
    std::uint64_t hash = mix_hash(seed, salt);
    hash = mix_hash(hash, static_cast<std::uint32_t>(x));
    return mix_hash(hash, static_cast<std::uint32_t>(y));
}

[[nodiscard]] inline float mixed_grid_random01(
    std::uint64_t seed,
    std::int32_t x,
    std::int32_t y,
    std::uint64_t salt
) noexcept {
    return random01_from_hash(mixed_hash_grid_coordinate(seed, x, y, salt));
}

[[nodiscard]] inline float mixed_value_noise(
    std::uint64_t seed,
    float x,
    float y,
    float frequency,
    std::uint64_t salt
) noexcept {
    const float sample_x = x * frequency;
    const float sample_y = y * frequency;
    const auto x0 = static_cast<std::int32_t>(std::floor(sample_x));
    const auto y0 = static_cast<std::int32_t>(std::floor(sample_y));
    const float tx = smoothstep01(sample_x - static_cast<float>(x0));
    const float ty = smoothstep01(sample_y - static_cast<float>(y0));
    const float low = lerp(
        mixed_grid_random01(seed, x0, y0, salt),
        mixed_grid_random01(seed, x0 + 1, y0, salt),
        tx
    );
    const float high = lerp(
        mixed_grid_random01(seed, x0, y0 + 1, salt),
        mixed_grid_random01(seed, x0 + 1, y0 + 1, salt),
        tx
    );
    return lerp(low, high, ty);
}

} // namespace procgen::detail
