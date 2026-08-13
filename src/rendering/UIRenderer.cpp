#include "../../include/rendering/UIRenderer.hpp"
#include "../../include/rendering/GPUDevice.hpp"
#include "../../include/rendering/TextureManager.hpp"
#include "../../include/rendering/UniformBuffers.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <rendering/PipelineManager.hpp>

namespace rendering {

UIRenderer::UIRenderer(GPUDevice* device)
    : m_device(device) {
    m_commands.reserve(256);
    m_vertices.reserve(1024);
    m_indices.reserve(1536);
}

UIRenderer::~UIRenderer() {
    if (m_textEngine) {
        TTF_DestroyGPUTextEngine(m_textEngine);
        m_textEngine = nullptr;
    }

    if (m_device && m_device->is_valid()) {
        SDL_GPUDevice* gpu = m_device->handle();
        if (m_sampler) {
            SDL_ReleaseGPUSampler(gpu, m_sampler);
        }
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
    , m_fontManager(other.m_fontManager)
    , m_defaultFont(other.m_defaultFont)
    , m_textEngine(other.m_textEngine)
    , m_commands(std::move(other.m_commands))
    , m_vertices(std::move(other.m_vertices))
    , m_indices(std::move(other.m_indices))
    , m_vertexBuffer(other.m_vertexBuffer)
    , m_indexBuffer(other.m_indexBuffer)
    , m_vertexBufferSize(other.m_vertexBufferSize)
    , m_indexBufferSize(other.m_indexBufferSize)
    , m_sampler(other.m_sampler)
    , m_whiteTexture(other.m_whiteTexture) {
    other.m_device = nullptr;
    other.m_textureManager = nullptr;
    other.m_fontManager = nullptr;
    other.m_textEngine = nullptr;
    other.m_vertexBuffer = nullptr;
    other.m_indexBuffer = nullptr;
    other.m_sampler = nullptr;
}

UIRenderer& UIRenderer::operator=(UIRenderer&& other) noexcept {
    if (this != &other) {
        if (m_textEngine) {
            TTF_DestroyGPUTextEngine(m_textEngine);
        }

        if (m_device && m_device->is_valid()) {
            SDL_GPUDevice* gpu = m_device->handle();
            if (m_sampler) {
                SDL_ReleaseGPUSampler(gpu, m_sampler);
            }
            if (m_vertexBuffer) {
                SDL_ReleaseGPUBuffer(gpu, m_vertexBuffer);
            }
            if (m_indexBuffer) {
                SDL_ReleaseGPUBuffer(gpu, m_indexBuffer);
            }
        }

        m_device = other.m_device;
        m_textureManager = other.m_textureManager;
        m_fontManager = other.m_fontManager;
        m_defaultFont = other.m_defaultFont;
        m_textEngine = other.m_textEngine;
        m_commands = std::move(other.m_commands);
        m_openComboBoxes.clear();
        m_vertices = std::move(other.m_vertices);
        m_indices = std::move(other.m_indices);
        m_vertexBuffer = other.m_vertexBuffer;
        m_indexBuffer = other.m_indexBuffer;
        m_vertexBufferSize = other.m_vertexBufferSize;
        m_indexBufferSize = other.m_indexBufferSize;
        m_sampler = other.m_sampler;
        m_whiteTexture = other.m_whiteTexture;

        other.m_device = nullptr;
        other.m_textureManager = nullptr;
        other.m_fontManager = nullptr;
        other.m_textEngine = nullptr;
        other.m_vertexBuffer = nullptr;
        other.m_indexBuffer = nullptr;
        other.m_sampler = nullptr;
    }
    return *this;
}

FontID UIRenderer::loadFont(const char* path, float pointSize) {
    if (!m_fontManager) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "UIRenderer: No FontManager set");
        return INVALID_FONT_ID;
    }
    return m_fontManager->load(path, pointSize);
}

FontID UIRenderer::resolveFont(FontID fontId, float fontSize) {
    if (!m_fontManager) {
        return INVALID_FONT_ID;
    }

    const FontID baseFont = fontId != INVALID_FONT_ID ? fontId : m_defaultFont;
    if (baseFont == INVALID_FONT_ID) {
        return INVALID_FONT_ID;
    }

    return m_fontManager->loadVariant(baseFont, fontSize);
}

bool UIRenderer::measureText(
    const std::string& text,
    FontID fontId,
    float fontSize,
    int* outWidth,
    int* outHeight
) {
    const FontID resolvedFont = resolveFont(fontId, fontSize);
    if (resolvedFont != INVALID_FONT_ID
        && m_fontManager->getTextSize(resolvedFont, text.c_str(), outWidth, outHeight)) {
        return true;
    }

    if (outWidth) {
        *outWidth = static_cast<int>(text.length() * fontSize * 0.6f);
    }
    if (outHeight) {
        *outHeight = static_cast<int>(fontSize * 1.2f);
    }
    return false;
}

bool UIRenderer::ensureTextEngine() {
    if (m_textEngine) {
        return true;
    }

    if (!m_device || !m_device->is_valid() || !m_fontManager || !m_fontManager->isInitialized()) {
        return false;
    }

    m_textEngine = TTF_CreateGPUTextEngine(m_device->handle());
    if (!m_textEngine) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create GPU text engine: %s", SDL_GetError());
        return false;
    }

