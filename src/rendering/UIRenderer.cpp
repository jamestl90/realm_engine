#include "../../include/rendering/UIRenderer.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include "../../include/rendering/TextureManager.hpp"
#include <algorithm>
#include <cstring>

namespace rendering {

UIRenderer::UIRenderer(GPUDevice* device)
    : m_device(device) {
    m_commands.reserve(256);
    m_vertices.reserve(1024);
    m_indices.reserve(1536);
}

UIRenderer::~UIRenderer() {
    if (m_device && m_device->is_valid()) {
        SDL_GPUDevice* gpu = m_device->handle();
        if (m_vertexBuffer) {
            SDL_ReleaseGPUBuffer(gpu, m_vertexBuffer);
        }
        if (m_indexBuffer) {
            SDL_ReleaseGPUBuffer(gpu, m_indexBuffer);
        }
    }
}

UIRenderer::UIRenderer(UIRenderer&& other) noexcept
    : m_device(other.m_device)
    , m_textureManager(other.m_textureManager)
    , m_commands(std::move(other.m_commands))
    , m_vertices(std::move(other.m_vertices))
    , m_indices(std::move(other.m_indices))
    , m_vertexBuffer(other.m_vertexBuffer)
    , m_indexBuffer(other.m_indexBuffer)
    , m_vertexBufferSize(other.m_vertexBufferSize)
    , m_indexBufferSize(other.m_indexBufferSize)
    , m_whiteTexture(other.m_whiteTexture) {
    other.m_device = nullptr;
    other.m_textureManager = nullptr;
    other.m_vertexBuffer = nullptr;
    other.m_indexBuffer = nullptr;
}

UIRenderer& UIRenderer::operator=(UIRenderer&& other) noexcept {
    if (this != &other) {
        if (m_device && m_device->is_valid()) {
            SDL_GPUDevice* gpu = m_device->handle();
            if (m_vertexBuffer) {
                SDL_ReleaseGPUBuffer(gpu, m_vertexBuffer);
            }
            if (m_indexBuffer) {
                SDL_ReleaseGPUBuffer(gpu, m_indexBuffer);
            }
        }

        m_device = other.m_device;
        m_textureManager = other.m_textureManager;
        m_commands = std::move(other.m_commands);
        m_vertices = std::move(other.m_vertices);
        m_indices = std::move(other.m_indices);
        m_vertexBuffer = other.m_vertexBuffer;
        m_indexBuffer = other.m_indexBuffer;
        m_vertexBufferSize = other.m_vertexBufferSize;
        m_indexBufferSize = other.m_indexBufferSize;
        m_whiteTexture = other.m_whiteTexture;

        other.m_device = nullptr;
        other.m_textureManager = nullptr;
        other.m_vertexBuffer = nullptr;
        other.m_indexBuffer = nullptr;
    }
    return *this;
}

void UIRenderer::render(
    SDL_GPUCommandBuffer* commandBuffer,
    SDL_GPURenderPass* renderPass,
    ui::UIElement* root,
    float screenWidth,
    float screenHeight
) {
    if (!root || !commandBuffer || !renderPass) {
        return;
    }

    // Clear and collect commands
    m_commands.clear();
    collectCommands(root);

    if (m_commands.empty()) {
        return;
    }

    // Execute commands
    executeCommands(commandBuffer, renderPass, screenWidth, screenHeight);
}

void UIRenderer::collectCommands(ui::UIElement* element) {
    if (!element || element->visibility() != ui::Visibility::Visible) {
        return;
    }

    // Check element type and collect appropriate commands
    if (auto* button = dynamic_cast<ui::Button*>(element)) {
        collectButtonCommands(button);
    } else if (auto* textBox = dynamic_cast<ui::TextBox*>(element)) {
        collectTextBoxCommands(textBox);
    } else if (auto* rect = dynamic_cast<ui::Rectangle*>(element)) {
        collectRectangleCommands(rect);
    } else if (auto* text = dynamic_cast<ui::TextBlock*>(element)) {
        collectTextBlockCommands(text);
    } else if (auto* image = dynamic_cast<ui::Image*>(element)) {
        collectImageCommands(image);
    }

    // Collect commands from children
    for (const auto& child : element->children()) {
        if (child) {
            collectCommands(child.get());
        }
    }
}

void UIRenderer::collectButtonCommands(ui::Button* button) {
    const auto& bounds = button->bounds();

    // Background rectangle
    UIRenderCommand bgCmd;
    bgCmd.type = UICommandType::Rectangle;
    bgCmd.x = bounds.x;
    bgCmd.y = bounds.y;
    bgCmd.width = bounds.width;
    bgCmd.height = bounds.height;

    ui::Colour bg = button->currentBackgroundColour();
    bgCmd.r = bg.r;
    bgCmd.g = bg.g;
    bgCmd.b = bg.b;
    bgCmd.a = bg.a;

    bgCmd.borderThickness = button->borderThickness();
    const auto& border = button->borderColour();
    bgCmd.borderR = border.r;
    bgCmd.borderG = border.g;
    bgCmd.borderB = border.b;
    bgCmd.borderA = border.a;
    bgCmd.cornerRadius = button->cornerRadius();
    m_commands.push_back(bgCmd);

    // Text (centered)
    if (!button->text().empty()) {
        UIRenderCommand textCmd;
        textCmd.type = UICommandType::Text;
        textCmd.text = button->text();
        textCmd.fontSize = button->fontSize();

        const auto& textColour = button->textColour();
        textCmd.r = textColour.r;
        textCmd.g = textColour.g;
        textCmd.b = textColour.b;
        textCmd.a = textColour.a;

        // Center text in button
        float charWidth = textCmd.fontSize * 0.6f;
        float textWidth = static_cast<float>(textCmd.text.length()) * charWidth;
        float textHeight = textCmd.fontSize;

        textCmd.x = bounds.x + (bounds.width - textWidth) * 0.5f;
        textCmd.y = bounds.y + (bounds.height - textHeight) * 0.5f;
        textCmd.width = textWidth;
        textCmd.height = textHeight;
        m_commands.push_back(textCmd);
    }
}

void UIRenderer::collectTextBoxCommands(ui::TextBox* textBox) {
    const auto& bounds = textBox->bounds();
    const auto& padding = textBox->padding();

    // Background rectangle
    UIRenderCommand bgCmd;
    bgCmd.type = UICommandType::Rectangle;
    bgCmd.x = bounds.x;
    bgCmd.y = bounds.y;
    bgCmd.width = bounds.width;
    bgCmd.height = bounds.height;

    const auto& bg = textBox->backgroundColour();
    bgCmd.r = bg.r;
    bgCmd.g = bg.g;
    bgCmd.b = bg.b;
    bgCmd.a = bg.a;

    bgCmd.borderThickness = textBox->borderThickness();
    ui::Colour border = textBox->currentBorderColour();
    bgCmd.borderR = border.r;
    bgCmd.borderG = border.g;
    bgCmd.borderB = border.b;
    bgCmd.borderA = border.a;
    m_commands.push_back(bgCmd);

    // Selection highlight (if any)
    if (textBox->hasSelection() && textBox->isFocused()) {
        std::size_t selStart = std::min(textBox->selectionStart(), textBox->selectionEnd());
        std::size_t selEnd = std::max(textBox->selectionStart(), textBox->selectionEnd());

        float charWidth = textBox->fontSize() * 0.6f;
        float selX = bounds.x + padding.left + static_cast<float>(selStart) * charWidth;
        float selWidth = static_cast<float>(selEnd - selStart) * charWidth;

        UIRenderCommand selCmd;
        selCmd.type = UICommandType::Rectangle;
        selCmd.x = selX;
        selCmd.y = bounds.y + padding.top;
        selCmd.width = selWidth;
        selCmd.height = textBox->fontSize();

        const auto& selColour = textBox->selectionColour();
        selCmd.r = selColour.r;
        selCmd.g = selColour.g;
        selCmd.b = selColour.b;
        selCmd.a = selColour.a;
        m_commands.push_back(selCmd);
    }

    // Text or placeholder
    std::string displayText = textBox->displayText();
    bool showPlaceholder = displayText.empty() && !textBox->placeholder().empty();

    if (showPlaceholder) {
        displayText = textBox->placeholder();
    }

    if (!displayText.empty()) {
        UIRenderCommand textCmd;
        textCmd.type = UICommandType::Text;
        textCmd.text = displayText;
        textCmd.fontSize = textBox->fontSize();
        textCmd.x = bounds.x + padding.left;
        textCmd.y = bounds.y + padding.top;

        if (showPlaceholder) {
            const auto& phColour = textBox->placeholderColour();
            textCmd.r = phColour.r;
            textCmd.g = phColour.g;
            textCmd.b = phColour.b;
            textCmd.a = phColour.a;
        } else {
            const auto& textColour = textBox->textColour();
            textCmd.r = textColour.r;
            textCmd.g = textColour.g;
            textCmd.b = textColour.b;
            textCmd.a = textColour.a;
        }

        m_commands.push_back(textCmd);
    }

    // Cursor
    if (textBox->isFocused() && textBox->isCursorVisible()) {
        float charWidth = textBox->fontSize() * 0.6f;
        float cursorX = bounds.x + padding.left +
            static_cast<float>(textBox->cursorPosition()) * charWidth;

        UIRenderCommand cursorCmd;
        cursorCmd.type = UICommandType::Rectangle;
        cursorCmd.x = cursorX;
        cursorCmd.y = bounds.y + padding.top;
        cursorCmd.width = 2.0f;
        cursorCmd.height = textBox->fontSize();

        const auto& cursorColour = textBox->cursorColour();
        cursorCmd.r = cursorColour.r;
        cursorCmd.g = cursorColour.g;
        cursorCmd.b = cursorColour.b;
        cursorCmd.a = cursorColour.a;
        m_commands.push_back(cursorCmd);
    }
}

void UIRenderer::collectRectangleCommands(ui::Rectangle* rect) {
    const auto& bounds = rect->bounds();

    UIRenderCommand cmd;
    cmd.type = UICommandType::Rectangle;
    cmd.x = bounds.x;
    cmd.y = bounds.y;
    cmd.width = bounds.width;
    cmd.height = bounds.height;

    const auto& fill = rect->fill();
    cmd.r = fill.r;
    cmd.g = fill.g;
    cmd.b = fill.b;
    cmd.a = fill.a;

    cmd.borderThickness = rect->borderThickness();
    const auto& border = rect->borderColour();
    cmd.borderR = border.r;
    cmd.borderG = border.g;
    cmd.borderB = border.b;
    cmd.borderA = border.a;

    cmd.cornerRadius = rect->cornerRadius();
    m_commands.push_back(cmd);
}

void UIRenderer::collectTextBlockCommands(ui::TextBlock* text) {
    if (text->text().empty()) {
        return;
    }

    const auto& bounds = text->bounds();

    UIRenderCommand cmd;
    cmd.type = UICommandType::Text;
    cmd.text = text->text();
    cmd.fontSize = text->fontSize();
    cmd.x = bounds.x;
    cmd.y = bounds.y;
    cmd.width = bounds.width;
    cmd.height = bounds.height;

    const auto& colour = text->colour();
    cmd.r = colour.r;
    cmd.g = colour.g;
    cmd.b = colour.b;
    cmd.a = colour.a;

    m_commands.push_back(cmd);
}

void UIRenderer::collectImageCommands(ui::Image* image) {
    if (image->textureId() == INVALID_TEXTURE_ID) {
        return;
    }

    const auto& bounds = image->bounds();

    UIRenderCommand cmd;
    cmd.type = UICommandType::TexturedRect;
    cmd.x = bounds.x;
    cmd.y = bounds.y;
    cmd.width = bounds.width;
    cmd.height = bounds.height;
    cmd.textureId = image->textureId();

    const auto& tint = image->tint();
    cmd.r = tint.r;
    cmd.g = tint.g;
    cmd.b = tint.b;
    cmd.a = tint.a;

    // Get UV coords from region if specified
    if (m_textureManager && !image->regionName().empty()) {
        const auto* region = m_textureManager->get_region(
            image->textureId(), image->regionName());
        if (region) {
            cmd.u0 = region->u0;
            cmd.v0 = region->v0;
            cmd.u1 = region->u1;
            cmd.v1 = region->v1;
        }
    }

    m_commands.push_back(cmd);
}

void UIRenderer::executeCommands(
    SDL_GPUCommandBuffer* commandBuffer,
    SDL_GPURenderPass* renderPass,
    float screenWidth,
    float screenHeight
) {
    if (m_commands.empty() || !m_device || !m_device->is_valid()) {
        return;
    }

    // Build vertex and index data
    m_vertices.clear();
    m_indices.clear();

    std::uint16_t vertexOffset = 0;

    for (const auto& cmd : m_commands) {
        if (cmd.type == UICommandType::Text) {
            // Text rendering - draw each character as a coloured rectangle for now
            // This is a placeholder until proper font rendering is implemented
            float charWidth = cmd.fontSize * 0.6f;
            float charHeight = cmd.fontSize;
            float x = cmd.x;

            for (char c : cmd.text) {
                if (c == ' ') {
                    x += charWidth;
                    continue;
                }

                // Simple coloured rectangle per character (placeholder)
                UIRenderCommand charCmd = cmd;
                charCmd.type = UICommandType::Rectangle;
                charCmd.x = x;
                charCmd.width = charWidth * 0.8f;
                charCmd.height = charHeight;

                SpriteVertex verts[4];
                generateRectVertices(charCmd, verts, screenWidth, screenHeight);

                for (int i = 0; i < 4; ++i) {
                    m_vertices.push_back(verts[i]);
                }

                m_indices.push_back(vertexOffset + 0);
                m_indices.push_back(vertexOffset + 1);
                m_indices.push_back(vertexOffset + 2);
                m_indices.push_back(vertexOffset + 2);
                m_indices.push_back(vertexOffset + 3);
                m_indices.push_back(vertexOffset + 0);

                vertexOffset += 4;
                x += charWidth;
            }
        } else {
            // Rectangle or textured rect
            SpriteVertex verts[4];
            generateRectVertices(cmd, verts, screenWidth, screenHeight);

            for (int i = 0; i < 4; ++i) {
                m_vertices.push_back(verts[i]);
            }

            m_indices.push_back(vertexOffset + 0);
            m_indices.push_back(vertexOffset + 1);
            m_indices.push_back(vertexOffset + 2);
            m_indices.push_back(vertexOffset + 2);
            m_indices.push_back(vertexOffset + 3);
            m_indices.push_back(vertexOffset + 0);

            vertexOffset += 4;

            // Draw border if specified
            if (cmd.borderThickness > 0.0f && cmd.borderA > 0) {
                // Top border
                UIRenderCommand borderCmd;
                borderCmd.type = UICommandType::Rectangle;
                borderCmd.x = cmd.x;
                borderCmd.y = cmd.y;
                borderCmd.width = cmd.width;
                borderCmd.height = cmd.borderThickness;
                borderCmd.r = cmd.borderR;
                borderCmd.g = cmd.borderG;
                borderCmd.b = cmd.borderB;
                borderCmd.a = cmd.borderA;

                generateRectVertices(borderCmd, verts, screenWidth, screenHeight);
                for (int i = 0; i < 4; ++i) m_vertices.push_back(verts[i]);
                m_indices.push_back(vertexOffset + 0);
                m_indices.push_back(vertexOffset + 1);
                m_indices.push_back(vertexOffset + 2);
                m_indices.push_back(vertexOffset + 2);
                m_indices.push_back(vertexOffset + 3);
                m_indices.push_back(vertexOffset + 0);
                vertexOffset += 4;

                // Bottom border
                borderCmd.y = cmd.y + cmd.height - cmd.borderThickness;
                generateRectVertices(borderCmd, verts, screenWidth, screenHeight);
                for (int i = 0; i < 4; ++i) m_vertices.push_back(verts[i]);
                m_indices.push_back(vertexOffset + 0);
                m_indices.push_back(vertexOffset + 1);
                m_indices.push_back(vertexOffset + 2);
                m_indices.push_back(vertexOffset + 2);
                m_indices.push_back(vertexOffset + 3);
                m_indices.push_back(vertexOffset + 0);
                vertexOffset += 4;

                // Left border
                borderCmd.x = cmd.x;
                borderCmd.y = cmd.y + cmd.borderThickness;
                borderCmd.width = cmd.borderThickness;
                borderCmd.height = cmd.height - 2.0f * cmd.borderThickness;
                generateRectVertices(borderCmd, verts, screenWidth, screenHeight);
                for (int i = 0; i < 4; ++i) m_vertices.push_back(verts[i]);
                m_indices.push_back(vertexOffset + 0);
                m_indices.push_back(vertexOffset + 1);
                m_indices.push_back(vertexOffset + 2);
                m_indices.push_back(vertexOffset + 2);
                m_indices.push_back(vertexOffset + 3);
                m_indices.push_back(vertexOffset + 0);
                vertexOffset += 4;

                // Right border
                borderCmd.x = cmd.x + cmd.width - cmd.borderThickness;
                generateRectVertices(borderCmd, verts, screenWidth, screenHeight);
                for (int i = 0; i < 4; ++i) m_vertices.push_back(verts[i]);
                m_indices.push_back(vertexOffset + 0);
                m_indices.push_back(vertexOffset + 1);
                m_indices.push_back(vertexOffset + 2);
                m_indices.push_back(vertexOffset + 2);
                m_indices.push_back(vertexOffset + 3);
                m_indices.push_back(vertexOffset + 0);
                vertexOffset += 4;
            }
        }
    }

    if (m_vertices.empty()) {
        return;
    }

    SDL_GPUDevice* gpu = m_device->handle();

    // Resize vertex buffer if needed
    std::size_t requiredVertexSize = m_vertices.size() * sizeof(SpriteVertex);
    if (requiredVertexSize > m_vertexBufferSize) {
        if (m_vertexBuffer) {
            SDL_ReleaseGPUBuffer(gpu, m_vertexBuffer);
        }

        SDL_GPUBufferCreateInfo bufferInfo{};
        bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        bufferInfo.size = static_cast<Uint32>(requiredVertexSize * 2); // Double for growth
        m_vertexBuffer = SDL_CreateGPUBuffer(gpu, &bufferInfo);
        m_vertexBufferSize = bufferInfo.size;
    }

    // Resize index buffer if needed
    std::size_t requiredIndexSize = m_indices.size() * sizeof(std::uint16_t);
    if (requiredIndexSize > m_indexBufferSize) {
        if (m_indexBuffer) {
            SDL_ReleaseGPUBuffer(gpu, m_indexBuffer);
        }

        SDL_GPUBufferCreateInfo bufferInfo{};
        bufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        bufferInfo.size = static_cast<Uint32>(requiredIndexSize * 2);
        m_indexBuffer = SDL_CreateGPUBuffer(gpu, &bufferInfo);
        m_indexBufferSize = bufferInfo.size;
    }

    // Upload vertex data
    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<Uint32>(requiredVertexSize + requiredIndexSize);

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(gpu, &transferInfo);
    if (!transferBuffer) {
        return;
    }

    void* mapped = SDL_MapGPUTransferBuffer(gpu, transferBuffer, false);
    if (mapped) {
        std::memcpy(mapped, m_vertices.data(), requiredVertexSize);
        std::memcpy(static_cast<std::uint8_t*>(mapped) + requiredVertexSize,
                    m_indices.data(), requiredIndexSize);
        SDL_UnmapGPUTransferBuffer(gpu, transferBuffer);
    }

    // Copy to GPU buffers
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

    SDL_GPUTransferBufferLocation srcVertex{};
    srcVertex.transfer_buffer = transferBuffer;
    srcVertex.offset = 0;

    SDL_GPUBufferRegion dstVertex{};
    dstVertex.buffer = m_vertexBuffer;
    dstVertex.offset = 0;
    dstVertex.size = static_cast<Uint32>(requiredVertexSize);

    SDL_UploadToGPUBuffer(copyPass, &srcVertex, &dstVertex, false);

    SDL_GPUTransferBufferLocation srcIndex{};
    srcIndex.transfer_buffer = transferBuffer;
    srcIndex.offset = static_cast<Uint32>(requiredVertexSize);

    SDL_GPUBufferRegion dstIndex{};
    dstIndex.buffer = m_indexBuffer;
    dstIndex.offset = 0;
    dstIndex.size = static_cast<Uint32>(requiredIndexSize);

    SDL_UploadToGPUBuffer(copyPass, &srcIndex, &dstIndex, false);

    SDL_EndGPUCopyPass(copyPass);

    SDL_ReleaseGPUTransferBuffer(gpu, transferBuffer);

    // Bind buffers and draw
    SDL_GPUBufferBinding vertexBinding{};
    vertexBinding.buffer = m_vertexBuffer;
    vertexBinding.offset = 0;

    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    SDL_GPUBufferBinding indexBinding{};
    indexBinding.buffer = m_indexBuffer;
    indexBinding.offset = 0;

    SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_DrawGPUIndexedPrimitives(renderPass, static_cast<Uint32>(m_indices.size()), 1, 0, 0, 0);
}



void UIRenderer::generateRectVertices(
    const UIRenderCommand& cmd,
    SpriteVertex* vertices,
    float screenWidth,
    float screenHeight
) const {
    // Convert screen coordinates to NDC (-1 to 1)
    float x0 = (cmd.x / screenWidth) * 2.0f - 1.0f;
    float y0 = 1.0f - (cmd.y / screenHeight) * 2.0f;
    float x1 = ((cmd.x + cmd.width) / screenWidth) * 2.0f - 1.0f;
    float y1 = 1.0f - ((cmd.y + cmd.height) / screenHeight) * 2.0f;

    // UV coordinates
    float u0 = cmd.u0;
    float v0 = cmd.v0;
    float u1 = cmd.u1;
    float v1 = cmd.v1;

    // Top-left
    vertices[0].x = x0;
    vertices[0].y = y0;
    vertices[0].u = u0;
    vertices[0].v = v0;
    vertices[0].r = cmd.r;
    vertices[0].g = cmd.g;
    vertices[0].b = cmd.b;
    vertices[0].a = cmd.a;

    // Top-right
    vertices[1].x = x1;
    vertices[1].y = y0;
    vertices[1].u = u1;
    vertices[1].v = v0;
    vertices[1].r = cmd.r;
    vertices[1].g = cmd.g;
    vertices[1].b = cmd.b;
    vertices[1].a = cmd.a;

    // Bottom-right
    vertices[2].x = x1;
    vertices[2].y = y1;
    vertices[2].u = u1;
    vertices[2].v = v1;
    vertices[2].r = cmd.r;
    vertices[2].g = cmd.g;
    vertices[2].b = cmd.b;
    vertices[2].a = cmd.a;

    // Bottom-left
    vertices[3].x = x0;
    vertices[3].y = y1;
    vertices[3].u = u0;
    vertices[3].v = v1;
    vertices[3].r = cmd.r;
    vertices[3].g = cmd.g;
    vertices[3].b = cmd.b;
    vertices[3].a = cmd.a;
}


} // namespace rendering