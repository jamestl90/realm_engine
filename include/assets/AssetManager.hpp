#pragma once

#include "AssetHandle.hpp"
#include "AssetTypes.hpp"
#include "../rendering/Texture.hpp"
#include "../rendering/TextureManager.hpp"
#include "../rendering/PipelineManager.hpp"
#include "../audio/AudioSystem.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>
#include <vector>
#include <filesystem>

namespace assets {

// Load priority for async loading (future use)
enum class LoadPriority : std::uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Immediate = 3  // Synchronous load
};

// Callback for batch loading completion
using LoadCallback = std::function<void(bool success)>;

// Callback for hot reload notifications
using ReloadCallback = std::function<void(const std::string& path)>;

// Asset cache entry with reference counting
template<typename T>
struct AssetEntry {
    std::unique_ptr<T> asset;
    std::uint32_t generation{0};
    std::uint32_t ref_count{0};
    AssetState state{AssetState::Unloaded};
    std::string path;
    std::uint64_t last_modified{0};
    bool pinned{false};
};

// Asset manager - centralised loading and caching
class AssetManager {
public:
    explicit AssetManager(
        rendering::TextureManager* texture_manager,
        audio::AudioSystem* audio_system,
        rendering::PipelineManager* pipeline_manager = nullptr
    );

    ~AssetManager();

    // Non-copyable, movable
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(AssetManager&&) noexcept;
    AssetManager& operator=(AssetManager&&) noexcept;

    // ===== Texture Loading =====
    [[nodiscard]] TextureHandle load_texture(const char* path, LoadPriority priority = LoadPriority::Normal);
    [[nodiscard]] TextureAsset* get_texture(TextureHandle handle) noexcept;
    [[nodiscard]] const TextureAsset* get_texture(TextureHandle handle) const noexcept;
    void unload_texture(TextureHandle handle) noexcept;

    // ===== Audio Loading =====
    [[nodiscard]] AudioHandle load_audio(const char* path, LoadPriority priority = LoadPriority::Normal);
    [[nodiscard]] AudioAsset* get_audio(AudioHandle handle) noexcept;
    [[nodiscard]] const AudioAsset* get_audio(AudioHandle handle) const noexcept;
    void unload_audio(AudioHandle handle) noexcept;

    // ===== Font Loading =====
    [[nodiscard]] FontHandle load_font(const char* path, float size = 16.0f, LoadPriority priority = LoadPriority::Normal);
    [[nodiscard]] FontAsset* get_font(FontHandle handle) noexcept;
    [[nodiscard]] const FontAsset* get_font(FontHandle handle) const noexcept;
    void unload_font(FontHandle handle) noexcept;

    // ===== Animation Loading =====
    [[nodiscard]] AnimationHandle load_animation(const char* path, LoadPriority priority = LoadPriority::Normal);
    [[nodiscard]] AnimationAsset* get_animation(AnimationHandle handle) noexcept;
    [[nodiscard]] const AnimationAsset* get_animation(AnimationHandle handle) const noexcept;
    void unload_animation(AnimationHandle handle) noexcept;

    // ===== Data Loading =====
    [[nodiscard]] DataHandle load_data(const char* path, LoadPriority priority = LoadPriority::Normal);
    [[nodiscard]] DataAsset* get_data(DataHandle handle) noexcept;
    [[nodiscard]] const DataAsset* get_data(DataHandle handle) const noexcept;
    void unload_data(DataHandle handle) noexcept;

    // ===== Tileset Loading =====
    [[nodiscard]] TilesetHandle load_tileset(const char* path, LoadPriority priority = LoadPriority::Normal);
    [[nodiscard]] TilesetAsset* get_tileset(TilesetHandle handle) noexcept;
    [[nodiscard]] const TilesetAsset* get_tileset(TilesetHandle handle) const noexcept;
    void unload_tileset(TilesetHandle handle) noexcept;

    // ===== Pipeline Loading =====
    [[nodiscard]] PipelineHandle load_pipeline(const char* path, LoadPriority priority = LoadPriority::Normal);
    [[nodiscard]] PipelineHandle load_pipeline_from_config(
        const char* name,
        rendering::PipelineType base_type,
        const rendering::PipelineConfig& config
    );
    [[nodiscard]] PipelineAsset* get_pipeline(PipelineHandle handle) noexcept;
    [[nodiscard]] const PipelineAsset* get_pipeline(PipelineHandle handle) const noexcept;
    void unload_pipeline(PipelineHandle handle) noexcept;

    // Get the underlying GPU pipeline handle for rendering
    [[nodiscard]] SDL_GPUGraphicsPipeline* get_gpu_pipeline(PipelineHandle handle) const noexcept;

    // Get core pipeline directly from PipelineManager
    [[nodiscard]] SDL_GPUGraphicsPipeline* get_core_pipeline(rendering::PipelineType type) const noexcept;

