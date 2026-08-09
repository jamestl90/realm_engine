#include "ecs/World.hpp"
#include <iostream>
#include <vector>

#if !defined(RFD_TEST_BUILD)
#error "ECS tests must be compiled through a test-only target"
#endif

namespace {

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

class RecordingSystem final : public ecs::ISystem {
public:
    RecordingSystem(std::vector<int>& updates, int id, ecs::SystemPriority priority)
        : updates_(updates), id_(id), priority_(priority) {}

    void update(ecs::World&, float dt) override {
        updates_.push_back(id_);
        last_dt_ = dt;
    }

    [[nodiscard]] ecs::SystemPriority priority() const noexcept override {
        return priority_;
    }

    [[nodiscard]] float last_dt() const noexcept {
        return last_dt_;
    }

private:
    std::vector<int>& updates_;
    int id_{0};
    ecs::SystemPriority priority_{100};
    float last_dt_{0.0f};
};

struct TestResource {
    int value{0};
};

bool test_systems_update_in_priority_order() {
    ecs::World world;
    std::vector<int> updates;

    auto* late = world.add_system<RecordingSystem>(updates, 3, 90);
    world.add_system<RecordingSystem>(updates, 1, 10);
    world.add_system<RecordingSystem>(updates, 2, 50);
    world.update(0.25f);

    bool ok = true;
    ok &= require(updates == std::vector<int>({1, 2, 3}), "systems update from lowest to highest priority");
    ok &= require(late->last_dt() == 0.25f, "world forwards the fixed timestep to systems");
    return ok;
}

bool test_disabled_systems_are_skipped() {
    ecs::World world;
    std::vector<int> updates;
    auto* system = world.add_system<RecordingSystem>(updates, 1, 10);

    system->set_enabled(false);
    world.update(0.1f);
    bool ok = require(updates.empty(), "disabled systems are not updated");

    system->set_enabled(true);
    world.update(0.1f);
    ok &= require(updates == std::vector<int>({1}), "re-enabled systems resume updating");
    return ok;
}

bool test_removed_systems_are_not_updated() {
    ecs::World world;
    std::vector<int> updates;
    auto* removed = world.add_system<RecordingSystem>(updates, 1, 10);
    world.add_system<RecordingSystem>(updates, 2, 20);

    world.remove_system(removed);
    world.remove_system(nullptr);
    world.update(0.1f);

    return require(updates == std::vector<int>({2}), "removed systems no longer update");
}

bool test_resource_lifecycle() {
    ecs::World world;
    TestResource first{11};
    TestResource replacement{22};

    bool ok = true;
    ok &= require(world.get_resource<TestResource>() == nullptr, "missing resources return null");

    world.set_resource(&first);
    ok &= require(world.get_resource<TestResource>() == &first, "set resource is returned by type");

    const ecs::World& const_world = world;
    ok &= require(const_world.get_resource<TestResource>() == &first, "const worlds can read resources");

    world.set_resource(&replacement);
    ok &= require(world.get_resource<TestResource>() == &replacement, "setting the same resource type replaces its pointer");
    ok &= require(first.value == 11, "resource replacement does not modify externally owned data");

    world.remove_resource<TestResource>();
    world.remove_resource<TestResource>();
    ok &= require(world.get_resource<TestResource>() == nullptr, "removed resources return null");
    return ok;
}

} // namespace

int main() {
    bool ok = true;
    ok &= test_systems_update_in_priority_order();
    ok &= test_disabled_systems_are_skipped();
    ok &= test_removed_systems_are_not_updated();
    ok &= test_resource_lifecycle();

    if (!ok) {
        return 1;
    }

    std::cout << "ECS system and resource tests passed.\n";
    return 0;
}
