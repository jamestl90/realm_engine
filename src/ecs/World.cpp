#include "ecs/World.hpp"
#include <algorithm>

namespace ecs {

World::World() = default;

World::~World() {
    clear();
}

Entity World::create_entity() {
    std::uint32_t index;
    
    if (!free_indices_.empty()) {
        // Reuse a free index
        index = free_indices_.back();
        free_indices_.pop_back();
    } else {
        // Allocate new index
        index = entity_count_++;
        generations_.push_back(0);
    }
    
    return Entity(EntityID{index, generations_[index]});
}

void World::destroy_entity(Entity entity) {
    const EntityID id = entity.id();
    
    if (!is_valid(entity)) {
        return;
    }
    
    // Remove all components for this entity
    for (auto& [type_id, array] : component_arrays_) {
        // Component arrays handle removal internally
        // This is type-erased, so we can't call remove directly
        // The actual removal will happen when systems process dead entities
    }
    
    // Increment generation to invalidate existing handles
    ++generations_[id.index];
    
    // Add to free list
    free_indices_.push_back(id.index);
}

bool World::is_valid(Entity entity) const noexcept {
    const EntityID id = entity.id();
    
    if (id.index >= generations_.size()) {
        return false;
    }
    
    return generations_[id.index] == id.generation;
}

void World::remove_system(ISystem* system) {
    auto it = std::find_if(systems_.begin(), systems_.end(),
        [system](const std::unique_ptr<ISystem>& ptr) {
            return ptr.get() == system;
        });
    
    if (it != systems_.end()) {
        systems_.erase(it);
    }
}

void World::update(float dt) {
    for (auto& system : systems_) {
        if (system->is_enabled()) {
            system->update(*this, dt);
        }
    }
}

void World::clear() {
    systems_.clear();
    component_arrays_.clear();
    // Don't clear resources - they are owned by Engine
    free_indices_.clear();
    generations_.clear();
    entity_count_ = 0;
}

void World::sort_systems() {
    std::sort(systems_.begin(), systems_.end(),
        [](const std::unique_ptr<ISystem>& a, const std::unique_ptr<ISystem>& b) {
            return a->priority() < b->priority();
        });
}

} // namespace ecs
