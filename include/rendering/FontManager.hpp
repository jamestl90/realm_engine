#pragma once

#include "Texture.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace rendering {

// Forward declarations
class GPUDevice;
class TextureManager;

// Font handle type
using FontID = std::uint32_t;
constexpr FontID INVALID_FONT_ID = 0;

// Font metrics
struct FontMetrics {
    int height{0};      // Total height of font
    int ascent{0};      // Pixels above baseline
    int descent{0};     // Pixels below baseline (usually negative)
    int lineSkip{0};    // Recommended line spacing
};

// Loaded font data
struct Font {
    TTF_Font* ttf_font{nullptr};
    FontID id{INVALID_FONT_ID};
    std::string path;
    float pointSize{0.0f};
    FontMetrics metrics;
};

// Result of rendering text to a texture
struct RenderedText {
    TextureID textureId{INVALID_TEXTURE_ID};
    int width{0};
    int height{0};
};

// Font manager - handles loading fonts and rendering text
class FontManager {
public:
    explicit FontManager(GPUDevice* device, TextureManager* textureManager);
    ~FontManager();

    // Non-copyable, movable
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    FontManager(FontManager&&) noexcept;
    FontManager& operator=(FontManager&&) noexcept;

    // Load font from file at specified point size
    [[nodiscard]] FontID load(const char* path, float pointSize);

    // Resolve the same font face at another point size, loading it on demand.
    [[nodiscard]] FontID loadVariant(FontID baseFontId, float pointSize);

    // Get font by ID
    [[nodiscard]] const Font* get(FontID id) const noexcept;

    // Unload font
    void unload(FontID id) noexcept;

    // Clear all fonts
    void clear() noexcept;

    // Render text to a new texture (caller should cache result)
    [[nodiscard]] RenderedText renderText(
        FontID fontId,
        const char* text,
        std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a
    );

    // Get text dimensions without rendering
    [[nodiscard]] bool getTextSize(FontID fontId, const char* text, int* width, int* height) const;

    // Check if SDL_ttf is initialized
    [[nodiscard]] bool isInitialized() const noexcept { return m_initialized; }

private:
    GPUDevice* m_device{nullptr};
    TextureManager* m_textureManager{nullptr};
    std::unordered_map<FontID, std::unique_ptr<Font>> m_fonts;
    std::unordered_map<std::string, FontID> m_pathToId;  // path+size -> id
    FontID m_nextId{1};
    bool m_initialized{false};
};

} // namespace rendering
