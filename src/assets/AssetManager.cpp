#include "../../include/assets/AssetManager.hpp"
#include <SDL3/SDL.h>
#include <fstream>
#include <algorithm>
#include <array>
#include <sstream>

namespace assets {

AssetManager::AssetManager(
    rendering::TextureManager* texture_manager,
    audio::AudioSystem* audio_system,
    rendering::PipelineManager* pipeline_manager
)
    : texture_manager_(texture_manager)
    , audio_system_(audio_system)
    , pipeline_manager_(pipeline_manager)
{
}

AssetManager::~AssetManager() {
    clear();
}

AssetManager::AssetManager(AssetManager&&) noexcept = default;
AssetManager& AssetManager::operator=(AssetManager&&) noexcept = default;

TextureHandle AssetManager::load_texture(const char* path, LoadPriority priority) {
    (void)priority;

    const std::string normalised = normalise_path(path);

    auto it = texture_path_to_handle_.find(normalised);
    if (it != texture_path_to_handle_.end()) {
        auto& entry = textures_[it->second.id()];
        if (entry.generation == it->second.generation() && entry.state == AssetState::Loaded) {
            ++entry.ref_count;
            return it->second;
        }
    }

    const std::uint32_t id = next_texture_id_++;
    auto& entry = textures_[id];
    entry.asset = std::make_unique<TextureAsset>();
    entry.generation = 1;
    entry.ref_count = 1;
    entry.path = normalised;
    entry.state = AssetState::Loading;

    const std::string full_path = resolve_path(normalised.c_str());
    entry.last_modified = get_file_modified_time(full_path);

    if (load_texture_internal(full_path, *entry.asset)) {
        entry.state = AssetState::Loaded;
        entry.asset->source_path = normalised;
    } else {
        entry.state = AssetState::Failed;
        SDL_Log("Failed to load texture: %s", full_path.c_str());
    }

    TextureHandle handle(id, entry.generation);
    texture_path_to_handle_[normalised] = handle;
    return handle;
}

TextureAsset* AssetManager::get_texture(TextureHandle handle) noexcept {
    auto it = textures_.find(handle.id());
    if (it == textures_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

const TextureAsset* AssetManager::get_texture(TextureHandle handle) const noexcept {
    auto it = textures_.find(handle.id());
    if (it == textures_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

void AssetManager::unload_texture(TextureHandle handle) noexcept {
    auto it = textures_.find(handle.id());
    if (it == textures_.end() || it->second.generation != handle.generation()) {
        return;
    }

    auto& entry = it->second;
    if (entry.ref_count > 0) {
        --entry.ref_count;
    }

    if (entry.ref_count == 0 && !entry.pinned) {
        if (entry.asset && entry.asset->texture_id != rendering::INVALID_TEXTURE_ID) {
            texture_manager_->unload(entry.asset->texture_id);
        }
        texture_path_to_handle_.erase(entry.path);
        textures_.erase(it);
    }
}

AudioHandle AssetManager::load_audio(const char* path, LoadPriority priority) {
    (void)priority;

    const std::string normalised = normalise_path(path);

    auto it = audio_path_to_handle_.find(normalised);
    if (it != audio_path_to_handle_.end()) {
        auto& entry = audio_clips_[it->second.id()];
        if (entry.generation == it->second.generation() && entry.state == AssetState::Loaded) {
            ++entry.ref_count;
            return it->second;
        }
    }

    const std::uint32_t id = next_audio_id_++;
    auto& entry = audio_clips_[id];
    entry.asset = std::make_unique<AudioAsset>();
    entry.generation = 1;
    entry.ref_count = 1;
    entry.path = normalised;
    entry.state = AssetState::Loading;

    const std::string full_path = resolve_path(normalised.c_str());
    entry.last_modified = get_file_modified_time(full_path);

    if (load_audio_internal(full_path, *entry.asset)) {
        entry.state = AssetState::Loaded;
        entry.asset->source_path = normalised;
    } else {
        entry.state = AssetState::Failed;
        SDL_Log("Failed to load audio: %s", full_path.c_str());
    }

    AudioHandle handle(id, entry.generation);
    audio_path_to_handle_[normalised] = handle;
    return handle;
}

AudioAsset* AssetManager::get_audio(AudioHandle handle) noexcept {
    auto it = audio_clips_.find(handle.id());
    if (it == audio_clips_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

const AudioAsset* AssetManager::get_audio(AudioHandle handle) const noexcept {
    auto it = audio_clips_.find(handle.id());
    if (it == audio_clips_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

void AssetManager::unload_audio(AudioHandle handle) noexcept {
    auto it = audio_clips_.find(handle.id());
    if (it == audio_clips_.end() || it->second.generation != handle.generation()) {
        return;
    }

    auto& entry = it->second;
    if (entry.ref_count > 0) {
        --entry.ref_count;
    }

    if (entry.ref_count == 0 && !entry.pinned) {
        if (entry.asset && entry.asset->buffer) {
            SDL_free(entry.asset->buffer);
            entry.asset->buffer = nullptr;
        }
        audio_path_to_handle_.erase(entry.path);
        audio_clips_.erase(it);
    }
}

FontHandle AssetManager::load_font(const char* path, float size, LoadPriority priority) {
    (void)priority;

    const std::string normalised = normalise_path(path);
    const std::string cache_key = normalised + "@" + std::to_string(static_cast<int>(size));

    auto it = font_path_to_handle_.find(cache_key);
    if (it != font_path_to_handle_.end()) {
        auto& entry = fonts_[it->second.id()];
        if (entry.generation == it->second.generation() && entry.state == AssetState::Loaded) {
            ++entry.ref_count;
            return it->second;
        }
    }

    const std::uint32_t id = next_font_id_++;
    auto& entry = fonts_[id];
    entry.asset = std::make_unique<FontAsset>();
    entry.generation = 1;
    entry.ref_count = 1;
    entry.path = cache_key;
    entry.state = AssetState::Loading;

    const std::string full_path = resolve_path(normalised.c_str());
    entry.last_modified = get_file_modified_time(full_path);

    if (load_font_internal(full_path, size, *entry.asset)) {
        entry.state = AssetState::Loaded;
        entry.asset->source_path = normalised;
    } else {
        entry.state = AssetState::Failed;
        SDL_Log("Failed to load font: %s", full_path.c_str());
    }

    FontHandle handle(id, entry.generation);
    font_path_to_handle_[cache_key] = handle;
    return handle;
}

FontAsset* AssetManager::get_font(FontHandle handle) noexcept {
    auto it = fonts_.find(handle.id());
    if (it == fonts_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

const FontAsset* AssetManager::get_font(FontHandle handle) const noexcept {
    auto it = fonts_.find(handle.id());
    if (it == fonts_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

void AssetManager::unload_font(FontHandle handle) noexcept {
    auto it = fonts_.find(handle.id());
    if (it == fonts_.end() || it->second.generation != handle.generation()) {
        return;
    }

    auto& entry = it->second;
    if (entry.ref_count > 0) {
        --entry.ref_count;
    }

    if (entry.ref_count == 0 && !entry.pinned) {
        if (entry.asset && entry.asset->atlas_texture != rendering::INVALID_TEXTURE_ID) {
            texture_manager_->unload(entry.asset->atlas_texture);
        }
        font_path_to_handle_.erase(entry.path);
        fonts_.erase(it);
    }
}

AnimationHandle AssetManager::load_animation(const char* path, LoadPriority priority) {
    (void)priority;

    const std::string normalised = normalise_path(path);

    auto it = animation_path_to_handle_.find(normalised);
    if (it != animation_path_to_handle_.end()) {
        auto& entry = animations_[it->second.id()];
        if (entry.generation == it->second.generation() && entry.state == AssetState::Loaded) {
            ++entry.ref_count;
            return it->second;
        }
    }

    const std::uint32_t id = next_animation_id_++;
    auto& entry = animations_[id];
    entry.asset = std::make_unique<AnimationAsset>();
    entry.generation = 1;
    entry.ref_count = 1;
    entry.path = normalised;
    entry.state = AssetState::Loading;

    const std::string full_path = resolve_path(normalised.c_str());
    entry.last_modified = get_file_modified_time(full_path);

    if (load_animation_internal(full_path, *entry.asset)) {
        entry.state = AssetState::Loaded;
        entry.asset->source_path = normalised;
    } else {
        entry.state = AssetState::Failed;
        SDL_Log("Failed to load animation: %s", full_path.c_str());
    }

    AnimationHandle handle(id, entry.generation);
    animation_path_to_handle_[normalised] = handle;
    return handle;
}

AnimationAsset* AssetManager::get_animation(AnimationHandle handle) noexcept {
    auto it = animations_.find(handle.id());
    if (it == animations_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

const AnimationAsset* AssetManager::get_animation(AnimationHandle handle) const noexcept {
    auto it = animations_.find(handle.id());
    if (it == animations_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

void AssetManager::unload_animation(AnimationHandle handle) noexcept {
    auto it = animations_.find(handle.id());
    if (it == animations_.end() || it->second.generation != handle.generation()) {
        return;
    }

    auto& entry = it->second;
    if (entry.ref_count > 0) {
        --entry.ref_count;
    }

    if (entry.ref_count == 0 && !entry.pinned) {
        animation_path_to_handle_.erase(entry.path);
        animations_.erase(it);
    }
}

DataHandle AssetManager::load_data(const char* path, LoadPriority priority) {
    (void)priority;

    const std::string normalised = normalise_path(path);

    auto it = data_path_to_handle_.find(normalised);
    if (it != data_path_to_handle_.end()) {
        auto& entry = data_assets_[it->second.id()];
        if (entry.generation == it->second.generation() && entry.state == AssetState::Loaded) {
            ++entry.ref_count;
            return it->second;
        }
    }

    const std::uint32_t id = next_data_id_++;
    auto& entry = data_assets_[id];
    entry.asset = std::make_unique<DataAsset>();
    entry.generation = 1;
    entry.ref_count = 1;
    entry.path = normalised;
    entry.state = AssetState::Loading;

    const std::string full_path = resolve_path(normalised.c_str());
    entry.last_modified = get_file_modified_time(full_path);

    if (load_data_internal(full_path, *entry.asset)) {
        entry.state = AssetState::Loaded;
        entry.asset->source_path = normalised;
    } else {
        entry.state = AssetState::Failed;
        SDL_Log("Failed to load data: %s", full_path.c_str());
    }

    DataHandle handle(id, entry.generation);
    data_path_to_handle_[normalised] = handle;
    return handle;
}

DataAsset* AssetManager::get_data(DataHandle handle) noexcept {
    auto it = data_assets_.find(handle.id());
    if (it == data_assets_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

const DataAsset* AssetManager::get_data(DataHandle handle) const noexcept {
    auto it = data_assets_.find(handle.id());
    if (it == data_assets_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

void AssetManager::unload_data(DataHandle handle) noexcept {
    auto it = data_assets_.find(handle.id());
    if (it == data_assets_.end() || it->second.generation != handle.generation()) {
        return;
    }

    auto& entry = it->second;
    if (entry.ref_count > 0) {
        --entry.ref_count;
    }

    if (entry.ref_count == 0 && !entry.pinned) {
        data_path_to_handle_.erase(entry.path);
        data_assets_.erase(it);
    }
}

TilesetHandle AssetManager::load_tileset(const char* path, LoadPriority priority) {
    (void)priority;

    const std::string normalised = normalise_path(path);

    auto it = tileset_path_to_handle_.find(normalised);
    if (it != tileset_path_to_handle_.end()) {
        auto& entry = tilesets_[it->second.id()];
        if (entry.generation == it->second.generation() && entry.state == AssetState::Loaded) {
            ++entry.ref_count;
            return it->second;
        }
    }

    const std::uint32_t id = next_tileset_id_++;
    auto& entry = tilesets_[id];
    entry.asset = std::make_unique<TilesetAsset>();
    entry.generation = 1;
    entry.ref_count = 1;
    entry.path = normalised;
    entry.state = AssetState::Loading;

    const std::string full_path = resolve_path(normalised.c_str());
    entry.last_modified = get_file_modified_time(full_path);

    if (load_tileset_internal(full_path, *entry.asset)) {
        entry.state = AssetState::Loaded;
        entry.asset->source_path = normalised;
    } else {
        entry.state = AssetState::Failed;
        SDL_Log("Failed to load tileset: %s", full_path.c_str());
    }

    TilesetHandle handle(id, entry.generation);
    tileset_path_to_handle_[normalised] = handle;
    return handle;
}

TilesetAsset* AssetManager::get_tileset(TilesetHandle handle) noexcept {
    auto it = tilesets_.find(handle.id());
    if (it == tilesets_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

const TilesetAsset* AssetManager::get_tileset(TilesetHandle handle) const noexcept {
    auto it = tilesets_.find(handle.id());
    if (it == tilesets_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

void AssetManager::unload_tileset(TilesetHandle handle) noexcept {
    auto it = tilesets_.find(handle.id());
    if (it == tilesets_.end() || it->second.generation != handle.generation()) {
        return;
    }

    auto& entry = it->second;
    if (entry.ref_count > 0) {
        --entry.ref_count;
    }

    if (entry.ref_count == 0 && !entry.pinned) {
        if (entry.asset && entry.asset->texture_id != rendering::INVALID_TEXTURE_ID) {
            texture_manager_->unload(entry.asset->texture_id);
        }
        tileset_path_to_handle_.erase(entry.path);
        tilesets_.erase(it);
    }
}

PipelineHandle AssetManager::load_pipeline(const char* path, LoadPriority priority) {
    (void)priority;

    if (!pipeline_manager_) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "PipelineManager not available");
        return PipelineHandle{};
    }

    const std::string normalised = normalise_path(path);

    auto it = pipeline_path_to_handle_.find(normalised);
    if (it != pipeline_path_to_handle_.end()) {
        auto& entry = pipelines_[it->second.id()];
        if (entry.generation == it->second.generation() && entry.state == AssetState::Loaded) {
            ++entry.ref_count;
            return it->second;
        }
    }

    const std::uint32_t id = next_pipeline_id_++;
    auto& entry = pipelines_[id];
    entry.asset = std::make_unique<PipelineAsset>();
    entry.generation = 1;
    entry.ref_count = 1;
    entry.path = normalised;
    entry.state = AssetState::Loading;

    const std::string full_path = resolve_path(normalised.c_str());
    entry.last_modified = get_file_modified_time(full_path);

    if (load_pipeline_internal(full_path, *entry.asset)) {
        entry.state = AssetState::Loaded;
        entry.asset->source_path = normalised;
    } else {
        entry.state = AssetState::Failed;
        SDL_Log("Failed to load pipeline: %s", full_path.c_str());
    }

    PipelineHandle handle(id, entry.generation);
    pipeline_path_to_handle_[normalised] = handle;
    return handle;
}

PipelineHandle AssetManager::load_pipeline_from_config(
    const char* name,
    rendering::PipelineType base_type,
    const rendering::PipelineConfig& config
) {
    if (!pipeline_manager_) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "PipelineManager not available");
        return PipelineHandle{};
    }

    const std::string cache_key = std::string("config:") + name;

    auto it = pipeline_path_to_handle_.find(cache_key);
    if (it != pipeline_path_to_handle_.end()) {
        auto& entry = pipelines_[it->second.id()];
        if (entry.generation == it->second.generation() && entry.state == AssetState::Loaded) {
            ++entry.ref_count;
            return it->second;
        }
    }

    const std::uint32_t id = next_pipeline_id_++;
    auto& entry = pipelines_[id];
    entry.asset = std::make_unique<PipelineAsset>();
    entry.generation = 1;
    entry.ref_count = 1;
    entry.path = cache_key;
    entry.state = AssetState::Loading;
    entry.last_modified = 0;

    entry.asset->base_type = base_type;
    entry.asset->config = config;
    entry.asset->pipeline_handle = pipeline_manager_->get_or_create_pipeline(base_type, config);

    if (entry.asset->pipeline_handle != rendering::INVALID_PIPELINE_HANDLE) {
        entry.state = AssetState::Loaded;
        entry.asset->source_path = cache_key;
    } else {
        entry.state = AssetState::Failed;
        SDL_Log("Failed to create pipeline from config: %s", name);
    }

    PipelineHandle handle(id, entry.generation);
    pipeline_path_to_handle_[cache_key] = handle;
    return handle;
}

PipelineAsset* AssetManager::get_pipeline(PipelineHandle handle) noexcept {
    auto it = pipelines_.find(handle.id());
    if (it == pipelines_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

const PipelineAsset* AssetManager::get_pipeline(PipelineHandle handle) const noexcept {
    auto it = pipelines_.find(handle.id());
    if (it == pipelines_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }
    return it->second.asset.get();
}

void AssetManager::unload_pipeline(PipelineHandle handle) noexcept {
    auto it = pipelines_.find(handle.id());
    if (it == pipelines_.end() || it->second.generation != handle.generation()) {
        return;
    }

    auto& entry = it->second;
    if (entry.ref_count > 0) {
        --entry.ref_count;
    }

    if (entry.ref_count == 0 && !entry.pinned) {
        pipeline_path_to_handle_.erase(entry.path);
        pipelines_.erase(it);
    }
}

SDL_GPUGraphicsPipeline* AssetManager::get_gpu_pipeline(PipelineHandle handle) const noexcept {
    if (!pipeline_manager_) {
        return nullptr;
    }

    auto it = pipelines_.find(handle.id());
    if (it == pipelines_.end() || it->second.generation != handle.generation()) {
        return nullptr;
    }

    const auto& asset = it->second.asset;
    if (!asset || asset->pipeline_handle == rendering::INVALID_PIPELINE_HANDLE) {
        return nullptr;
    }

    return pipeline_manager_->get_pipeline(asset->pipeline_handle);
}

SDL_GPUGraphicsPipeline* AssetManager::get_core_pipeline(rendering::PipelineType type) const noexcept {
    if (!pipeline_manager_) {
        return nullptr;
    }
    return pipeline_manager_->get_core_pipeline(type);
}

std::optional<std::string> AssetManager::load_text_file(const char* path) const {
    const std::string full_path = resolve_path(path);

    std::ifstream file(full_path, std::ios::in);
    if (!file.is_open()) {
        SDL_Log("Failed to open text file: %s", full_path.c_str());
        return std::nullopt;
    }

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return content;
}

std::optional<std::vector<std::uint8_t>> AssetManager::load_binary_file(const char* path) const {
    const std::string full_path = resolve_path(path);

    std::ifstream file(full_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        SDL_Log("Failed to open binary file: %s", full_path.c_str());
        return std::nullopt;
    }

    const auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        SDL_Log("Failed to read binary file: %s", full_path.c_str());
        return std::nullopt;
    }

    return buffer;
}

void AssetManager::preload_manifest(const char* manifest_path) {
    auto text = load_text_file(manifest_path);
    if (!text) {
        SDL_Log("Failed to load manifest: %s", manifest_path);
        return;
    }

    std::istringstream stream(*text);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const auto colon_pos = line.find(':');
        if (colon_pos == std::string::npos) {
            continue;
        }

        const std::string type = line.substr(0, colon_pos);
        const std::string path = line.substr(colon_pos + 1);

        bool success = true;
        if (type == "texture") {
            auto handle = load_texture(path.c_str(), LoadPriority::Normal);
            success = get_state(handle) == AssetState::Loaded;
        } else if (type == "audio") {
            auto handle = load_audio(path.c_str(), LoadPriority::Normal);
            success = get_state(handle) == AssetState::Loaded;
        } else if (type == "font") {
            auto handle = load_font(path.c_str(), 16.0f, LoadPriority::Normal);
            success = get_state(handle) == AssetState::Loaded;
        } else if (type == "animation") {
            auto handle = load_animation(path.c_str(), LoadPriority::Normal);
            success = get_state(handle) == AssetState::Loaded;
        } else if (type == "data") {
            auto handle = load_data(path.c_str(), LoadPriority::Normal);
            success = get_state(handle) == AssetState::Loaded;
        } else if (type == "tileset") {
            auto handle = load_tileset(path.c_str(), LoadPriority::Normal);
            success = get_state(handle) == AssetState::Loaded;
        } else if (type == "pipeline") {
            auto handle = load_pipeline(path.c_str(), LoadPriority::Normal);
            success = get_state(handle) == AssetState::Loaded;
        }

        if (!success) {
            SDL_Log("Failed to load asset: %s", path.c_str());
        }
    }
}

void AssetManager::load_batch(const std::vector<std::string>& paths, LoadCallback on_complete) {
    bool all_success = true;

    for (const auto& path : paths) {
        const auto dot_pos = path.rfind('.');
        if (dot_pos == std::string::npos) {
            all_success = false;
            continue;
        }

        std::string ext = path.substr(dot_pos);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".bmp") {
            auto handle = load_texture(path.c_str());
            if (get_state(handle) == AssetState::Failed) {
                all_success = false;
            }
        } else if (ext == ".wav" || ext == ".ogg" || ext == ".mp3") {
            auto handle = load_audio(path.c_str());
            if (get_state(handle) == AssetState::Failed) {
                all_success = false;
            }
        } else if (ext == ".ttf" || ext == ".otf") {
            auto handle = load_font(path.c_str());
            if (get_state(handle) == AssetState::Failed) {
                all_success = false;
            }
        } else if (ext == ".json") {
            auto handle = load_data(path.c_str());
            if (get_state(handle) == AssetState::Failed) {
                all_success = false;
            }
        } else if (ext == ".pipeline") {
            auto handle = load_pipeline(path.c_str());
            if (get_state(handle) == AssetState::Failed) {
                all_success = false;
            }
        }
    }

    if (on_complete) {
        on_complete(all_success);
    }
}

void AssetManager::set_hot_reload_enabled(bool enable) noexcept {
    hot_reload_enabled_ = enable;
}

void AssetManager::poll_hot_reload() {
    if (!hot_reload_enabled_) {
        return;
    }

    for (auto& [id, entry] : textures_) {
        if (entry.state != AssetState::Loaded) {
            continue;
        }

        const std::string full_path = resolve_path(entry.path.c_str());
        if (has_file_changed(full_path, entry.last_modified)) {
            entry.last_modified = get_file_modified_time(full_path);

            if (entry.asset->texture_id != rendering::INVALID_TEXTURE_ID) {
                texture_manager_->unload(entry.asset->texture_id);
            }

            bool success = load_texture_internal(full_path, *entry.asset);
            if (success) {
                for (const auto& callback : reload_callbacks_) {
                    callback(entry.path);
                }
            }
        }
    }

    for (auto& [id, entry] : data_assets_) {
        if (entry.state != AssetState::Loaded) {
            continue;
        }

        const std::string full_path = resolve_path(entry.path.c_str());
        if (has_file_changed(full_path, entry.last_modified)) {
            entry.last_modified = get_file_modified_time(full_path);

            bool success = load_data_internal(full_path, *entry.asset);
            if (success) {
                for (const auto& callback : reload_callbacks_) {
                    callback(entry.path);
                }
            }
        }
    }

    for (auto& [id, entry] : pipelines_) {
        if (entry.state != AssetState::Loaded || entry.last_modified == 0) {
            continue;
        }

        const std::string full_path = resolve_path(entry.path.c_str());
        if (has_file_changed(full_path, entry.last_modified)) {
            entry.last_modified = get_file_modified_time(full_path);

            bool success = load_pipeline_internal(full_path, *entry.asset);
            if (success) {
                for (const auto& callback : reload_callbacks_) {
                    callback(entry.path);
                }
            }
        }
    }
}

void AssetManager::register_reload_callback(ReloadCallback callback) {
    reload_callbacks_.push_back(std::move(callback));
}

template<>
void AssetManager::force_reload(TextureHandle handle) {
    auto it = textures_.find(handle.id());
    if (it == textures_.end() || it->second.generation != handle.generation()) {
        return;
    }

    auto& entry = it->second;
    const std::string full_path = resolve_path(entry.path.c_str());

    if (entry.asset->texture_id != rendering::INVALID_TEXTURE_ID) {
        texture_manager_->unload(entry.asset->texture_id);
    }

    (void)load_texture_internal(full_path, *entry.asset);
    entry.last_modified = get_file_modified_time(full_path);
}

template<>
void AssetManager::force_reload(DataHandle handle) {
    auto it = data_assets_.find(handle.id());
    if (it == data_assets_.end() || it->second.generation != handle.generation()) {
        return;
    }

    auto& entry = it->second;
    const std::string full_path = resolve_path(entry.path.c_str());

    (void)load_data_internal(full_path, *entry.asset);
    entry.last_modified = get_file_modified_time(full_path);
}

template<>
void AssetManager::force_reload(PipelineHandle handle) {
    auto it = pipelines_.find(handle.id());
    if (it == pipelines_.end() || it->second.generation != handle.generation()) {
        return;
    }

    auto& entry = it->second;
    
    if (entry.last_modified == 0) {
        return;
    }

    const std::string full_path = resolve_path(entry.path.c_str());
    (void)load_pipeline_internal(full_path, *entry.asset);
    entry.last_modified = get_file_modified_time(full_path);
}

void AssetManager::collect_unused(float grace_period_seconds) {
    (void)grace_period_seconds;

    for (auto it = textures_.begin(); it != textures_.end();) {
        if (it->second.ref_count == 0 && !it->second.pinned) {
            if (it->second.asset && it->second.asset->texture_id != rendering::INVALID_TEXTURE_ID) {
                texture_manager_->unload(it->second.asset->texture_id);
            }
            texture_path_to_handle_.erase(it->second.path);
            it = textures_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = audio_clips_.begin(); it != audio_clips_.end();) {
        if (it->second.ref_count == 0 && !it->second.pinned) {
            if (it->second.asset && it->second.asset->buffer) {
                SDL_free(it->second.asset->buffer);
            }
            audio_path_to_handle_.erase(it->second.path);
            it = audio_clips_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = fonts_.begin(); it != fonts_.end();) {
        if (it->second.ref_count == 0 && !it->second.pinned) {
            if (it->second.asset && it->second.asset->atlas_texture != rendering::INVALID_TEXTURE_ID) {
                texture_manager_->unload(it->second.asset->atlas_texture);
            }
            font_path_to_handle_.erase(it->second.path);
            it = fonts_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = animations_.begin(); it != animations_.end();) {
        if (it->second.ref_count == 0 && !it->second.pinned) {
            animation_path_to_handle_.erase(it->second.path);
            it = animations_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = data_assets_.begin(); it != data_assets_.end();) {
        if (it->second.ref_count == 0 && !it->second.pinned) {
            data_path_to_handle_.erase(it->second.path);
            it = data_assets_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = tilesets_.begin(); it != tilesets_.end();) {
        if (it->second.ref_count == 0 && !it->second.pinned) {
            if (it->second.asset && it->second.asset->texture_id != rendering::INVALID_TEXTURE_ID) {
                texture_manager_->unload(it->second.asset->texture_id);
            }
            tileset_path_to_handle_.erase(it->second.path);
            it = tilesets_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = pipelines_.begin(); it != pipelines_.end();) {
        if (it->second.ref_count == 0 && !it->second.pinned) {
            pipeline_path_to_handle_.erase(it->second.path);
            it = pipelines_.erase(it);
        } else {
            ++it;
        }
    }
}

void AssetManager::clear() noexcept {
    for (auto& [id, entry] : textures_) {
        if (entry.asset && entry.asset->texture_id != rendering::INVALID_TEXTURE_ID) {
            texture_manager_->unload(entry.asset->texture_id);
        }
    }
    textures_.clear();
    texture_path_to_handle_.clear();

    for (auto& [id, entry] : audio_clips_) {
        if (entry.asset && entry.asset->buffer) {
            SDL_free(entry.asset->buffer);
        }
    }
    audio_clips_.clear();
    audio_path_to_handle_.clear();

    for (auto& [id, entry] : fonts_) {
        if (entry.asset && entry.asset->atlas_texture != rendering::INVALID_TEXTURE_ID) {
            texture_manager_->unload(entry.asset->atlas_texture);
        }
    }
    fonts_.clear();
    font_path_to_handle_.clear();

    animations_.clear();
    animation_path_to_handle_.clear();

    data_assets_.clear();
    data_path_to_handle_.clear();

    for (auto& [id, entry] : tilesets_) {
        if (entry.asset && entry.asset->texture_id != rendering::INVALID_TEXTURE_ID) {
            texture_manager_->unload(entry.asset->texture_id);
        }
    }
    tilesets_.clear();
    tileset_path_to_handle_.clear();

    pipelines_.clear();
    pipeline_path_to_handle_.clear();

    reload_callbacks_.clear();
}

void AssetManager::set_base_path(const char* path) {
    base_path_ = path;
    if (!base_path_.empty() && base_path_.back() != '/' && base_path_.back() != '\\') {
        base_path_ += '/';
    }
}

std::string AssetManager::resolve_path(const char* relative_path) const {
    if (std::filesystem::path(relative_path).is_absolute()) {
        return relative_path;
    }
    return base_path_ + relative_path;
}

bool AssetManager::load_texture_internal(const std::string& path, TextureAsset& asset) {
    if (!texture_manager_) {
        return false;
    }

    SDL_Surface* surface = SDL_LoadBMP(path.c_str());
    
    if (!surface) {
        surface = SDL_LoadSurface(path.c_str());
    }
    
    if (!surface) {
        SDL_Log("Failed to load image %s: %s", path.c_str(), SDL_GetError());
        return false;
    }

    asset.texture_id = texture_manager_->create_from_surface(surface);
    SDL_DestroySurface(surface);

    if (asset.texture_id == rendering::INVALID_TEXTURE_ID) {
        return false;
    }

    const rendering::Texture* tex = texture_manager_->get(asset.texture_id);
    if (tex) {
        asset.width = tex->width;
        asset.height = tex->height;
    }

    return true;
}

bool AssetManager::load_audio_internal(const std::string& path, AudioAsset& asset) {
    SDL_AudioSpec spec;
    std::uint8_t* buffer = nullptr;
    std::uint32_t length = 0;

    if (!SDL_LoadWAV(path.c_str(), &spec, &buffer, &length)) {
        SDL_Log("SDL_LoadWAV failed: %s", SDL_GetError());
        return false;
    }

    asset.buffer = buffer;
    asset.length = length;
    asset.sample_rate = static_cast<std::uint32_t>(spec.freq);
    asset.channels = static_cast<std::uint8_t>(spec.channels);

    const int bytes_per_sample = SDL_AUDIO_BYTESIZE(spec.format);
    const int samples = static_cast<int>(length) / (bytes_per_sample * spec.channels);
    asset.duration = static_cast<float>(samples) / static_cast<float>(spec.freq);

    return true;
}

bool AssetManager::load_font_internal(const std::string& path, float size, FontAsset& asset) {
    (void)path;
    (void)size;

    asset.base_size = size;
    asset.line_height = size * 1.2f;

    SDL_Log("Font loading not yet implemented: %s", path.c_str());
    return false;
}

bool AssetManager::load_animation_internal(const std::string& path, AnimationAsset& asset) {
    auto data = load_binary_file(path.c_str());
    if (!data) {
        return false;
    }

    (void)asset;

    SDL_Log("Animation loading not yet implemented: %s", path.c_str());
    return false;
}

bool AssetManager::load_data_internal(const std::string& path, DataAsset& asset) {
    auto data = load_binary_file(path.c_str());
    if (!data) {
        return false;
    }

    asset.raw_data = std::move(*data);
    return true;
}

bool AssetManager::load_tileset_internal(const std::string& path, TilesetAsset& asset) {
    auto data = load_text_file(path.c_str());
    if (!data) {
        return false;
    }

    (void)asset;

    SDL_Log("Tileset JSON loading not yet implemented: %s", path.c_str());
    return false;
}

bool AssetManager::load_pipeline_internal(const std::string& path, PipelineAsset& asset) {
    if (!pipeline_manager_) {
        return false;
    }

    auto data = load_text_file(path.c_str());
    if (!data) {
        return false;
    }

    asset.base_type = rendering::PipelineType::Sprite;
    asset.config = rendering::PipelineManager::get_default_config(rendering::PipelineType::Sprite);
    asset.pipeline_handle = pipeline_manager_->get_or_create_pipeline(asset.base_type, asset.config);

    if (asset.pipeline_handle == rendering::INVALID_PIPELINE_HANDLE) {
        SDL_Log("Failed to create pipeline from definition: %s", path.c_str());
        return false;
    }

    SDL_Log("Pipeline loaded (using defaults, JSON parsing not yet implemented): %s", path.c_str());
    return true;
}

std::string AssetManager::normalise_path(const char* path) const {
    std::string result(path);

    std::replace(result.begin(), result.end(), '\\', '/');

    if (result.size() >= 2 && result[0] == '.' && result[1] == '/') {
        result = result.substr(2);
    }

    std::transform(result.begin(), result.end(), result.begin(), ::tolower);

    return result;
}

std::uint64_t AssetManager::get_file_modified_time(const std::string& path) const {
    std::error_code ec;
    const auto time = std::filesystem::last_write_time(path, ec);
    if (ec) {
        return 0;
    }

    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            time.time_since_epoch()
        ).count()
    );
}

bool AssetManager::has_file_changed(const std::string& path, std::uint64_t last_modified) const {
    const std::uint64_t current = get_file_modified_time(path);
    return current != 0 && current != last_modified;
}

} // namespace assets
