#pragma once

#include "Entity.hpp"
#include "Component.hpp"
#include "System.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <typeindex>
#include <functional>
#include <tuple>
#include <utility>

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

    template<Component... Ts, typename Func>
        requires (sizeof...(Ts) >= 2)
    void each(Func&& callback) {
        each_impl<Ts...>(*this, std::forward<Func>(callback));
    }

    template<Component... Ts, typename Func>
        requires (sizeof...(Ts) >= 2)
    void each(Func&& callback) const {
        each_impl<Ts...>(*this, std::forward<Func>(callback));
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
    template<Component... Ts, typename WorldType, typename Func>
    static void each_impl(WorldType& world, Func&& callback) {
        auto arrays = std::tuple{world.template get_component_array<Ts>()...};
        const bool missing_array = std::apply(
            [](const auto*... array) { return ((array == nullptr) || ...); },
            arrays
        );
        if (missing_array) {
            return;
        }

        std::array<std::size_t, sizeof...(Ts)> sizes{};
        std::apply(
            [&sizes](const auto*... array) { sizes = {array->size()...}; },
            arrays
        );

        std::size_t smallest_index = 0;
        for (std::size_t i = 1; i < sizes.size(); ++i) {
            if (sizes[i] < sizes[smallest_index]) {
                smallest_index = i;
            }
        }

        auto iterate_array = [&]<std::size_t I>() {
            const auto* primary_array = std::get<I>(arrays);
            for (const EntityID entity_id : primary_array->entity_data()) {
                const Entity entity(entity_id);
                if (!world.is_valid(entity)) {
                    continue;
                }

                auto components = std::apply(
                    [entity_id](auto*... array) { return std::tuple{array->get(entity_id)...}; },
                    arrays
                );
                const bool has_all_components = std::apply(
                    [](const auto*... component) { return ((component != nullptr) && ...); },
                    components
                );
                if (!has_all_components) {
                    continue;
                }

                std::apply(
                    [&callback, entity](auto*... component) {
                        std::invoke(callback, entity, *component...);
                    },
                    components
                );
            }
        };

        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((smallest_index == Is ? iterate_array.template operator()<Is>() : void()), ...);
        }(std::index_sequence_for<Ts...>{});
    }

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
