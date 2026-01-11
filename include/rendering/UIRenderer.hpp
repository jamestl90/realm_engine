#pragma once

#include "../ui/UIElement.hpp"
#include "../ui/Primitives.hpp"
#include "../ui/Button.hpp"
#include "../ui/TextBox.hpp"
#include "Sprite.hpp"
#include "Texture.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <string>

namespace rendering {

// Forward declarations
class GPUDevice;
class TextureManager;

// UI render command types
enum class UICommandType : std::uint8_t {
    Rectangle,
    TexturedRect,
    Text
};

// UI render command
struct UIRenderCommand {
    UICommandType type{UICommandType::Rectangle};
    float x{0.0f};
    float y{0.0f};
    float width{0.0f};
    float height{0.0f};
    std::uint8_t r{255};
    std::uint8_t g{255};
    std::uint8_t b{255};
    std::uint8_t a{255};

    // For textured rects
    TextureID textureId{INVALID_TEXTURE_ID};
    float u0{0.0f}, v0{0.0f};
    float u1{1.0f}, v1{1.0f};

    // For text
    std::string text;
    float fontSize{14.0f};

    // Border
    float borderThickness{0.0f};
    std::uint8_t borderR{0};
    std::uint8_t borderG{0};
    std::uint8_t borderB{0};
    std::uint8_t borderA{0};

    // Corner radius (for future rounded rect support)
    float cornerRadius{0.0f};
};

// UI Renderer - renders UI elements using the sprite pipeline
class UIRenderer {
public:
    explicit UIRenderer(GPUDevice* device);
    ~UIRenderer();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;
    UIRenderer(UIRenderer&&) noexcept;
    UIRenderer& operator=(UIRenderer&&) noexcept;

    // Set texture manager for textured elements
    void setTextureManager(TextureManager* manager) noexcept { m_textureManager = manager; }

    // Render UI tree
    void render(
        SDL_GPUCommandBuffer* commandBuffer,
        SDL_GPURenderPass* renderPass,
        ui::UIElement* root,
        float screenWidth,
        float screenHeight
    );

    // Clear render commands
    void clear() noexcept { m_commands.clear(); }

private:
    // Collect render commands from UI tree
    void collectCommands(ui::UIElement* element);

    // Collect commands for specific element types
    void collectButtonCommands(ui::Button* button);
    void collectTextBoxCommands(ui::TextBox* textBox);
    void collectRectangleCommands(ui::Rectangle* rect);
    void collectTextBlockCommands(ui::TextBlock* text);
    void collectImageCommands(ui::Image* image);

    // Execute render commands
    void executeCommands(
        SDL_GPUCommandBuffer* commandBuffer,
        SDL_GPURenderPass* renderPass,
        float screenWidth,
        float screenHeight
    );

    // Generate vertices for a rectangle
    void generateRectVertices(
        const UIRenderCommand& cmd,
        SpriteVertex* vertices,
        float screenWidth,
        float screenHeight
    ) const;

    GPUDevice* m_device{nullptr};
    TextureManager* m_textureManager{nullptr};
    std::vector<UIRenderCommand> m_commands;

    // Vertex/index buffers for UI rendering
    std::vector<SpriteVertex> m_vertices;
    std::vector<std::uint16_t> m_indices;

    // GPU buffers
    SDL_GPUBuffer* m_vertexBuffer{nullptr};
    SDL_GPUBuffer* m_indexBuffer{nullptr};
    std::size_t m_vertexBufferSize{0};
    std::size_t m_indexBufferSize{0};

    // White texture for solid colour rendering
    TextureID m_whiteTexture{INVALID_TEXTURE_ID};
};

} // namespace rendering