    TTF_SetGPUTextEngineWinding(m_textEngine, TTF_GPU_TEXTENGINE_WINDING_CLOCKWISE);
    return true;
}

void UIRenderer::render(
    SDL_GPUCommandBuffer* commandBuffer,
    SDL_GPUTexture* swapchainTexture,
    ui::UIElement* root,
    float logicalWidth,
    float logicalHeight
) {
    if (!root || !commandBuffer || !swapchainTexture) {
        return;
    }

    // Clear and collect commands
    m_commands.clear();
    m_openComboBoxes.clear();
    collectCommands(root);

    // Popup content renders after the normal tree so later siblings cannot
    // cover an open dropdown.
    for (auto* comboBox : m_openComboBoxes) {
        collectComboBoxDropdownCommands(comboBox);
    }

    if (m_commands.empty()) {
        return;
    }

    // Execute commands (handles copy pass + render pass internally)
    executeCommands(commandBuffer, swapchainTexture, logicalWidth, logicalHeight);
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
    } else if (auto* comboBox = dynamic_cast<ui::ComboBox*>(element)) {
        collectComboBoxCommands(comboBox);
    } else if (auto* rect = dynamic_cast<ui::Rectangle*>(element)) {
        collectRectangleCommands(rect);
    } else if (auto* text = dynamic_cast<ui::TextBlock*>(element)) {
        collectTextBlockCommands(text);
    } else if (auto* image = dynamic_cast<ui::Image*>(element)) {
        collectImageCommands(image);
    } else if (auto* container = dynamic_cast<ui::LayoutContainer*>(element)) {
        collectLayoutContainerCommands(container);
    }

    // Collect commands from children
    for (const auto& child : element->children()) {
        if (child) {
            collectCommands(child.get());
        }
    }
}

void UIRenderer::collectLayoutContainerCommands(ui::LayoutContainer* container) {
    const auto& bg = container->backgroundColour();

    // Only render background if it has visible alpha
    if (bg.a == 0) {
        return;
    }

    const auto& bounds = container->bounds();

    UIRenderCommand bgCmd;
    bgCmd.type = UICommandType::Rectangle;
    bgCmd.x = bounds.x;
    bgCmd.y = bounds.y;
    bgCmd.width = bounds.width;
    bgCmd.height = bounds.height;
    bgCmd.r = bg.r;
    bgCmd.g = bg.g;
    bgCmd.b = bg.b;
    bgCmd.a = bg.a;

    m_commands.push_back(bgCmd);
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

        int textWidth = 0;
        int textHeight = 0;
        if (!measureText(textCmd.text, textCmd.fontId, textCmd.fontSize, &textWidth, &textHeight)) {
            textWidth = static_cast<int>(textCmd.text.length() * textCmd.fontSize * 0.6f);
            textHeight = static_cast<int>(textCmd.fontSize * 1.2f);
        }

        textCmd.x = bounds.x + (bounds.width - static_cast<float>(textWidth)) * 0.5f;
        textCmd.y = bounds.y + (bounds.height - static_cast<float>(textHeight)) * 0.5f;
        textCmd.width = static_cast<float>(textWidth);
        textCmd.height = static_cast<float>(textHeight);
        m_commands.push_back(textCmd);
    }
}