    // ===== Raw File Loading =====
    [[nodiscard]] std::optional<std::string> load_text_file(const char* path) const;
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> load_binary_file(const char* path) const;

    // ===== Handle Queries =====
    template<typename T>
    [[nodiscard]] bool is_loaded(AssetHandle<T> handle) const noexcept;

    template<typename T>
    [[nodiscard]] AssetState get_state(AssetHandle<T> handle) const noexcept;

    // ===== Lifetime Management =====
    template<typename T>
    void pin(AssetHandle<T> handle) noexcept;

    template<typename T>
    void unpin(AssetHandle<T> handle) noexcept;

    template<typename T>
    void add_ref(AssetHandle<T> handle) noexcept;

    template<typename T>
    void release_ref(AssetHandle<T> handle) noexcept;

    // ===== Batch Loading =====
    void preload_manifest(const char* manifest_path);
    void load_batch(const std::vector<std::string>& paths, LoadCallback on_complete);

    // ===== Hot Reload =====
    void set_hot_reload_enabled(bool enable) noexcept;
    [[nodiscard]] bool is_hot_reload_enabled() const noexcept { return hot_reload_enabled_; }
    void poll_hot_reload();
    void register_reload_callback(ReloadCallback callback);

    template<typename T>
    void force_reload(AssetHandle<T> handle);

    // ===== Memory Management =====
    void collect_unused(float grace_period_seconds = 5.0f);
    void clear() noexcept;

    // ===== Statistics =====
    [[nodiscard]] std::size_t texture_count() const noexcept { return textures_.size(); }
    [[nodiscard]] std::size_t audio_count() const noexcept { return audio_clips_.size(); }
    [[nodiscard]] std::size_t font_count() const noexcept { return fonts_.size(); }
    [[nodiscard]] std::size_t animation_count() const noexcept { return animations_.size(); }
    [[nodiscard]] std::size_t data_count() const noexcept { return data_assets_.size(); }
    [[nodiscard]] std::size_t tileset_count() const noexcept { return tilesets_.size(); }
    [[nodiscard]] std::size_t pipeline_count() const noexcept { return pipelines_.size(); }

    // ===== Path Management =====
    void set_base_path(const char* path);
    [[nodiscard]] const std::string& base_path() const noexcept { return base_path_; }
    [[nodiscard]] std::string resolve_path(const char* relative_path) const;

    // ===== Pipeline Manager Access =====
    [[nodiscard]] rendering::PipelineManager* pipeline_manager() noexcept { return pipeline_manager_; }
    [[nodiscard]] const rendering::PipelineManager* pipeline_manager() const noexcept { return pipeline_manager_; }

private:
    // Internal loading functions
    [[nodiscard]] bool load_texture_internal(const std::string& path, TextureAsset& asset);
    [[nodiscard]] bool load_audio_internal(const std::string& path, AudioAsset& asset);
    [[nodiscard]] bool load_font_internal(const std::string& path, float size, FontAsset& asset);
    [[nodiscard]] bool load_animation_internal(const std::string& path, AnimationAsset& asset);
    [[nodiscard]] bool load_data_internal(const std::string& path, DataAsset& asset);
    [[nodiscard]] bool load_tileset_internal(const std::string& path, TilesetAsset& asset);
    [[nodiscard]] bool load_pipeline_internal(const std::string& path, PipelineAsset& asset);

    // Path normalisation
    [[nodiscard]] std::string normalise_path(const char* path) const;

    // File modification time
    [[nodiscard]] std::uint64_t get_file_modified_time(const std::string& path) const;

    // Check if file has been modified
    [[nodiscard]] bool has_file_changed(const std::string& path, std::uint64_t last_modified) const;

    // External system references
    rendering::TextureManager* texture_manager_{nullptr};
    audio::AudioSystem* audio_system_{nullptr};
    rendering::PipelineManager* pipeline_manager_{nullptr};

    // Asset caches
    std::unordered_map<std::uint32_t, AssetEntry<TextureAsset>> textures_;
    std::unordered_map<std::uint32_t, AssetEntry<AudioAsset>> audio_clips_;
    std::unordered_map<std::uint32_t, AssetEntry<FontAsset>> fonts_;
    std::unordered_map<std::uint32_t, AssetEntry<AnimationAsset>> animations_;
    std::unordered_map<std::uint32_t, AssetEntry<DataAsset>> data_assets_;
    std::unordered_map<std::uint32_t, AssetEntry<TilesetAsset>> tilesets_;
    std::unordered_map<std::uint32_t, AssetEntry<PipelineAsset>> pipelines_;

