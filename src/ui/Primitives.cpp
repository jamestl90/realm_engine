#include "../../include/ui/Primitives.hpp"
#include <algorithm>

namespace ui {

// TextBlock implementation
TextBlock::TextBlock(ElementID id) noexcept
    : UIElement(id) {
}

TextBlock::TextBlock(std::string text, ElementID id) noexcept
    : UIElement(id)
    , m_text(std::move(text)) {
}

TextBlock::~TextBlock() = default;

TextBlock::TextBlock(TextBlock&& other) noexcept
    : UIElement(std::move(other))
    , m_text(std::move(other.m_text))
    , m_fontFamily(std::move(other.m_fontFamily))
    , m_colour(other.m_colour)
    , m_fontSize(other.m_fontSize)
    , m_alignment(other.m_alignment) {
}

TextBlock& TextBlock::operator=(TextBlock&& other) noexcept {
    if (this != &other) {
        UIElement::operator=(std::move(other));
        m_text = std::move(other.m_text);
        m_fontFamily = std::move(other.m_fontFamily);
        m_colour = other.m_colour;
        m_fontSize = other.m_fontSize;
        m_alignment = other.m_alignment;
    }
    return *this;
}

void TextBlock::setText(std::string text) {
    if (m_text != text) {
        m_text = std::move(text);
        invalidateLayout();
    }
}

void TextBlock::measure(float availableWidth, float availableHeight) {
    const TextMetrics textMetrics = measureText(m_text, m_fontSize);

    const auto& constraints = sizeConstraints();

    m_measuredWidth = std::clamp(
        constraints.preferred_width > 0 ? constraints.preferred_width : textMetrics.width,
        constraints.min_width,
        std::min(constraints.max_width, availableWidth)
    );

    m_measuredHeight = std::clamp(
        constraints.preferred_height > 0 ? constraints.preferred_height : textMetrics.height,
        constraints.min_height,
        std::min(constraints.max_height, availableHeight)
    );
}

// Image implementation
Image::Image(ElementID id) noexcept
    : UIElement(id) {
}

Image::~Image() = default;

Image::Image(Image&& other) noexcept
    : UIElement(std::move(other))
    , m_textureId(other.m_textureId)
    , m_regionName(std::move(other.m_regionName))
    , m_tint(other.m_tint)
    , m_stretch(other.m_stretch)
    , m_sourceWidth(other.m_sourceWidth)
    , m_sourceHeight(other.m_sourceHeight) {
    other.m_textureId = rendering::INVALID_TEXTURE_ID;
}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        UIElement::operator=(std::move(other));
        m_textureId = other.m_textureId;
        m_regionName = std::move(other.m_regionName);
        m_tint = other.m_tint;
        m_stretch = other.m_stretch;
        m_sourceWidth = other.m_sourceWidth;
        m_sourceHeight = other.m_sourceHeight;

        other.m_textureId = rendering::INVALID_TEXTURE_ID;
    }
    return *this;
}

void Image::setSourceSize(float width, float height) noexcept {
    m_sourceWidth = width;
    m_sourceHeight = height;
    invalidateLayout();
}

void Image::measure(float availableWidth, float availableHeight) {
    const auto& constraints = sizeConstraints();

    float desiredWidth = m_sourceWidth;
    float desiredHeight = m_sourceHeight;

    switch (m_stretch) {
        case Stretch::None:
            // Use source size
            break;

        case Stretch::Fill:
            desiredWidth = availableWidth;
            desiredHeight = availableHeight;
            break;

        case Stretch::Uniform:
            if (m_sourceWidth > 0 && m_sourceHeight > 0) {
                float scaleX = availableWidth / m_sourceWidth;
                float scaleY = availableHeight / m_sourceHeight;
                float scale = std::min(scaleX, scaleY);
                desiredWidth = m_sourceWidth * scale;
                desiredHeight = m_sourceHeight * scale;
            }
            break;

        case Stretch::UniformToFill:
            if (m_sourceWidth > 0 && m_sourceHeight > 0) {
                float scaleX = availableWidth / m_sourceWidth;
                float scaleY = availableHeight / m_sourceHeight;
                float scale = std::max(scaleX, scaleY);
                desiredWidth = m_sourceWidth * scale;
                desiredHeight = m_sourceHeight * scale;
            }
            break;
    }

    m_measuredWidth = std::clamp(
        constraints.preferred_width > 0 ? constraints.preferred_width : desiredWidth,
        constraints.min_width,
        std::min(constraints.max_width, availableWidth)
    );

    m_measuredHeight = std::clamp(
        constraints.preferred_height > 0 ? constraints.preferred_height : desiredHeight,
        constraints.min_height,
        std::min(constraints.max_height, availableHeight)
    );
}

// Rectangle implementation
Rectangle::Rectangle(ElementID id) noexcept
    : UIElement(id) {
}

Rectangle::~Rectangle() = default;

Rectangle::Rectangle(Rectangle&& other) noexcept
    : UIElement(std::move(other))
    , m_fill(other.m_fill)
    , m_borderColour(other.m_borderColour)
    , m_borderThickness(other.m_borderThickness)
    , m_cornerRadius(other.m_cornerRadius) {
}

Rectangle& Rectangle::operator=(Rectangle&& other) noexcept {
    if (this != &other) {
        UIElement::operator=(std::move(other));
        m_fill = other.m_fill;
        m_borderColour = other.m_borderColour;
        m_borderThickness = other.m_borderThickness;
        m_cornerRadius = other.m_cornerRadius;
    }
    return *this;
}

} // namespace ui
