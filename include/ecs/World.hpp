#pragma once

#include "Entity.hpp"
#include "Component.hpp"
#include "System.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <typeindex>
#include <functional>

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
        if (!is_valid(entity)) {
            return;
        }
        get_or_create_component_array<T>().insert(entity.id(), component);
    }

    template<Component T>
    void remove_component(Entity entity) {
        if (!is_valid(entity)) {
            return;
        }
        if (auto* array = get_component_array<T>()) {
            array->remove(entity.id());
        }
    }

    template<Component T>
    [[nodiscard]] T* get_component(Entity entity) noexcept {
        if (!is_valid(entity)) {
            return nullptr;
        }
        if (auto* array = get_component_array<T>()) {
            return array->get(entity.id());
        }
        return nullptr;
    }

    template<Component T>
    [[nodiscard]] const T* get_component(Entity entity) const noexcept {
        if (!is_valid(entity)) {
            return nullptr;
        }
        if (auto* array = get_component_array<T>()) {
            return array->get(entity.id());
        }
        return nullptr;
    }

    template<Component T>
    [[nodiscard]] bool has_component(Entity entity) const noexcept {
        if (!is_valid(entity)) {
            return false;
        }
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

    // Resource management
    template<typename T>
    void set_resource(T* resource) {
        resources_[std::type_index(typeid(T))] = {
            resource,
            [](void*) { /* Don't delete - resource is owned elsewhere */ }
        };
    }

    template<typename T>
    void remove_resource() {
        resources_.erase(std::type_index(typeid(T)));
    }

    template<typename T>
    [[nodiscard]] T* get_resource() noexcept {
        auto it = resources_.find(std::type_index(typeid(T)));
        return it != resources_.end() 
            ? static_cast<T*>(it->second.get()) 
            : nullptr;
    }

    template<typename T>
    [[nodiscard]] const T* get_resource() const noexcept {
        auto it = resources_.find(std::type_index(typeid(T)));
        return it != resources_.end() 
            ? static_cast<const T*>(it->second.get()) 
            : nullptr;
    }

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
        return dynamic_cast<ComponentArray<T>&>(*it->second);
    }

    void sort_systems();

    // Entity storage
    std::vector<std::uint32_t> free_indices_;
    std::vector<std::uint32_t> generations_;
    std::uint32_t entity_count_{0};

    std::unordered_map<ComponentTypeID, std::unique_ptr<IComponentArray>> component_arrays_;

    // System storage
    std::vector<std::unique_ptr<ISystem>> systems_;

    // Resource storage (type-erased)
    std::unordered_map<std::type_index, std::unique_ptr<void, std::function<void(void*)>>> resources_;
};

} // namespace ecs
