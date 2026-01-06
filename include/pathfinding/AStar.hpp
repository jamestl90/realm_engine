#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include <functional>

namespace pathfinding {

// Grid position
struct GridPos {
    std::int32_t x{0};
    std::int32_t y{0};

    [[nodiscard]] constexpr bool operator==(const GridPos& other) const noexcept {
        return x == other.x && y == other.y;
    }
};

// Path result
using Path = std::vector<GridPos>;

// Grid-based A* pathfinding with DoD layout
class AStarGrid {
public:
    explicit AStarGrid(std::uint32_t width, std::uint32_t height);

    // Set cell walkability
    void set_walkable(std::int32_t x, std::int32_t y, bool walkable) noexcept;
    [[nodiscard]] bool is_walkable(std::int32_t x, std::int32_t y) const noexcept;

    // Set movement cost (default = 1.0)
    void set_cost(std::int32_t x, std::int32_t y, float cost) noexcept;
    [[nodiscard]] float get_cost(std::int32_t x, std::int32_t y) const noexcept;

    // Find path from start to goal
    [[nodiscard]] std::optional<Path> find_path(
        GridPos start,
        GridPos goal,
        bool allow_diagonal = true
    ) const;

    // Batch pathfinding for multiple requests
    [[nodiscard]] std::vector<std::optional<Path>> find_paths(
        const std::vector<std::pair<GridPos, GridPos>>& requests,
        bool allow_diagonal = true
    ) const;

    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }

private:
    [[nodiscard]] std::uint32_t to_index(std::int32_t x, std::int32_t y) const noexcept;
    [[nodiscard]] bool in_bounds(std::int32_t x, std::int32_t y) const noexcept;
    [[nodiscard]] float heuristic(GridPos a, GridPos b) const noexcept;

    std::uint32_t width_;
    std::uint32_t height_;
    std::vector<bool> walkable_;      // SoA: walkability flags
    std::vector<float> costs_;        // SoA: movement costs
};

} // namespace pathfinding

// Hash support for GridPos
namespace std {
    template<>
    struct hash<pathfinding::GridPos> {
        [[nodiscard]] std::size_t operator()(const pathfinding::GridPos& pos) const noexcept {
            return std::hash<std::uint64_t>{}(
                (static_cast<std::uint64_t>(pos.x) << 32) | static_cast<std::uint32_t>(pos.y)
            );
        }
    };
}
