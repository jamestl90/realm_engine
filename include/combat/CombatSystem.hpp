#pragma once

#include "../ecs/System.hpp"
#include "../ecs/Entity.hpp"
#include <cstdint>
#include <vector>

namespace combat {

// Health component
struct Health {
    float current{100.0f};
    float maximum{100.0f};
    bool invulnerable{false};

    [[nodiscard]] bool is_alive() const noexcept {
        return current > 0.0f;
    }

    [[nodiscard]] float percentage() const noexcept {
        return maximum > 0.0f ? current / maximum : 0.0f;
    }
};

// Damage component - applied once then removed
struct Damage {
    float amount{0.0f};
    ecs::EntityID source{ecs::EntityID::invalid()};
    std::uint32_t damage_type{0}; // Bitmask for damage types
};

// Combat stats component
struct CombatStats {
    float attack_power{10.0f};
    float defense{0.0f};
    float attack_speed{1.0f};    // Attacks per second
    float attack_range{1.0f};
    float cooldown{0.0f};        // Current attack cooldown
};

// Target component - which entity to attack
struct Target {
    ecs::EntityID entity{ecs::EntityID::invalid()};
    float aggro_range{10.0f};

    [[nodiscard]] bool has_target() const noexcept {
        return entity.is_valid();
    }
};

// Combat event
struct CombatEvent {
    enum class Type : std::uint8_t {
        Damage,
        Heal,
        Death,
        Attack
    };

    Type type;
    ecs::EntityID source;
    ecs::EntityID target;
    float value;
};

// Combat system - handles damage, targeting, and combat logic
class CombatSystem : public ecs::ISystem {
public:
    CombatSystem() = default;

    void update(ecs::World& world, float dt) override;

    [[nodiscard]] SystemPriority priority() const noexcept override { return 60; }

    // Get combat events from last frame
    [[nodiscard]] const std::vector<CombatEvent>& get_events() const noexcept {
        return events_;
    }

    void clear_events() noexcept {
        events_.clear();
    }

private:
    void update_targeting(ecs::World& world);
    void update_attack_cooldowns(ecs::World& world, float dt);
    void process_attacks(ecs::World& world);
    void apply_damage(ecs::World& world);
    void remove_dead_entities(ecs::World& world);

    std::vector<CombatEvent> events_;
};

} // namespace combat
