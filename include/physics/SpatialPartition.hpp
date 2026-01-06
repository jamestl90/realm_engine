#pragma once

#include "../ecs/Entity.hpp"
#include "../rendering/Sprite.hpp"
#include "Collision.hpp"
#include <vector>
#include <cstdint>

namespace physics {

// Grid-based spatial hash for broad-phase collision detection
class SpatialGrid {
public:
    explicit SpatialGrid(float cell_size = 64.0f, std::uint32_t grid_width = 256, std::uint32_t grid_height = 256);

    // Clear grid
    void clear() noexcept;

    // Insert entity at position
    void insert(ecs::EntityID entity, float x, float y, float radius);

    // Query entities in radius
    [[nodiscard]] std::vector<ecs::EntityID> query_radius(float x, float y, float radius) const;

    // Query entities in AABB
    [[nodiscard]] std::vector<ecs::EntityID> query_aabb(float x, float y, float half_width, float half_height) const;

    // Get all potential collision pairs
    [[nodiscard]] std::vector<std::pair<ecs::EntityID, ecs::EntityID>> get_potential_pairs() const;

private:
    [[nodiscard]] std::uint32_t hash_position(float x, float y) const noexcept;
    [[nodiscard]] std::uint32_t get_cell_index(std::int32_t grid_x, std::int32_t grid_y) const noexcept;

    struct Cell {
        std::vector<ecs::EntityID> entities;
    };

    float cell_size_;
    float inv_cell_size_;
    std::uint32_t grid_width_;
    std::uint32_t grid_height_;
    std::vector<Cell> cells_;
};

// Collision system - performs broad and narrow phase collision detection
class CollisionSystem : public ecs::ISystem {
public:
    explicit CollisionSystem(float cell_size = 64.0f);

    void update(ecs::World& world, float dt) override;

    [[nodiscard]] SystemPriority priority() const noexcept override { return 50; }

    // Get collision results from last frame
    [[nodiscard]] const std::vector<CollisionResult>& get_collisions() const noexcept {
        return collisions_;
    }

private:
    void broad_phase(ecs::World& world);
    void narrow_phase(ecs::World& world);

    SpatialGrid spatial_grid_;
    std::vector<std::pair<ecs::EntityID, ecs::EntityID>> potential_pairs_;
    std::vector<CollisionResult> collisions_;
};

} // namespace physics
