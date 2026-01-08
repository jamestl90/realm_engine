#pragma once

#include <cstdint>
#include <type_traits>
#include <functional>

namespace assets {

// Asset state enumeration
enum class AssetState : std::uint8_t {
    Unloaded,   // Not in memory
    Loading,    // Async load in progress (future)
    Loaded,     // Ready for use
    Failed      // Load failed
};

// Type-safe asset handle with generation for stale detection
template<typename T>
class AssetHandle {
public:
    using ValueType = T;

    constexpr AssetHandle() noexcept = default;

    constexpr AssetHandle(std::uint32_t id, std::uint32_t generation) noexcept
        : id_(id), generation_(generation) {}

    [[nodiscard]] constexpr std::uint32_t id() const noexcept { return id_; }
    [[nodiscard]] constexpr std::uint32_t generation() const noexcept { return generation_; }

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return id_ != 0;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return is_valid();
    }

    [[nodiscard]] constexpr bool operator==(const AssetHandle& other) const noexcept {
        return id_ == other.id_ && generation_ == other.generation_;
    }

    [[nodiscard]] constexpr bool operator!=(const AssetHandle& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] static constexpr AssetHandle invalid() noexcept {
        return AssetHandle{0, 0};
    }

private:
    std::uint32_t id_{0};
    std::uint32_t generation_{0};
};

// Forward declarations for asset types
struct TextureAsset;
struct AudioAsset;
struct FontAsset;
struct DataAsset;
struct AnimationAsset;
struct TilesetAsset;
struct PipelineAsset;

// Type aliases for common handles
using TextureHandle = AssetHandle<TextureAsset>;
using AudioHandle = AssetHandle<AudioAsset>;
using FontHandle = AssetHandle<FontAsset>;
using DataHandle = AssetHandle<DataAsset>;
using AnimationHandle = AssetHandle<AnimationAsset>;
using TilesetHandle = AssetHandle<TilesetAsset>;
using PipelineHandle = AssetHandle<PipelineAsset>;

} // namespace assets

// Hash support for AssetHandle
namespace std {
    template<typename T>
    struct hash<assets::AssetHandle<T>> {
        [[nodiscard]] std::size_t operator()(const assets::AssetHandle<T>& handle) const noexcept {
            return std::hash<std::uint64_t>{}(
                (static_cast<std::uint64_t>(handle.id()) << 32) | handle.generation()
            );
        }
    };
}
