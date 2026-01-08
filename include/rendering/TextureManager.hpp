#pragma once

#include "Texture.hpp"
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace rendering {

// Forward declarations
class GPUDevice;

// Texture manager - handles loading and caching
class TextureManager {
public:
    explicit TextureManager(GPUDevice* device);

    // Non-copyable, movable
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) noexcept;
    TextureManager& operator=(TextureManager&&) noexcept;
    ~TextureManager();

    // Load texture from file
    [[nodiscard]] TextureID load(const char* path);

    // Create texture from SDL texture
    [[nodiscard]] TextureID create_from_texture(SDL_Texture* texture);

    // Get texture by ID
    [[nodiscard]] const Texture* get(TextureID id) const noexcept;

    // Unload texture
    void unload(TextureID id) noexcept;

    // Destroy texture
    void destroy(TextureID id) noexcept;

    // Clear all textures
    void clear() noexcept;

    // Create texture from SDL surface
    [[nodiscard]] TextureID create_from_surface(SDL_Surface* surface, SDL_GPUTextureUsageFlags usage = SDL_GPU_TEXTUREUSAGE_SAMPLER);

    // Define atlas region
    void define_region(TextureID texture_id, const std::string& name, const TextureRegion& region);

    // Get atlas region
    [[nodiscard]] const TextureRegion* get_region(TextureID texture_id, const std::string& name) const noexcept;

private:
    GPUDevice* device_{nullptr};
    std::unordered_map<TextureID, std::unique_ptr<Texture>> textures_{};
    std::unordered_map<std::string, TextureID> path_to_id_{};
    std::unordered_map<TextureID, std::unordered_map<std::string, TextureRegion>> atlas_regions_{};
    TextureID next_id_{1};
};

} // namespace rendering