    // Path to handle lookup
    std::unordered_map<std::string, TextureHandle> texture_path_to_handle_;
    std::unordered_map<std::string, AudioHandle> audio_path_to_handle_;
    std::unordered_map<std::string, FontHandle> font_path_to_handle_;
    std::unordered_map<std::string, AnimationHandle> animation_path_to_handle_;
    std::unordered_map<std::string, DataHandle> data_path_to_handle_;
    std::unordered_map<std::string, TilesetHandle> tileset_path_to_handle_;
    std::unordered_map<std::string, PipelineHandle> pipeline_path_to_handle_;

    // ID generation
    std::uint32_t next_texture_id_{1};
    std::uint32_t next_audio_id_{1};
    std::uint32_t next_font_id_{1};
    std::uint32_t next_animation_id_{1};
    std::uint32_t next_data_id_{1};
    std::uint32_t next_tileset_id_{1};
    std::uint32_t next_pipeline_id_{1};

    // Hot reload
    bool hot_reload_enabled_{false};
    std::vector<ReloadCallback> reload_callbacks_;

    // Base path for asset resolution
    std::string base_path_{"assets/"};

    // Fallback assets
    std::unique_ptr<TextureAsset> fallback_texture_;
    std::unique_ptr<FontAsset> fallback_font_;
};

// ===== Template Implementations =====

template<typename T>
bool AssetManager::is_loaded(AssetHandle<T> handle) const noexcept {
    return get_state(handle) == AssetState::Loaded;
}

template<>
inline AssetState AssetManager::get_state(TextureHandle handle) const noexcept {
    auto it = textures_.find(handle.id());
    if (it == textures_.end() || it->second.generation != handle.generation()) {
        return AssetState::Unloaded;
    }
    return it->second.state;
}

template<>
inline AssetState AssetManager::get_state(AudioHandle handle) const noexcept {
    auto it = audio_clips_.find(handle.id());
    if (it == audio_clips_.end() || it->second.generation != handle.generation()) {
        return AssetState::Unloaded;
    }
    return it->second.state;
}

template<>
inline AssetState AssetManager::get_state(FontHandle handle) const noexcept {
    auto it = fonts_.find(handle.id());
    if (it == fonts_.end() || it->second.generation != handle.generation()) {
        return AssetState::Unloaded;
    }
    return it->second.state;
}

template<>
inline AssetState AssetManager::get_state(AnimationHandle handle) const noexcept {
    auto it = animations_.find(handle.id());
    if (it == animations_.end() || it->second.generation != handle.generation()) {
        return AssetState::Unloaded;
    }
    return it->second.state;
}

template<>
inline AssetState AssetManager::get_state(DataHandle handle) const noexcept {
    auto it = data_assets_.find(handle.id());
    if (it == data_assets_.end() || it->second.generation != handle.generation()) {
        return AssetState::Unloaded;
    }
    return it->second.state;
}

template<>
inline AssetState AssetManager::get_state(TilesetHandle handle) const noexcept {
    auto it = tilesets_.find(handle.id());
    if (it == tilesets_.end() || it->second.generation != handle.generation()) {
        return AssetState::Unloaded;
    }
    return it->second.state;
}

template<>
inline AssetState AssetManager::get_state(PipelineHandle handle) const noexcept {
    auto it = pipelines_.find(handle.id());
    if (it == pipelines_.end() || it->second.generation != handle.generation()) {
        return AssetState::Unloaded;
    }
    return it->second.state;
}

template<>
inline void AssetManager::pin(TextureHandle handle) noexcept {
    auto it = textures_.find(handle.id());
    if (it != textures_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = true;
    }
}

template<>
inline void AssetManager::pin(AudioHandle handle) noexcept {
    auto it = audio_clips_.find(handle.id());
    if (it != audio_clips_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = true;
    }
}

template<>
inline void AssetManager::pin(FontHandle handle) noexcept {
    auto it = fonts_.find(handle.id());
    if (it != fonts_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = true;
    }
}

template<>
inline void AssetManager::pin(AnimationHandle handle) noexcept {
    auto it = animations_.find(handle.id());
    if (it != animations_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = true;
    }
}

template<>
inline void AssetManager::pin(DataHandle handle) noexcept {
    auto it = data_assets_.find(handle.id());
    if (it != data_assets_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = true;
    }
}

template<>
inline void AssetManager::pin(TilesetHandle handle) noexcept {
    auto it = tilesets_.find(handle.id());
    if (it != tilesets_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = true;
    }
}

template<>
inline void AssetManager::pin(PipelineHandle handle) noexcept {
    auto it = pipelines_.find(handle.id());
    if (it != pipelines_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = true;
    }
}

template<>
inline void AssetManager::unpin(TextureHandle handle) noexcept {
    auto it = textures_.find(handle.id());
    if (it != textures_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = false;
    }
}

