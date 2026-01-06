#pragma once

#include "Entity.hpp"
#include "Component.hpp"
#include "System.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace ecs {

// ECS World - coordinates entities, components, and systems
class World {
public:
    World();
    ~World();

    // Entity management
    [[nodiscard]] Entity create_entity();
    void destroy_entity(Entity entity);
    [[nodiscard]] bool is_valid(Entity entity) const noexcept;

    // Component management
    template<Component T>
    void add_component(Entity entity, const T& component) {
        get_or_create_component_array<T>().insert(entity.id(), component);
    }

    template<Component T>
    void remove_component(Entity entity) {
        if (auto* array = get_component_array<T>()) {
            array->remove(entity.id());
        }
    }

    template<Component T>
    [[nodiscard]] T* get_component(Entity entity) noexcept {
        if (auto* array = get_component_array<T>()) {
            return array->get(entity.id());
        }
        return nullptr;
    }

    template<Component T>
    [[nodiscard]] const T* get_component(Entity entity) const noexcept {
        if (auto* array = get_component_array<T>()) {
            return array->get(entity.id());
        }
        return nullptr;
    }

    template<Component T>
    [[nodiscard]] bool has_component(Entity entity) const noexcept {
        if (auto* array = get_component_array<T>()) {
            return array->has(entity.id());
        }
        return false;
    }

    // Get component array for iteration
    template<Component T>
    [[nodiscard]] ComponentArray<T>* get_component_array() noexcept {
        const auto type_id = detail::component_type_id<T>();
        auto it = component_arrays_.find(type_id);
        return it != component_arrays_.end() 
            ? static_cast<ComponentArray<T>*>(it->second.get()) 
            : nullptr;
    }

    template<Component T>
    [[nodiscard]] const ComponentArray<T>* get_component_array() const noexcept {
        const auto type_id = detail::component_type_id<T>();
        auto it = component_arrays_.find(type_id);
        return it != component_arrays_.end() 
            ? static_cast<const ComponentArray<T>*>(it->second.get()) 
            : nullptr;
    }

    // System management
    template<typename T, typename... Args>
    T* add_system(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = system.get();
        systems_.push_back(std::move(system));
        sort_systems();
        return ptr;
    }

    void remove_system(ISystem* system);

    // Update all enabled systems
    void update(float dt);

    // Clear all entities and components
    void clear();

private:
    template<Component T>
    ComponentArray<T>& get_or_create_component_array() {
        const auto type_id = detail::component_type_id<T>();
        auto it = component_arrays_.find(type_id);
        if (it == component_arrays_.end()) {
            auto array = std::make_unique<ComponentArray<T>>();
            auto* ptr = array.get();
            component_arrays_[type_id] = std::move(array);
            return *ptr;
        }
        return *static_cast<ComponentArray<T>*>(it->second.get());
    }

    void sort_systems();

    // Entity storage
    std::vector<std::uint32_t> free_indices_;
    std::vector<std::uint32_t> generations_;
    std::uint32_t entity_count_{0};

    // Component storage (type-erased)
    struct IComponentArray {
        virtual ~IComponentArray() = default;
    };

    template<Component T>
    struct ComponentArrayWrapper : IComponentArray {
        ComponentArray<T> array;
    };

    std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_arrays_;

    // System storage
    std::vector<std::unique_ptr<ISystem>> systems_;
};

} // namespace ecs