void UIRenderer::collectTextBoxCommands(ui::TextBox* textBox) {
    const auto& bounds = textBox->bounds();
    const auto& padding = textBox->padding();
    const std::string renderedText = textBox->displayText();
    int lineWidth = 0;
    int lineHeight = 0;
    if (!measureText("Mg", INVALID_FONT_ID, textBox->fontSize(), &lineWidth, &lineHeight)) {
        lineHeight = static_cast<int>(textBox->fontSize() * 1.2f);
    }

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

        int selectionStartWidth = 0;
        int selectionEndWidth = 0;
        measureText(renderedText.substr(0, selStart), INVALID_FONT_ID, textBox->fontSize(), &selectionStartWidth, nullptr);
        measureText(renderedText.substr(0, selEnd), INVALID_FONT_ID, textBox->fontSize(), &selectionEndWidth, nullptr);

        UIRenderCommand selCmd;
        selCmd.type = UICommandType::Rectangle;
        selCmd.x = bounds.x + padding.left + static_cast<float>(selectionStartWidth);
        selCmd.y = bounds.y + (bounds.height - static_cast<float>(lineHeight)) * 0.5f;
        selCmd.width = static_cast<float>(selectionEndWidth - selectionStartWidth);
        selCmd.height = static_cast<float>(lineHeight);

        const auto& selColour = textBox->selectionColour();
        selCmd.r = selColour.r;
        selCmd.g = selColour.g;
        selCmd.b = selColour.b;
        selCmd.a = selColour.a;
        m_commands.push_back(selCmd);
    }

    // Text or placeholder
    std::string displayText = renderedText;
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
        textCmd.y = bounds.y + (bounds.height - static_cast<float>(lineHeight)) * 0.5f;

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
        float cursorX = bounds.x + padding.left;

        // Measure actual text width up to cursor position
        if (textBox->cursorPosition() > 0) {
            const std::string textToCursor = renderedText.substr(0, textBox->cursorPosition());
            int textWidth = 0;
            if (measureText(textToCursor, INVALID_FONT_ID, textBox->fontSize(), &textWidth, nullptr)) {
                cursorX += static_cast<float>(textWidth);
            }
        }

        UIRenderCommand cursorCmd;
        cursorCmd.type = UICommandType::Rectangle;
        cursorCmd.x = cursorX;
        cursorCmd.y = bounds.y + (bounds.height - static_cast<float>(lineHeight)) * 0.5f;
        cursorCmd.width = 2.0f;
        cursorCmd.height = static_cast<float>(lineHeight);

        const auto& cursorColour = textBox->cursorColour();
        cursorCmd.r = cursorColour.r;
        cursorCmd.g = cursorColour.g;
        cursorCmd.b = cursorColour.b;
        cursorCmd.a = cursorColour.a;
        m_commands.push_back(cursorCmd);
    }
}

