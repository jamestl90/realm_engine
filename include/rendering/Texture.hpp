#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

namespace rendering {

// Texture handle
using TextureID = std::uint32_t;
constexpr TextureID INVALID_TEXTURE_ID = 0;

// Texture region for sprite atlases
struct TextureRegion {
    float u0{0.0f}, v0{0.0f}; // Top-left UV
    float u1{1.0f}, v1{1.0f}; // Bottom-right UV
    std::uint16_t width{0};
    std::uint16_t height{0};
};

// Texture resource
struct Texture {
    SDL_Texture* gpu_texture{nullptr};
    std::uint16_t width{0};
    std::uint16_t height{0};
    TextureID id{INVALID_TEXTURE_ID};
};

// Texture manager - handles loading and caching
class TextureManager {
public:
    explicit TextureManager(SDL_Renderer* renderer);
    ~TextureManager();

    // Non-copyable, movable
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) noexcept;
    TextureManager& operator=(TextureManager&&) noexcept;

    // Load texture from file
    [[nodiscard]] TextureID load(const char* path);

    // Create texture from surface
    [[nodiscard]] TextureID create_from_surface(SDL_Surface* surface);

    // Get texture by ID
    [[nodiscard]] const Texture* get(TextureID id) const noexcept;

    // Unload texture
    void unload(TextureID id) noexcept;

    // Clear all textures
    void clear() noexcept;

    // Define atlas region
    void define_region(TextureID texture_id, const std::string& name, const TextureRegion& region);

    // Get atlas region
    [[nodiscard]] const TextureRegion* get_region(TextureID texture_id, const std::string& name) const noexcept;

private:
    SDL_Renderer* renderer_{nullptr};
    std::unordered_map<TextureID, std::unique_ptr<Texture>> textures_;
    std::unordered_map<std::string, TextureID> path_to_id_;
    std::unordered_map<TextureID, std::unordered_map<std::string, TextureRegion>> atlas_regions_;
    TextureID next_id_{1};
};

} // namespace rendering
