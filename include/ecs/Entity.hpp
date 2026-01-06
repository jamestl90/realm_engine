#pragma once

#include <cstdint>
#include <limits>

namespace ecs {

// Entity ID: 32-bit index + 32-bit generation for safe handle reuse
struct EntityID {
    std::uint32_t index{0};
    std::uint32_t generation{0};

    [[nodiscard]] constexpr bool operator==(const EntityID& other) const noexcept {
        return index == other.index && generation == other.generation;
    }

    [[nodiscard]] constexpr bool operator!=(const EntityID& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return index != std::numeric_limits<std::uint32_t>::max();
    }

    [[nodiscard]] static constexpr EntityID invalid() noexcept {
        return EntityID{std::numeric_limits<std::uint32_t>::max(), 0};
    }
};

// Entity handle - lightweight wrapper for type safety
class Entity {
public:
    constexpr explicit Entity(EntityID id = EntityID::invalid()) noexcept : id_(id) {}

    [[nodiscard]] constexpr EntityID id() const noexcept { return id_; }
    [[nodiscard]] constexpr bool is_valid() const noexcept { return id_.is_valid(); }

    [[nodiscard]] constexpr bool operator==(const Entity& other) const noexcept {
        return id_ == other.id_;
    }

    [[nodiscard]] constexpr bool operator!=(const Entity& other) const noexcept {
        return id_ != other.id_;
    }

private:
    EntityID id_;
};

} // namespace ecs

// Hash support for EntityID
namespace std {
    template<>
    struct hash<ecs::EntityID> {
        [[nodiscard]] std::size_t operator()(const ecs::EntityID& id) const noexcept {
            return std::hash<std::uint64_t>{}(
                (static_cast<std::uint64_t>(id.index) << 32) | id.generation
            );
        }
    };
}
