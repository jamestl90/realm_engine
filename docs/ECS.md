# ECS Feature Inventory

This document tracks Entity Component System features currently present in the engine. It is a capability snapshot, not the full ECS architecture.

## Core Types

- `EntityID`: 32-bit index plus 32-bit generation.
- `Entity`: lightweight handle wrapper around `EntityID`.
- Invalid entities use `std::numeric_limits<std::uint32_t>::max()` as the invalid index.
- Hash support exists for `EntityID`.

## Entity Management

- `World::create_entity()` creates a new entity or reuses an index from the free list.
- `World::destroy_entity()` invalidates existing handles by incrementing the entity generation and returning the index to the free list.
- `World::is_valid()` checks an entity handle against the current generation table.
- `World::clear()` removes systems, component arrays, entity generations, and free indices.

## Component Storage

- Components must satisfy the `Component` concept: trivially copyable and standard-layout.
- Component type IDs are generated per component type.
- `ComponentArray<T>` stores components and entity IDs in parallel vectors.
- Entity-to-component lookup uses an `unordered_map<EntityID, size_t>`.
- Component removal uses swap-with-last compaction.
- Direct component and entity array access is available for cache-friendly system iteration.
- Component arrays can reserve capacity manually.

## World Component API

- `add_component<T>()`
- `remove_component<T>()`
- `get_component<T>()`
- `has_component<T>()`
- `get_component_array<T>()`

## Systems

- Systems implement `ISystem::update(World&, float dt)`.
- Systems expose a numeric priority; lower priority runs earlier.
- Systems can be enabled or disabled.
- `World::add_system<T>()` owns systems with `std::unique_ptr` and sorts after insertion.
- `World::remove_system()` removes a system by pointer.
- `World::update()` runs all enabled systems in priority order.

## Resources

- `World` supports type-indexed non-owning resources.
- Resources can be set, removed, and fetched by type.
- The engine currently registers `TextureManager` as a world resource for rendering.

## Current Notes

- Destroying an entity invalidates the handle, but components are not currently removed from type-erased component arrays during `destroy_entity()`.
- There is no query/filter API beyond direct component-array access.
- There is no archetype storage or multi-component join helper yet.
- Component storage is SoA per component type, not archetype chunk storage.
- Systems are fixed-timestep update systems; rendering is handled separately by the renderer.
