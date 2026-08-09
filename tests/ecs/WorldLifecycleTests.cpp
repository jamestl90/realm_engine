#include "ecs/World.hpp"
#include <iostream>

#if !defined(RFD_TEST_BUILD)
#error "ECS tests must be compiled through a test-only target"
#endif

namespace {

struct Position {
    float x{0.0f};
    float y{0.0f};
};

struct Marker {
    std::uint32_t value{0};
};

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool test_destroy_removes_all_components() {
    ecs::World world;
    const auto entity = world.create_entity();
    world.add_component(entity, Position{12.0f, 24.0f});
    world.add_component(entity, Marker{7});

    world.destroy_entity(entity);

    const auto* positions = world.get_component_array<Position>();
    const auto* markers = world.get_component_array<Marker>();

    bool ok = true;
    ok &= require(!world.is_valid(entity), "destroyed entity handle is invalidated");
    ok &= require(positions && positions->empty(), "destroy removes the entity's Position component");
    ok &= require(markers && markers->empty(), "destroy removes the entity's Marker component");
    ok &= require(world.get_component<Position>(entity) == nullptr, "stale handles cannot access components");
    ok &= require(!world.has_component<Marker>(entity), "stale handles do not report components");
    return ok;
}

bool test_duplicate_add_replaces_component() {
    ecs::World world;
    const auto entity = world.create_entity();
    world.add_component(entity, Position{1.0f, 2.0f});
    world.add_component(entity, Position{3.0f, 4.0f});

    const auto* positions = world.get_component_array<Position>();
    const auto* position = world.get_component<Position>(entity);

    bool ok = true;
    ok &= require(positions && positions->size() == 1, "re-adding a component does not duplicate storage");
    ok &= require(position && position->x == 3.0f && position->y == 4.0f, "re-adding a component replaces its value");
    return ok;
}

bool test_stale_handle_rejected_after_index_reuse() {
    ecs::World world;
    const auto stale = world.create_entity();
    world.add_component(stale, Marker{11});
    world.destroy_entity(stale);

    const auto replacement = world.create_entity();
    world.add_component(replacement, Marker{22});
    world.add_component(stale, Marker{33});
    world.remove_component<Marker>(stale);

    const auto* marker = world.get_component<Marker>(replacement);
    const auto* markers = world.get_component_array<Marker>();

    bool ok = true;
    ok &= require(replacement.id().index == stale.id().index, "destroyed entity index is reused");
    ok &= require(replacement.id().generation != stale.id().generation, "reused entity receives a new generation");
    ok &= require(!world.is_valid(stale), "old generation remains invalid after index reuse");
    ok &= require(world.get_component<Marker>(stale) == nullptr, "stale generation cannot read replacement components");
    ok &= require(marker && marker->value == 22, "stale writes and removals do not affect the replacement entity");
    ok &= require(markers && markers->size() == 1, "stale writes do not grow component storage");
    return ok;
}

bool test_component_removal_preserves_compacted_lookup() {
    ecs::World world;
    const auto first = world.create_entity();
    const auto middle = world.create_entity();
    const auto last = world.create_entity();
    world.add_component(first, Marker{1});
    world.add_component(middle, Marker{2});
    world.add_component(last, Marker{3});

    world.remove_component<Marker>(middle);

    const auto* markers = world.get_component_array<Marker>();
    const auto* first_marker = world.get_component<Marker>(first);
    const auto* last_marker = world.get_component<Marker>(last);

    bool ok = true;
    ok &= require(markers && markers->size() == 2, "component removal compacts storage");
    ok &= require(!world.has_component<Marker>(middle), "removed component is absent");
    ok &= require(first_marker && first_marker->value == 1, "first component remains addressable after compaction");
    ok &= require(last_marker && last_marker->value == 3, "moved component lookup is repaired after compaction");
    return ok;
}

bool test_invalid_operations_are_harmless() {
    ecs::World world;
    const ecs::Entity invalid;
    world.add_component(invalid, Marker{1});
    world.remove_component<Marker>(invalid);
    world.destroy_entity(invalid);

    const auto first = world.create_entity();
    world.destroy_entity(first);
    world.destroy_entity(first);
    const auto reused = world.create_entity();
    const auto next = world.create_entity();

    bool ok = true;
    ok &= require(world.get_component_array<Marker>() == nullptr, "invalid add does not create component storage");
    ok &= require(reused.id().index == first.id().index, "valid destruction recycles its index once");
    ok &= require(next.id().index != reused.id().index, "repeated destruction does not duplicate a free index");
    return ok;
}

} // namespace

int main() {
    bool ok = true;
    ok &= test_destroy_removes_all_components();
    ok &= test_duplicate_add_replaces_component();
    ok &= test_stale_handle_rejected_after_index_reuse();
    ok &= test_component_removal_preserves_compacted_lookup();
    ok &= test_invalid_operations_are_harmless();

    if (!ok) {
        return 1;
    }

    std::cout << "ECS world lifecycle tests passed.\n";
    return 0;
}
