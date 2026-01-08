#include "../../include/rendering/TextureManager.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include <cassert>

namespace rendering {

TextureManager::TextureManager(GPUDevice* device)
    : device_(device) {
    assert(device_ && "Device cannot be null");
}

TextureManager::~TextureManager() {
    if (!textures_.empty()) {
        for (const auto& [id, texture] : textures_) {
            if (texture && texture->gpu_texture) {
                SDL_ReleaseGPUTexture(device_->handle(), texture->gpu_texture);
            }
        }
    }
    textures_.clear();
    path_to_id_.clear();
    atlas_regions_.clear();
}

TextureManager::TextureManager(TextureManager&& other) noexcept
    : device_(other.device_)
    , textures_(std::move(other.textures_))
    , path_to_id_(std::move(other.path_to_id_))
    , atlas_regions_(std::move(other.atlas_regions_))
    , next_id_(other.next_id_) {
    other.device_ = nullptr;
    other.textures_.clear();
    other.path_to_id_.clear();
    other.atlas_regions_.clear();
    other.next_id_ = 1;
}

TextureManager& TextureManager::operator=(TextureManager&& other) noexcept {
    if (this != &other) {
        clear();
        device_ = other.device_;
        textures_ = std::move(other.textures_);
        path_to_id_ = std::move(other.path_to_id_);
        atlas_regions_ = std::move(other.atlas_regions_);
        next_id_ = other.next_id_;
        
        other.device_ = nullptr;
        other.textures_.clear();
        other.path_to_id_.clear();
        other.atlas_regions_.clear();
        other.next_id_ = 1;
    }
    return *this;
}

TextureID TextureManager::load(const char* path) {
    auto it = path_to_id_.find(path);
    if (it != path_to_id_.end()) {
        return it->second;
    }

    SDL_Surface* surface = SDL_LoadBMP(path);
    if (!surface) {
        surface = SDL_LoadSurface(path);
        if (!surface) {
            return INVALID_TEXTURE_ID;
        }
    }

    TextureID id = create_from_surface(surface);
    SDL_DestroySurface(surface);

    if (id != INVALID_TEXTURE_ID) {
        path_to_id_[path] = id;
    }

    return id;
}

TextureID TextureManager::create_from_surface(SDL_Surface* surface, SDL_GPUTextureUsageFlags usage) {
    if (!surface) {
        SDL_Log("Invalid surface passed to create_from_surface");
        return INVALID_TEXTURE_ID;
    }
    if (!device_) {
        SDL_Log("No valid device in TextureManager");
        return INVALID_TEXTURE_ID;
    }

    SDL_GPUTextureCreateInfo createInfo = {};
    createInfo.type = SDL_GPU_TEXTURETYPE_2D;
    createInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    createInfo.usage = usage;
    createInfo.width = static_cast<Uint32>(surface->w);
    createInfo.height = static_cast<Uint32>(surface->h);
    createInfo.layer_count_or_depth = 1;
    createInfo.num_levels = 1;
    createInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    
    SDL_GPUTexture* gpu_texture = SDL_CreateGPUTexture(device_->handle(), &createInfo);
    if (!gpu_texture) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return INVALID_TEXTURE_ID;
    }
    
    auto texture = std::make_unique<Texture>();
    texture->gpu_texture = gpu_texture;
    texture->width = static_cast<std::uint16_t>(surface->w);
    texture->height = static_cast<std::uint16_t>(surface->h);
    texture->id = next_id_++;

    TextureID new_id = texture->id;
    textures_[new_id] = std::move(texture);
    return new_id;
}

TextureID TextureManager::create_from_texture(SDL_Texture* texture) {
    if (!texture) {
        SDL_Log("Invalid texture passed to create_from_texture");
        return INVALID_TEXTURE_ID;
    }

    float width_f = 0.0f;
    float height_f = 0.0f;
    if (!SDL_GetTextureSize(texture, &width_f, &height_f)) {
        SDL_Log("Failed to query texture size: %s", SDL_GetError());
        return INVALID_TEXTURE_ID;
    }

    auto tex = std::make_unique<Texture>();
    tex->gpu_texture = reinterpret_cast<SDL_GPUTexture*>(texture);
    tex->width = static_cast<std::uint16_t>(width_f);
    tex->height = static_cast<std::uint16_t>(height_f);
    tex->id = next_id_++;

    TextureID new_id = tex->id;
    textures_[new_id] = std::move(tex);
    return new_id;
}

const Texture* TextureManager::get(TextureID id) const noexcept {
    auto it = textures_.find(id);
    return it != textures_.end() ? it->second.get() : nullptr;
}

void TextureManager::unload(TextureID id) noexcept {
    auto it = textures_.find(id);
    if (it != textures_.end()) {
        if (it->second->gpu_texture) {
            SDL_ReleaseGPUTexture(device_->handle(), it->second->gpu_texture);
        }
        textures_.erase(it);
    }

    for (auto path_it = path_to_id_.begin(); path_it != path_to_id_.end();) {
        if (path_it->second == id) {
            path_it = path_to_id_.erase(path_it);
        } else {
            ++path_it;
        }
    }

    atlas_regions_.erase(id);
}

void TextureManager::destroy(TextureID id) noexcept {
    unload(id);
}

void TextureManager::clear() noexcept {
    for (auto& [id, texture] : textures_) {
        if (texture && texture->gpu_texture) {
            SDL_ReleaseGPUTexture(device_->handle(), texture->gpu_texture);
            texture->gpu_texture = nullptr;
        }
    }
    textures_.clear();
    path_to_id_.clear();
    atlas_regions_.clear();
    next_id_ = 1;
}

void TextureManager::define_region(TextureID texture_id, const std::string& name, const TextureRegion& region) {
    atlas_regions_[texture_id][name] = region;
}

const TextureRegion* TextureManager::get_region(TextureID texture_id, const std::string& name) const noexcept {
    auto texture_it = atlas_regions_.find(texture_id);
    if (texture_it == atlas_regions_.end()) {
        return nullptr;
    }

    auto region_it = texture_it->second.find(name);
    return region_it != texture_it->second.end() ? &region_it->second : nullptr;
}

} // namespace rendering
