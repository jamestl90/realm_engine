#include "../../include/rendering/TextureManager.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include <cassert>
#include <cstring>

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

    // Convert surface to RGBA32 format if needed
    SDL_Surface* converted_surface = surface;
    bool needs_free = false;
    
    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        converted_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        if (!converted_surface) {
            SDL_Log("Failed to convert surface to RGBA32: %s", SDL_GetError());
            return INVALID_TEXTURE_ID;
        }
        needs_free = true;
        SDL_Log("DEBUG: Converted surface from format %u to RGBA32", surface->format);
    }

    const Uint32 width = static_cast<Uint32>(converted_surface->w);
    const Uint32 height = static_cast<Uint32>(converted_surface->h);
    const Uint32 bytes_per_pixel = 4; // RGBA32
    const Uint32 data_size = width * height * bytes_per_pixel;

    SDL_Log("DEBUG: Creating texture %ux%u, data_size=%u bytes", width, height, data_size);

    // Create GPU texture with SAMPLER usage for blitting
    SDL_GPUTextureCreateInfo createInfo = {};
    createInfo.type = SDL_GPU_TEXTURETYPE_2D;
    createInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    createInfo.usage = usage | SDL_GPU_TEXTUREUSAGE_SAMPLER; // Ensure SAMPLER is always set
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layer_count_or_depth = 1;
    createInfo.num_levels = 1;
    createInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    
    SDL_GPUTexture* gpu_texture = SDL_CreateGPUTexture(device_->handle(), &createInfo);
    if (!gpu_texture) {
        SDL_Log("Failed to create GPU texture: %s", SDL_GetError());
        if (needs_free) {
            SDL_DestroySurface(converted_surface);
        }
        return INVALID_TEXTURE_ID;
    }
    SDL_Log("DEBUG: Created GPU texture successfully");

    // Create transfer buffer to upload pixel data
    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = data_size;

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device_->handle(), &transfer_info);
    if (!transfer_buffer) {
        SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTexture(device_->handle(), gpu_texture);
        if (needs_free) {
            SDL_DestroySurface(converted_surface);
        }
        return INVALID_TEXTURE_ID;
    }
    SDL_Log("DEBUG: Created transfer buffer successfully");

    // Map transfer buffer and copy pixel data
    void* mapped_data = SDL_MapGPUTransferBuffer(device_->handle(), transfer_buffer, false);
    if (!mapped_data) {
        SDL_Log("Failed to map transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device_->handle(), transfer_buffer);
        SDL_ReleaseGPUTexture(device_->handle(), gpu_texture);
        if (needs_free) {
            SDL_DestroySurface(converted_surface);
        }
        return INVALID_TEXTURE_ID;
    }

    // Copy pixel data row by row (handles pitch differences)
    const Uint8* src_pixels = static_cast<const Uint8*>(converted_surface->pixels);
    Uint8* dst_pixels = static_cast<Uint8*>(mapped_data);
    const Uint32 src_pitch = static_cast<Uint32>(converted_surface->pitch);
    const Uint32 dst_pitch = width * bytes_per_pixel;

    for (Uint32 y = 0; y < height; ++y) {
        std::memcpy(dst_pixels + y * dst_pitch, src_pixels + y * src_pitch, dst_pitch);
    }

    SDL_UnmapGPUTransferBuffer(device_->handle(), transfer_buffer);
    SDL_Log("DEBUG: Copied %u bytes of pixel data to transfer buffer", data_size);

    // Acquire command buffer for upload
    SDL_GPUCommandBuffer* cmd_buffer = SDL_AcquireGPUCommandBuffer(device_->handle());
    if (!cmd_buffer) {
        SDL_Log("Failed to acquire command buffer for texture upload: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device_->handle(), transfer_buffer);
        SDL_ReleaseGPUTexture(device_->handle(), gpu_texture);
        if (needs_free) {
            SDL_DestroySurface(converted_surface);
        }
        return INVALID_TEXTURE_ID;
    }

    // Begin copy pass
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buffer);
    if (!copy_pass) {
        SDL_Log("Failed to begin copy pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(cmd_buffer);
        SDL_ReleaseGPUTransferBuffer(device_->handle(), transfer_buffer);
        SDL_ReleaseGPUTexture(device_->handle(), gpu_texture);
        if (needs_free) {
            SDL_DestroySurface(converted_surface);
        }
        return INVALID_TEXTURE_ID;
    }

    // Set up transfer info
    SDL_GPUTextureTransferInfo src_transfer = {};
    src_transfer.transfer_buffer = transfer_buffer;
    src_transfer.offset = 0;
    src_transfer.pixels_per_row = width;
    src_transfer.rows_per_layer = height;

    // Set up destination region
    SDL_GPUTextureRegion dst_region = {};
    dst_region.texture = gpu_texture;
    dst_region.mip_level = 0;
    dst_region.layer = 0;
    dst_region.x = 0;
    dst_region.y = 0;
    dst_region.z = 0;
    dst_region.w = width;
    dst_region.h = height;
    dst_region.d = 1;

    // Upload texture data
    SDL_UploadToGPUTexture(copy_pass, &src_transfer, &dst_region, false);
    SDL_Log("DEBUG: Uploaded texture data via copy pass");

    // End copy pass and submit
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd_buffer);
    SDL_Log("DEBUG: Submitted texture upload command buffer");

    // Wait for upload to complete before releasing transfer buffer
    SDL_WaitForGPUIdle(device_->handle());

    // Release transfer buffer (no longer needed)
    SDL_ReleaseGPUTransferBuffer(device_->handle(), transfer_buffer);

    // Clean up converted surface if we created one
    if (needs_free) {
        SDL_DestroySurface(converted_surface);
    }

    // Create texture wrapper
    auto texture = std::make_unique<Texture>();
    texture->gpu_texture = gpu_texture;
    texture->width = static_cast<std::uint16_t>(width);
    texture->height = static_cast<std::uint16_t>(height);
    texture->id = next_id_++;

    TextureID new_id = texture->id;
    textures_[new_id] = std::move(texture);
    
    SDL_Log("DEBUG: Texture created successfully with ID %u", new_id);
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

TextureID TextureManager::default_white_texture() 
{ 
    if (default_white_texture_ == INVALID_TEXTURE_ID) {
        create_default_1x1_white();
    }
    return default_white_texture_; 
}

void TextureManager::create_default_1x1_white()
{
    SDL_Surface* surface = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        SDL_Log("Failed to create 1x1 surface: %s", SDL_GetError());
        return;
    }

    SDL_Color white = { 255, 255, 255, 255 };
    Uint32 pixel = SDL_MapSurfaceRGBA(surface, white.r, white.g, white.b, white.a);

    if (!SDL_FillSurfaceRect(surface, nullptr, pixel)) {
        SDL_Log("Failed to fill 1x1 surface: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return;
    }

    default_white_texture_ = create_from_surface(surface);
    if (default_white_texture_ == rendering::INVALID_TEXTURE_ID) {
        SDL_Log("Failed to create default white texture: %s", SDL_GetError());
        SDL_DestroySurface(surface);
        return;
    }

    SDL_DestroySurface(surface);
}

} // namespace rendering