template<>
inline void AssetManager::unpin(AudioHandle handle) noexcept {
    auto it = audio_clips_.find(handle.id());
    if (it != audio_clips_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = false;
    }
}

template<>
inline void AssetManager::unpin(FontHandle handle) noexcept {
    auto it = fonts_.find(handle.id());
    if (it != fonts_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = false;
    }
}

template<>
inline void AssetManager::unpin(AnimationHandle handle) noexcept {
    auto it = animations_.find(handle.id());
    if (it != animations_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = false;
    }
}

template<>
inline void AssetManager::unpin(DataHandle handle) noexcept {
    auto it = data_assets_.find(handle.id());
    if (it != data_assets_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = false;
    }
}

template<>
inline void AssetManager::unpin(TilesetHandle handle) noexcept {
    auto it = tilesets_.find(handle.id());
    if (it != tilesets_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = false;
    }
}

template<>
inline void AssetManager::unpin(PipelineHandle handle) noexcept {
    auto it = pipelines_.find(handle.id());
    if (it != pipelines_.end() && it->second.generation == handle.generation()) {
        it->second.pinned = false;
    }
}

template<>
inline void AssetManager::add_ref(TextureHandle handle) noexcept {
    auto it = textures_.find(handle.id());
    if (it != textures_.end() && it->second.generation == handle.generation()) {
        ++it->second.ref_count;
    }
}

template<>
inline void AssetManager::add_ref(AudioHandle handle) noexcept {
    auto it = audio_clips_.find(handle.id());
    if (it != audio_clips_.end() && it->second.generation == handle.generation()) {
        ++it->second.ref_count;
    }
}

template<>
inline void AssetManager::add_ref(FontHandle handle) noexcept {
    auto it = fonts_.find(handle.id());
    if (it != fonts_.end() && it->second.generation == handle.generation()) {
        ++it->second.ref_count;
    }
}

template<>
inline void AssetManager::add_ref(AnimationHandle handle) noexcept {
    auto it = animations_.find(handle.id());
    if (it != animations_.end() && it->second.generation == handle.generation()) {
        ++it->second.ref_count;
    }
}

template<>
inline void AssetManager::add_ref(DataHandle handle) noexcept {
    auto it = data_assets_.find(handle.id());
    if (it != data_assets_.end() && it->second.generation == handle.generation()) {
        ++it->second.ref_count;
    }
}

template<>
inline void AssetManager::add_ref(TilesetHandle handle) noexcept {
    auto it = tilesets_.find(handle.id());
    if (it != tilesets_.end() && it->second.generation == handle.generation()) {
        ++it->second.ref_count;
    }
}

template<>
inline void AssetManager::add_ref(PipelineHandle handle) noexcept {
    auto it = pipelines_.find(handle.id());
    if (it != pipelines_.end() && it->second.generation == handle.generation()) {
        ++it->second.ref_count;
    }
}

template<>
inline void AssetManager::release_ref(TextureHandle handle) noexcept {
    auto it = textures_.find(handle.id());
    if (it != textures_.end() && it->second.generation == handle.generation() && it->second.ref_count > 0) {
        --it->second.ref_count;
    }
}

template<>
inline void AssetManager::release_ref(AudioHandle handle) noexcept {
    auto it = audio_clips_.find(handle.id());
    if (it != audio_clips_.end() && it->second.generation == handle.generation() && it->second.ref_count > 0) {
        --it->second.ref_count;
    }
}

template<>
inline void AssetManager::release_ref(FontHandle handle) noexcept {
    auto it = fonts_.find(handle.id());
    if (it != fonts_.end() && it->second.generation == handle.generation() && it->second.ref_count > 0) {
        --it->second.ref_count;
    }
}

template<>
inline void AssetManager::release_ref(AnimationHandle handle) noexcept {
    auto it = animations_.find(handle.id());
    if (it != animations_.end() && it->second.generation == handle.generation() && it->second.ref_count > 0) {
        --it->second.ref_count;
    }
}

template<>
inline void AssetManager::release_ref(DataHandle handle) noexcept {
    auto it = data_assets_.find(handle.id());
    if (it != data_assets_.end() && it->second.generation == handle.generation() && it->second.ref_count > 0) {
        --it->second.ref_count;
    }
}

template<>
inline void AssetManager::release_ref(TilesetHandle handle) noexcept {
    auto it = tilesets_.find(handle.id());
    if (it != tilesets_.end() && it->second.generation == handle.generation() && it->second.ref_count > 0) {
        --it->second.ref_count;
    }
}

template<>
inline void AssetManager::release_ref(PipelineHandle handle) noexcept {
    auto it = pipelines_.find(handle.id());
    if (it != pipelines_.end() && it->second.generation == handle.generation() && it->second.ref_count > 0) {
        --it->second.ref_count;
    }
}

} // namespace assets
