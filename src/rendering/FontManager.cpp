#include "../../include/rendering/FontManager.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include "../../include/rendering/TextureManager.hpp"
#include <cassert>
#include <sstream>

namespace rendering {

FontManager::FontManager(GPUDevice* device, TextureManager* textureManager)
    : m_device(device)
    , m_textureManager(textureManager) {
    assert(device && "Device cannot be null");
    assert(textureManager && "TextureManager cannot be null");

    if (!TTF_Init()) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to initialize SDL_ttf: %s", SDL_GetError());
        return;
    }

    m_initialized = true;
    SDL_Log("FontManager: SDL_ttf initialized successfully");
}

FontManager::~FontManager() {
    clear();
    if (m_initialized) {
        TTF_Quit();
        m_initialized = false;
    }
}

FontManager::FontManager(FontManager&& other) noexcept
    : m_device(other.m_device)
    , m_textureManager(other.m_textureManager)
    , m_fonts(std::move(other.m_fonts))
    , m_pathToId(std::move(other.m_pathToId))
    , m_nextId(other.m_nextId)
    , m_initialized(other.m_initialized) {
    other.m_device = nullptr;
    other.m_textureManager = nullptr;
    other.m_initialized = false;
}

FontManager& FontManager::operator=(FontManager&& other) noexcept {
    if (this != &other) {
        clear();
        if (m_initialized) {
            TTF_Quit();
        }

        m_device = other.m_device;
        m_textureManager = other.m_textureManager;
        m_fonts = std::move(other.m_fonts);
        m_pathToId = std::move(other.m_pathToId);
        m_nextId = other.m_nextId;
        m_initialized = other.m_initialized;

        other.m_device = nullptr;
        other.m_textureManager = nullptr;
        other.m_initialized = false;
    }
    return *this;
}

FontID FontManager::load(const char* path, float pointSize) {
    if (!m_initialized) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "FontManager: SDL_ttf not initialized");
        return INVALID_FONT_ID;
    }

    // Create cache key from path + size
    std::ostringstream keyStream;
    keyStream << path << "@" << pointSize;
    std::string cacheKey = keyStream.str();

    // Check if already loaded
    auto it = m_pathToId.find(cacheKey);
    if (it != m_pathToId.end()) {
        return it->second;
    }

    // Load the font
    TTF_Font* ttfFont = TTF_OpenFont(path, pointSize);
    if (!ttfFont) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to load font '%s': %s", path, SDL_GetError());
        return INVALID_FONT_ID;
    }

    // Create font wrapper
    auto font = std::make_unique<Font>();
    font->ttf_font = ttfFont;
    font->id = m_nextId++;
    font->pointSize = pointSize;

    // Get font metrics
    font->metrics.height = TTF_GetFontHeight(ttfFont);
    font->metrics.ascent = TTF_GetFontAscent(ttfFont);
    font->metrics.descent = TTF_GetFontDescent(ttfFont);

    FontID newId = font->id;
    m_fonts[newId] = std::move(font);
    m_pathToId[cacheKey] = newId;

    SDL_Log("FontManager: Loaded font '%s' at %.1fpt (ID: %u, height: %d)",
            path, pointSize, newId, m_fonts[newId]->metrics.height);

    return newId;
}

const Font* FontManager::get(FontID id) const noexcept {
    auto it = m_fonts.find(id);
    return it != m_fonts.end() ? it->second.get() : nullptr;
}

void FontManager::unload(FontID id) noexcept {
    auto it = m_fonts.find(id);
    if (it != m_fonts.end()) {
        if (it->second->ttf_font) {
            TTF_CloseFont(it->second->ttf_font);
        }
        m_fonts.erase(it);
    }

    // Remove from path cache
    for (auto pathIt = m_pathToId.begin(); pathIt != m_pathToId.end();) {
        if (pathIt->second == id) {
            pathIt = m_pathToId.erase(pathIt);
        } else {
            ++pathIt;
        }
    }
}

void FontManager::clear() noexcept {
    for (auto& [id, font] : m_fonts) {
        if (font && font->ttf_font) {
            TTF_CloseFont(font->ttf_font);
            font->ttf_font = nullptr;
        }
    }
    m_fonts.clear();
    m_pathToId.clear();
    m_nextId = 1;
}

RenderedText FontManager::renderText(
    FontID fontId,
    const char* text,
    std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a
) {
    RenderedText result;

    if (!m_initialized || !m_textureManager) {
        return result;
    }

    const Font* font = get(fontId);
    if (!font || !font->ttf_font) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "FontManager: Invalid font ID %u", fontId);
        return result;
    }

    if (!text || text[0] == '\0') {
        return result;
    }

    // Render text to surface with blended (high quality) rendering
    SDL_Color color = { r, g, b, a };
    SDL_Surface* surface = TTF_RenderText_Blended(font->ttf_font, text, 0, color);
    if (!surface) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to render text: %s", SDL_GetError());
        return result;
    }

    result.width = surface->w;
    result.height = surface->h;

    // Convert surface to GPU texture
    result.textureId = m_textureManager->create_from_surface(surface);

    SDL_DestroySurface(surface);

    return result;
}

bool FontManager::getTextSize(FontID fontId, const char* text, int* width, int* height) const {
    if (!m_initialized) {
        return false;
    }

    const Font* font = get(fontId);
    if (!font || !font->ttf_font) {
        return false;
    }

    if (!text || text[0] == '\0') {
        if (width) *width = 0;
        if (height) *height = font->metrics.height;
        return true;
    }

    return TTF_GetStringSize(font->ttf_font, text, 0, width, height);
}

} // namespace rendering
