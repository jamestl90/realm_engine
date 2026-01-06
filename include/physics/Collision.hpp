#pragma once

#include "../ecs/Entity.hpp"
#include <cstdint>

namespace physics {

// AABB collision component
struct AABB {
    float half_width{0.5f};
    float half_height{0.5f};
    std::uint32_t collision_mask{0xFFFFFFFF};  // Which layers to collide with
    std::uint32_t collision_layer{1};          // Which layer this belongs to
    bool is_static{false};                     // Static objects don't move
};

// Circle collision component
struct CircleCollider {
    float radius{0.5f};
    std::uint32_t collision_mask{0xFFFFFFFF};
    std::uint32_t collision_layer{1};
    bool is_static{false};
};

// Velocity component for physics
struct Velocity {
    float vx{0.0f};
    float vy{0.0f};
};

// Collision result
struct CollisionResult {
    ecs::EntityID entity_a;
    ecs::EntityID entity_b;
    float penetration_x{0.0f};
    float penetration_y{0.0f};
    float normal_x{0.0f};
    float normal_y{0.0f};
};

// Collision detection functions
[[nodiscard]] bool test_aabb_aabb(
    float ax, float ay, float ahw, float ahh,
    float bx, float by, float bhw, float bhh
) noexcept;

[[nodiscard]] bool test_circle_circle(
    float ax, float ay, float ar,
    float bx, float by, float br
) noexcept;

[[nodiscard]] bool test_aabb_circle(
    float aabb_x, float aabb_y, float aabb_hw, float aabb_hh,
    float circle_x, float circle_y, float circle_r
) noexcept;

} // namespace physics