void UIRenderer::collectComboBoxCommands(ui::ComboBox* comboBox) {
    const auto& bounds = comboBox->bounds();
    const auto& padding = comboBox->padding();

    int sampleWidth = 0;
    int textHeight = 0;
    if (!measureText("Mg", INVALID_FONT_ID, comboBox->fontSize(), &sampleWidth, &textHeight)) {
        textHeight = static_cast<int>(comboBox->fontSize() * 1.2f);
    }

    float headerHeight = static_cast<float>(textHeight) + padding.verticalSum();
    headerHeight = std::max(headerHeight, 24.0f);

    // Determine header background color based on state
    ui::Colour headerBg = comboBox->backgroundColour();
    if (comboBox->isHovered() && !comboBox->isOpen()) {
        headerBg = comboBox->hoverColour();
    }

    // Header background rectangle
    UIRenderCommand headerCmd;
    headerCmd.type = UICommandType::Rectangle;
    headerCmd.x = bounds.x;
    headerCmd.y = bounds.y;
    headerCmd.width = bounds.width;
    headerCmd.height = headerHeight;
    headerCmd.r = headerBg.r;
    headerCmd.g = headerBg.g;
    headerCmd.b = headerBg.b;
    headerCmd.a = headerBg.a;
    headerCmd.borderThickness = comboBox->borderThickness();
    const auto& border = comboBox->borderColour();
    headerCmd.borderR = border.r;
    headerCmd.borderG = border.g;
    headerCmd.borderB = border.b;
    headerCmd.borderA = border.a;
    m_commands.push_back(headerCmd);

    // Header text (selected item or placeholder)
    std::string displayText = comboBox->placeholder();
    if (comboBox->selectedIndex() >= 0) {
        displayText = comboBox->selectedItem();
    }

    if (!displayText.empty()) {
        UIRenderCommand textCmd;
        textCmd.type = UICommandType::Text;
        textCmd.text = displayText;
        textCmd.fontSize = comboBox->fontSize();
        textCmd.x = bounds.x + padding.left;
        textCmd.y = bounds.y + (headerHeight - static_cast<float>(textHeight)) * 0.5f;

        const auto& textColour = comboBox->textColour();
        textCmd.r = textColour.r;
        textCmd.g = textColour.g;
        textCmd.b = textColour.b;
        // Use dimmer text for placeholder
        textCmd.a = (comboBox->selectedIndex() >= 0) ? textColour.a : static_cast<std::uint8_t>(textColour.a * 0.6f);
        m_commands.push_back(textCmd);
    }

    // Dropdown arrow indicator (simple triangle made of text)
    UIRenderCommand arrowCmd;
    arrowCmd.type = UICommandType::Text;
    arrowCmd.text = comboBox->isOpen() ? "^" : "v";
    arrowCmd.fontSize = comboBox->fontSize();
    int arrowWidth = 0;
    int arrowHeight = 0;
    measureText(arrowCmd.text, arrowCmd.fontId, arrowCmd.fontSize, &arrowWidth, &arrowHeight);
    arrowCmd.x = bounds.x + bounds.width - padding.right - static_cast<float>(arrowWidth);
    arrowCmd.y = bounds.y + (headerHeight - static_cast<float>(arrowHeight)) * 0.5f;
    const auto& textColour = comboBox->textColour();
    arrowCmd.r = textColour.r;
    arrowCmd.g = textColour.g;
    arrowCmd.b = textColour.b;
    arrowCmd.a = textColour.a;
    m_commands.push_back(arrowCmd);

    if (comboBox->isOpen() && !comboBox->items().empty()) {
        m_openComboBoxes.push_back(comboBox);
    }
}

