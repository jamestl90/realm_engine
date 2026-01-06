#pragma once

#include <cstdint>

namespace ecs {

class World;

// System priority for execution order (lower = earlier)
using SystemPriority = std::uint32_t;

// Base interface for all systems
class ISystem {
public:
    virtual ~ISystem() = default;

    // Fixed timestep update
    virtual void update(World& world, float dt) = 0;

    // System priority for ordering
    [[nodiscard]] virtual SystemPriority priority() const noexcept { return 100; }

    // Enable/disable system
    virtual void set_enabled(bool enabled) noexcept { enabled_ = enabled; }
    [[nodiscard]] virtual bool is_enabled() const noexcept { return enabled_; }

protected:
    bool enabled_{true};
};

} // namespace ecs
