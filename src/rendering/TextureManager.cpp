#include "../../include/rendering/TextureManager.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include <cassert>
#include <cstring>
#include <limits>
#include <vector>

namespace rendering {

namespace {

bool upload_rgba_pixels(
    SDL_GPUDevice* device,
    SDL_GPUTexture* texture,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const std::uint8_t> pixels
) {
    SDL_GPUTransferBufferCreateInfo transfer_info{};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = static_cast<Uint32>(pixels.size());

    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer_buffer) {
        SDL_Log("Failed to create texture transfer buffer: %s", SDL_GetError());
        return false;
    }

    void* mapped_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (!mapped_data) {
        SDL_Log("Failed to map texture transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }
    std::memcpy(mapped_data, pixels.data(), pixels.size());
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (!command_buffer) {
        SDL_Log("Failed to acquire texture upload command buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) {
        SDL_Log("Failed to begin texture upload copy pass: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(command_buffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        return false;
    }

    SDL_GPUTextureTransferInfo source{};
    source.transfer_buffer = transfer_buffer;
    source.pixels_per_row = width;
    source.rows_per_layer = height;

    SDL_GPUTextureRegion destination{};
    destination.texture = texture;
    destination.w = width;
    destination.h = height;
    destination.d = 1;

    SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);
    SDL_EndGPUCopyPass(copy_pass);
    const bool submitted = SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
    if (!submitted) {
        SDL_Log("Failed to submit texture upload: %s", SDL_GetError());
    }
    return submitted;
}

} // namespace

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
        //SDL_Log("DEBUG: Converted surface from format %u to RGBA32", surface->format);
    }

    const auto width = static_cast<std::uint32_t>(converted_surface->w);
    const auto height = static_cast<std::uint32_t>(converted_surface->h);
    constexpr std::size_t bytes_per_pixel = 4;
    const std::size_t row_bytes = static_cast<std::size_t>(width) * bytes_per_pixel;
    const std::size_t data_size = row_bytes * static_cast<std::size_t>(height);
    const auto* source = static_cast<const std::uint8_t*>(converted_surface->pixels);

    std::vector<std::uint8_t> packed_pixels;
    std::span<const std::uint8_t> pixels;
    if (static_cast<std::size_t>(converted_surface->pitch) == row_bytes) {
        pixels = {source, data_size};
    } else {
        packed_pixels.resize(data_size);
        for (std::uint32_t y = 0; y < height; ++y) {
            std::memcpy(
                packed_pixels.data() + static_cast<std::size_t>(y) * row_bytes,
                source + static_cast<std::size_t>(y) * static_cast<std::size_t>(converted_surface->pitch),
                row_bytes
            );
        }
        pixels = packed_pixels;
    }

    const TextureID texture_id = create_from_rgba_pixels(width, height, pixels, usage);
    if (needs_free) {
        SDL_DestroySurface(converted_surface);
    }
    return texture_id;
}

TextureID TextureManager::create_from_rgba_pixels(
    std::uint32_t width,
    std::uint32_t height,
    std::span<const std::uint8_t> pixels,
    SDL_GPUTextureUsageFlags usage
) {
    if (!device_) {
        SDL_Log("No valid device in TextureManager");
        return INVALID_TEXTURE_ID;
    }
    if (width == 0 || height == 0) {
        SDL_Log("Cannot create a zero-sized texture");
        return INVALID_TEXTURE_ID;
    }
    if (width > std::numeric_limits<std::uint16_t>::max()
        || height > std::numeric_limits<std::uint16_t>::max()) {
        SDL_Log("Texture dimensions exceed the Texture wrapper limit: %ux%u", width, height);
        return INVALID_TEXTURE_ID;
    }

    constexpr std::size_t bytes_per_pixel = 4;
    const std::size_t data_size = static_cast<std::size_t>(width)
        * static_cast<std::size_t>(height)
        * bytes_per_pixel;
    if (pixels.size() != data_size || data_size > std::numeric_limits<Uint32>::max()) {
        SDL_Log(
            "Invalid RGBA pixel data for %ux%u texture: expected %zu bytes, received %zu",
            width,
            height,
            data_size,
            pixels.size()
        );
        return INVALID_TEXTURE_ID;
    }

    SDL_Log("DEBUG: Creating texture %ux%u, data_size=%zu bytes", width, height, data_size);

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
        return INVALID_TEXTURE_ID;
    }
    //SDL_Log("DEBUG: Created GPU texture successfully");

    if (!upload_rgba_pixels(device_->handle(), gpu_texture, width, height, pixels)) {
        SDL_ReleaseGPUTexture(device_->handle(), gpu_texture);
        return INVALID_TEXTURE_ID;
    }

    // Create texture wrapper
    auto texture = std::make_unique<Texture>();
    texture->gpu_texture = gpu_texture;
    texture->width = static_cast<std::uint16_t>(width);
    texture->height = static_cast<std::uint16_t>(height);
    texture->id = next_id_++;

    TextureID new_id = texture->id;
    textures_[new_id] = std::move(texture);
    
    //SDL_Log("DEBUG: Texture created successfully with ID %u", new_id);
    return new_id;
}

bool TextureManager::update_rgba_pixels(
    TextureID texture_id,
    std::uint32_t width,
    std::uint32_t height,
    std::span<const std::uint8_t> pixels
) {
    const auto texture_iterator = textures_.find(texture_id);
    if (!device_
        || texture_iterator == textures_.end()
        || !texture_iterator->second
        || !texture_iterator->second->gpu_texture) {
        return false;
    }

    const auto& texture = *texture_iterator->second;
    if (texture.width != width || texture.height != height) {
        return false;
    }

    constexpr std::size_t bytes_per_pixel = 4;
    const std::size_t expected_size =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * bytes_per_pixel;
    if (pixels.size() != expected_size
        || expected_size > std::numeric_limits<Uint32>::max()) {
        SDL_Log(
            "Invalid RGBA pixel update for %ux%u texture: expected %zu bytes, received %zu",
            width,
            height,
            expected_size,
            pixels.size()
        );
        return false;
    }

    return upload_rgba_pixels(
        device_->handle(),
        texture.gpu_texture,
        width,
        height,
        pixels
    );
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
