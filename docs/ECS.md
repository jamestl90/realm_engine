# ECS Usage

This document describes the operational contracts of the current ECS API. The engine's broader technical direction remains in [ARCHITECTURE.md](ARCHITECTURE.md).

## Entities And Components

- Entities are generation-checked handles created and destroyed by `ecs::World`.
- Components must be trivially copyable, standard-layout data types.
- Re-adding an existing component replaces its value.
- Destroying an entity removes all of its components and invalidates stale handles.

## Component Queries

Use `World::each<Ts...>()` to process entities containing two or more required component types:

```cpp
world.each<Transform, Sprite>(
    [](ecs::Entity entity, Transform& transform, Sprite& sprite) {
        transform.x += 1.0f;
    }
);
```

- Mutable worlds provide mutable component references.
- Const worlds provide const component references.
- Queries iterate the smallest participating component array and perform no per-query heap allocation.
- Entities missing any requested component, and invalid entity generations, are skipped.
- Do not add, remove, or destroy queried components or entities inside the callback. Queue structural changes and apply them after iteration.

Direct component-array access remains available for specialized engine code, but ordinary multi-component iteration should use `each()`.