void UIRenderer::collectComboBoxDropdownCommands(ui::ComboBox* comboBox) {
    if (!comboBox || !comboBox->isOpen() || comboBox->items().empty()) {
        return;
    }

    const auto& bounds = comboBox->bounds();
    const auto& padding = comboBox->padding();
    int sampleWidth = 0;
    int textHeight = 0;
    if (!measureText("Mg", INVALID_FONT_ID, comboBox->fontSize(), &sampleWidth, &textHeight)) {
        textHeight = static_cast<int>(comboBox->fontSize() * 1.2f);
    }

    const float headerHeight = std::max(
        static_cast<float>(textHeight) + padding.verticalSum(),
        24.0f
    );
    const float itemHeight = static_cast<float>(textHeight) + 12.0f;
    const float dropdownHeight = itemHeight * static_cast<float>(comboBox->items().size());
    const auto& border = comboBox->borderColour();
    const auto& textColour = comboBox->textColour();

    UIRenderCommand dropdownBgCmd;
    dropdownBgCmd.type = UICommandType::Rectangle;
    dropdownBgCmd.x = bounds.x;
    dropdownBgCmd.y = bounds.y + headerHeight;
    dropdownBgCmd.width = bounds.width;
    dropdownBgCmd.height = dropdownHeight;
    const auto& dropdownBg = comboBox->dropdownBackgroundColour();
    dropdownBgCmd.r = dropdownBg.r;
    dropdownBgCmd.g = dropdownBg.g;
    dropdownBgCmd.b = dropdownBg.b;
    dropdownBgCmd.a = dropdownBg.a;
    dropdownBgCmd.borderThickness = comboBox->borderThickness();
    dropdownBgCmd.borderR = border.r;
    dropdownBgCmd.borderG = border.g;
    dropdownBgCmd.borderB = border.b;
    dropdownBgCmd.borderA = border.a;
    m_commands.push_back(dropdownBgCmd);

    float itemY = bounds.y + headerHeight;
    int itemIndex = 0;
    for (const auto& item : comboBox->items()) {
        if (itemIndex == comboBox->hoveredItemIndex()) {
            UIRenderCommand itemBgCmd;
            itemBgCmd.type = UICommandType::Rectangle;
            itemBgCmd.x = bounds.x;
            itemBgCmd.y = itemY;
            itemBgCmd.width = bounds.width;
            itemBgCmd.height = itemHeight;
            const auto& itemHoverColour = comboBox->itemHoverColour();
            itemBgCmd.r = itemHoverColour.r;
            itemBgCmd.g = itemHoverColour.g;
            itemBgCmd.b = itemHoverColour.b;
            itemBgCmd.a = itemHoverColour.a;
            m_commands.push_back(itemBgCmd);
        }

        UIRenderCommand itemTextCmd;
        itemTextCmd.type = UICommandType::Text;
        itemTextCmd.text = item;
        itemTextCmd.fontSize = comboBox->fontSize();
        itemTextCmd.x = bounds.x + padding.left;
        int itemTextWidth = 0;
        int itemTextHeight = 0;
        measureText(item, itemTextCmd.fontId, itemTextCmd.fontSize, &itemTextWidth, &itemTextHeight);
        itemTextCmd.y = itemY + (itemHeight - static_cast<float>(itemTextHeight)) * 0.5f;
        itemTextCmd.r = textColour.r;
        itemTextCmd.g = textColour.g;
        itemTextCmd.b = textColour.b;
        itemTextCmd.a = textColour.a;
        m_commands.push_back(itemTextCmd);

        itemY += itemHeight;
        ++itemIndex;
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

    int textWidth = 0;
    int textHeight = 0;
    if (measureText(cmd.text, cmd.fontId, cmd.fontSize, &textWidth, &textHeight)) {
        if (text->alignment() == ui::TextAlignment::Center) {
            cmd.x = bounds.x + (bounds.width - static_cast<float>(textWidth)) * 0.5f;
        } else if (text->alignment() == ui::TextAlignment::Right) {
            cmd.x = bounds.x + bounds.width - static_cast<float>(textWidth);
        }
        cmd.y = bounds.y + (bounds.height - static_cast<float>(textHeight)) * 0.5f;
    }

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
    SDL_GPUTexture* swapchainTexture,
    float logicalWidth,
    float logicalHeight
) {
    if (m_commands.empty() || !m_device || !m_device->is_valid() || !m_textureManager) {
        return;
    }

    // Build vertex and index data
    m_vertices.clear();
    m_indices.clear();

    std::uint32_t vertexOffset = 0;

    // Separate commands into batches by texture for proper rendering
    struct RenderBatch {
        SDL_GPUTexture* texture{nullptr};
        std::uint32_t startIndex{0};
        std::uint32_t indexCount{0};
    };
    std::vector<RenderBatch> batches;
    SDL_GPUTexture* currentTexture = nullptr;
    std::uint32_t batchStartIndex = 0;

    auto finishBatch = [&]() {
        if (currentTexture && m_indices.size() > batchStartIndex) {
            RenderBatch batch;
            batch.texture = currentTexture;
            batch.startIndex = batchStartIndex;
            batch.indexCount = static_cast<std::uint32_t>(m_indices.size()) - batchStartIndex;
            batches.push_back(batch);
        }
        batchStartIndex = static_cast<std::uint32_t>(m_indices.size());
    };

    auto switchTexture = [&](SDL_GPUTexture* texture) {
        if (currentTexture != texture) {
            finishBatch();
            currentTexture = texture;
        }
    };

    auto textureForId = [&](TextureID textureId) -> SDL_GPUTexture* {
        const Texture* texture = m_textureManager->get(textureId);
        return texture ? texture->gpu_texture : nullptr;
    };

    SDL_GPUTexture* whiteTexture = textureForId(m_textureManager->default_white_texture());
    if (!whiteTexture) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "UIRenderer: Default white texture is unavailable");
        return;
    }

    for (const auto& cmd : m_commands) {
        if (cmd.type == UICommandType::Text) {
            if (cmd.text.empty()) {
                continue;
            }

            bool rendered = false;
            const FontID resolvedFont = resolveFont(cmd.fontId, cmd.fontSize);
            const Font* font = resolvedFont != INVALID_FONT_ID ? m_fontManager->get(resolvedFont) : nullptr;

            if (font && font->ttf_font && ensureTextEngine()) {
                TTF_Text* text = TTF_CreateText(m_textEngine, font->ttf_font, cmd.text.c_str(), 0);
                if (text) {
                    for (TTF_GPUAtlasDrawSequence* sequence = TTF_GetGPUTextDrawData(text);
                         sequence;
                         sequence = sequence->next) {
                        if (!sequence->atlas_texture || sequence->num_vertices <= 0 || sequence->num_indices <= 0) {
                            continue;
                        }

                        if (vertexOffset + static_cast<std::uint32_t>(sequence->num_vertices)
                            > std::numeric_limits<std::uint16_t>::max()) {
                            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "UIRenderer: UI vertex limit exceeded");
                            break;
                        }

                        switchTexture(sequence->atlas_texture);
                        const auto baseVertex = static_cast<std::uint16_t>(vertexOffset);

                        for (int i = 0; i < sequence->num_vertices; ++i) {
                            const float screenX = cmd.x + sequence->xy[i].x;
                            const float screenY = cmd.y - sequence->xy[i].y;
                            m_vertices.push_back(SpriteVertex{
                                (screenX / logicalWidth) * 2.0f - 1.0f,
                                1.0f - (screenY / logicalHeight) * 2.0f,
                                sequence->uv[i].x,
                                sequence->uv[i].y,
                                cmd.r,
                                cmd.g,
                                cmd.b,
                                cmd.a
                            });
                        }

                        for (int i = 0; i < sequence->num_indices; ++i) {
                            const int index = sequence->indices[i];
                            if (index >= 0 && index < sequence->num_vertices) {
                                m_indices.push_back(static_cast<std::uint16_t>(baseVertex + index));
                            }
                        }

                        vertexOffset += static_cast<std::uint32_t>(sequence->num_vertices);
                        rendered = true;
                    }
                    TTF_DestroyText(text);
                }
            }

            if (!rendered) {
                const float characterWidth = cmd.fontSize * 0.6f;
                float x = cmd.x;
                switchTexture(whiteTexture);

                for (const char character : cmd.text) {
                    if (character != ' ') {
                        UIRenderCommand characterCommand = cmd;
                        characterCommand.type = UICommandType::Rectangle;
                        characterCommand.x = x;
                        characterCommand.width = characterWidth * 0.8f;
                        characterCommand.height = cmd.fontSize;

                        SpriteVertex vertices[4];
                        generateRectVertices(characterCommand, vertices, logicalWidth, logicalHeight);
                        for (const auto& vertex : vertices) {
                            m_vertices.push_back(vertex);
                        }

                        const auto baseVertex = static_cast<std::uint16_t>(vertexOffset);
                        m_indices.push_back(baseVertex + 0);
                        m_indices.push_back(baseVertex + 1);
                        m_indices.push_back(baseVertex + 2);
                        m_indices.push_back(baseVertex + 2);
                        m_indices.push_back(baseVertex + 3);
                        m_indices.push_back(baseVertex + 0);
                        vertexOffset += 4;
                    }
                    x += characterWidth;
                }
            }
        } else {
            SDL_GPUTexture* commandTexture = cmd.type == UICommandType::TexturedRect
                ? textureForId(cmd.textureId)
                : whiteTexture;
            if (!commandTexture) {
                continue;
            }
            switchTexture(commandTexture);

            // Rectangle or textured rect
            SpriteVertex verts[4];
            generateRectVertices(cmd, verts, logicalWidth, logicalHeight);

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
                switchTexture(whiteTexture);

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

                generateRectVertices(borderCmd, verts, logicalWidth, logicalHeight);
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
                generateRectVertices(borderCmd, verts, logicalWidth, logicalHeight);
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
                generateRectVertices(borderCmd, verts, logicalWidth, logicalHeight);
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
                generateRectVertices(borderCmd, verts, logicalWidth, logicalHeight);
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

    // Finish the last batch
    finishBatch();

    if (m_vertices.empty() || batches.empty()) {
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

    // Upload vertex data via copy pass (no render pass should be active)
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

    // Copy pass for uploading vertex/index data
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

    // Start render pass for drawing (LOAD to preserve existing content)
    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = swapchainTexture;
    colorTarget.load_op = SDL_GPU_LOADOP_LOAD;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);
    if (!renderPass) {
        return;
    }

    PipelineManager* pipeline_mgr = m_device->pipeline_manager();
    if (!pipeline_mgr) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "No pipeline manager available");
        SDL_EndGPURenderPass(renderPass);
        return;
    }

    SDL_GPUGraphicsPipeline* sprite_pipeline = pipeline_mgr->get_core_pipeline(PipelineType::Sprite);
    if (!sprite_pipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Sprite pipeline not available");
        SDL_EndGPURenderPass(renderPass);
        return;
    }

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, sprite_pipeline);

    // Push identity matrix for camera uniform (positions are already in NDC)
    CameraData camera_data{};
    // Identity matrix (column-major)
    camera_data.projection[0] = 1.0f;   // [0][0]
    camera_data.projection[5] = 1.0f;   // [1][1]
    camera_data.projection[10] = 1.0f;  // [2][2]
    camera_data.projection[15] = 1.0f;  // [3][3]
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &camera_data, sizeof(CameraData));

    // Bind buffers and draw
    SDL_GPUBufferBinding vertexBinding{};
    vertexBinding.buffer = m_vertexBuffer;
    vertexBinding.offset = 0;

    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    SDL_GPUBufferBinding indexBinding{};
    indexBinding.buffer = m_indexBuffer;
    indexBinding.offset = 0;

    SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Create sampler lazily if needed (LINEAR for smoother text rendering)
    if (!m_sampler) {
        SDL_GPUSamplerCreateInfo samplerInfo{};
        samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
        samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
        m_sampler = SDL_CreateGPUSampler(gpu, &samplerInfo);
        if (!m_sampler) {
            SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Failed to create UI sampler: %s", SDL_GetError());
            SDL_EndGPURenderPass(renderPass);
            return;
        }
    }

    // Draw each batch with its own texture
    for (const auto& batch : batches) {
        if (!batch.texture) {
            continue;
        }

        // Bind texture and sampler for this batch
        SDL_GPUTextureSamplerBinding textureSamplerBinding{};
        textureSamplerBinding.texture = batch.texture;
        textureSamplerBinding.sampler = m_sampler;
        SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSamplerBinding, 1);

        // Draw this batch
        SDL_DrawGPUIndexedPrimitives(renderPass, batch.indexCount, 1, batch.startIndex, 0, 0);
    }

    SDL_EndGPURenderPass(renderPass);
}



void UIRenderer::generateRectVertices(
    const UIRenderCommand& cmd,
    SpriteVertex* vertices,
    float logicalWidth,
    float logicalHeight
) const {
    // Convert logical coordinates to NDC (-1 to 1)
    float x0 = (cmd.x / logicalWidth) * 2.0f - 1.0f;
    float y0 = 1.0f - (cmd.y / logicalHeight) * 2.0f;
    float x1 = ((cmd.x + cmd.width) / logicalWidth) * 2.0f - 1.0f;
    float y1 = 1.0f - ((cmd.y + cmd.height) / logicalHeight) * 2.0f;

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
