#include "ecs/World.hpp"
#include <iostream>
#include <vector>

#if !defined(REALM_TEST_BUILD)
#error "ECS tests must be compiled through a test-only target"
#endif

namespace {

struct Position {
    float x{0.0f};
    float y{0.0f};
};

struct Velocity {
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

bool test_query_matches_required_components_from_smallest_array() {
    ecs::World world;
    const auto first = world.create_entity();
    const auto position_only = world.create_entity();
    const auto last = world.create_entity();

    world.add_component(first, Position{1.0f, 0.0f});
    world.add_component(position_only, Position{2.0f, 0.0f});
    world.add_component(last, Position{3.0f, 0.0f});

    world.add_component(last, Velocity{30.0f, 0.0f});
    world.add_component(first, Velocity{10.0f, 0.0f});

    std::vector<ecs::EntityID> matches;
    world.each<Position, Velocity>(
        [&matches](ecs::Entity entity, Position&, Velocity&) {
            matches.push_back(entity.id());
        }
    );

    bool ok = true;
    ok &= require(matches.size() == 2, "query visits only entities with every required component");
    if (matches.size() == 2) {
        ok &= require(matches[0] == last.id() && matches[1] == first.id(), "query follows the smallest component array");
    }
    return ok;
}

bool test_mutable_query_updates_components() {
    ecs::World world;
    const auto entity = world.create_entity();
    world.add_component(entity, Position{4.0f, 5.0f});
    world.add_component(entity, Velocity{2.0f, 3.0f});

    world.each<Position, Velocity>(
        [](ecs::Entity, Position& position, Velocity& velocity) {
            position.x += velocity.x;
            position.y += velocity.y;
            velocity.x = 0.0f;
        }
    );

    const auto* position = world.get_component<Position>(entity);
    const auto* velocity = world.get_component<Velocity>(entity);
    return require(position && position->x == 6.0f && position->y == 8.0f, "mutable query updates component values")
        && require(velocity && velocity->x == 0.0f, "mutable query exposes every requested component");
}

bool test_const_query_provides_read_only_components() {
    ecs::World world;
    const auto entity = world.create_entity();
    world.add_component(entity, Position{7.0f, 8.0f});
    world.add_component(entity, Velocity{1.0f, 2.0f});

    const ecs::World& const_world = world;
    float total = 0.0f;
    const_world.each<Position, Velocity>(
        [&total](ecs::Entity, const Position& position, const Velocity& velocity) {
            total = position.x + position.y + velocity.x + velocity.y;
        }
    );

    return require(total == 18.0f, "const query reads const component references");
}

bool test_three_component_query_and_missing_array() {
    ecs::World world;
    const auto complete = world.create_entity();
    const auto incomplete = world.create_entity();
    world.add_component(complete, Position{1.0f, 0.0f});
    world.add_component(complete, Velocity{2.0f, 0.0f});
    world.add_component(complete, Marker{3});
    world.add_component(incomplete, Position{4.0f, 0.0f});
    world.add_component(incomplete, Velocity{5.0f, 0.0f});

    int three_component_matches = 0;
    world.each<Position, Velocity, Marker>(
        [&three_component_matches](ecs::Entity, Position&, Velocity&, Marker&) {
            ++three_component_matches;
        }
    );

    struct Missing {
        int value{0};
    };
    int missing_matches = 0;
    world.each<Position, Missing>(
        [&missing_matches](ecs::Entity, Position&, Missing&) {
            ++missing_matches;
        }
    );

    return require(three_component_matches == 1, "query supports three required component types")
        && require(missing_matches == 0, "query returns immediately when a component array is absent");
}

} // namespace

int main() {
    bool ok = true;
    ok &= test_query_matches_required_components_from_smallest_array();
    ok &= test_mutable_query_updates_components();
    ok &= test_const_query_provides_read_only_components();
    ok &= test_three_component_query_and_missing_array();

    if (!ok) {
        return 1;
    }

    std::cout << "ECS query tests passed.\n";
    return 0;
}
