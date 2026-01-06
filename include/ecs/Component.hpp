#pragma once

#include "Entity.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>
#include <concepts>

namespace ecs {

// Component concept: must be trivially copyable for SoA storage
template<typename T>
concept Component = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

// Component type ID - unique per component type
using ComponentTypeID = std::uint32_t;

namespace detail {
    // Generate unique component type IDs
    inline ComponentTypeID next_component_type_id() noexcept {
        static ComponentTypeID counter = 0;
        return counter++;
    }

    template<Component T>
    [[nodiscard]] inline ComponentTypeID component_type_id() noexcept {
        static const ComponentTypeID id = next_component_type_id();
        return id;
    }
}

// Structure-of-Arrays storage for a single component type
template<Component T>
class ComponentArray {
public:
    ComponentArray() = default;

    // Reserve capacity to avoid reallocations
    void reserve(std::size_t capacity) {
        components_.reserve(capacity);
        entities_.reserve(capacity);
        entity_to_index_.reserve(capacity);
    }

    // Add component for entity
    void insert(EntityID entity, const T& component) {
        const std::size_t index = components_.size();
        components_.push_back(component);
        entities_.push_back(entity);
        entity_to_index_[entity] = index;
    }

    // Remove component for entity
    void remove(EntityID entity) {
        auto it = entity_to_index_.find(entity);
        if (it == entity_to_index_.end()) return;

        const std::size_t index = it->second;
        const std::size_t last_index = components_.size() - 1;

        // Swap with last element and pop
        if (index != last_index) {
            components_[index] = components_[last_index];
            entities_[index] = entities_[last_index];
            entity_to_index_[entities_[last_index]] = index;
        }

        components_.pop_back();
        entities_.pop_back();
        entity_to_index_.erase(entity);
    }

    // Get component for entity
    [[nodiscard]] T* get(EntityID entity) noexcept {
        auto it = entity_to_index_.find(entity);
        return it != entity_to_index_.end() ? &components_[it->second] : nullptr;
    }

    [[nodiscard]] const T* get(EntityID entity) const noexcept {
        auto it = entity_to_index_.find(entity);
        return it != entity_to_index_.end() ? &components_[it->second] : nullptr;
    }

    // Check if entity has component
    [[nodiscard]] bool has(EntityID entity) const noexcept {
        return entity_to_index_.contains(entity);
    }

    // Direct array access for iteration
    [[nodiscard]] std::vector<T>& data() noexcept { return components_; }
    [[nodiscard]] const std::vector<T>& data() const noexcept { return components_; }
    [[nodiscard]] std::vector<EntityID>& entity_data() noexcept { return entities_; }
    [[nodiscard]] const std::vector<EntityID>& entity_data() const noexcept { return entities_; }

    [[nodiscard]] std::size_t size() const noexcept { return components_.size(); }
    [[nodiscard]] bool empty() const noexcept { return components_.empty(); }

    void clear() noexcept {
        components_.clear();
        entities_.clear();
        entity_to_index_.clear();
    }

private:
    std::vector<T> components_;                                    // SoA: component data
    std::vector<EntityID> entities_;                               // Parallel array: entity IDs
    std::unordered_map<EntityID, std::size_t> entity_to_index_;   // Fast lookup
};

} // namespace ecs
