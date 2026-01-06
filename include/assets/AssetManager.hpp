#pragma once

#include "../rendering/Texture.hpp"
#include "../audio/AudioSystem.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

namespace assets {

// Asset type enumeration
enum class AssetType : std::uint8_t {
    Texture,
    Audio,
    Data
};

// Generic asset handle
struct AssetHandle {
    AssetType type;
    std::uint32_t id;

    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return id != 0;
    }
};

// Asset manager - centralized loading and caching
class AssetManager {
public:
    explicit AssetManager(
        rendering::TextureManager* texture_manager,
        audio::AudioSystem* audio_system
    );

    ~AssetManager();

    // Non-copyable, movable
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) noexcept;
    AssetManager& operator=(AssetManager&&) noexcept;

    // Load texture
    [[nodiscard]] rendering::TextureID load_texture(const char* path);

    // Load audio clip
    [[nodiscard]] audio::AudioClipID load_audio(const char* path);

    // Load text/data file
    [[nodiscard]] std::optional<std::string> load_text_file(const char* path) const;

    // Unload specific asset
    void unload_texture(rendering::TextureID id) noexcept;
    void unload_audio(audio::AudioClipID id) noexcept;

    // Clear all cached assets
    void clear() noexcept;

    // Hot-reload support (check for file changes)
    void check_for_changes();

private:
    rendering::TextureManager* texture_manager_{nullptr};
    audio::AudioSystem* audio_system_{nullptr};

    // Track loaded assets for hot-reload
    std::unordered_map<std::string, rendering::TextureID> texture_paths_;
    std::unordered_map<std::string, audio::AudioClipID> audio_paths_;
};

} // namespace assets
