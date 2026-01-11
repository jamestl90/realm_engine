#pragma once

#include "FocusableControl.hpp"
#include "../rendering/TextureManager.hpp"
#include <string>

namespace ui {

// Text alignment within a text element
enum class TextAlignment : std::uint8_t {
    Left,
    Center,
    Right
};

// Colour structure for UI
struct Colour {
    std::uint8_t r{255};
    std::uint8_t g{255};
    std::uint8_t b{255};
    std::uint8_t a{255};

    constexpr Colour() noexcept = default;
    constexpr Colour(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255) noexcept
        : r(red), g(green), b(blue), a(alpha) {}

    static constexpr Colour white() noexcept { return Colour{255, 255, 255, 255}; }
    static constexpr Colour black() noexcept { return Colour{0, 0, 0, 255}; }
    static constexpr Colour transparent() noexcept { return Colour{0, 0, 0, 0}; }
};

// Text primitive - displays text
class TextBlock : public UIElement {
public:
    explicit TextBlock(ElementID id = INVALID_ELEMENT_ID) noexcept;
    explicit TextBlock(std::string text, ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~TextBlock() override;

    TextBlock(TextBlock&&) noexcept;
    TextBlock& operator=(TextBlock&&) noexcept;

    // Text content
    [[nodiscard]] const std::string& text() const noexcept { return m_text; }
    void setText(std::string text);

    // Appearance
    [[nodiscard]] const Colour& colour() const noexcept { return m_colour; }
    void setColour(const Colour& colour) noexcept { m_colour = colour; }

    [[nodiscard]] float fontSize() const noexcept { return m_fontSize; }
    void setFontSize(float size) noexcept { m_fontSize = size; invalidateLayout(); }

    [[nodiscard]] TextAlignment alignment() const noexcept { return m_alignment; }
    void setAlignment(TextAlignment alignment) noexcept { m_alignment = alignment; }

    // Font (font ID or name - actual font loading handled by renderer)
    [[nodiscard]] const std::string& fontFamily() const noexcept { return m_fontFamily; }
    void setFontFamily(std::string family) { m_fontFamily = std::move(family); invalidateLayout(); }

    void measure(float availableWidth, float availableHeight) override;

private:
    std::string m_text;
    std::string m_fontFamily{"default"};
    Colour m_colour{Colour::white()};
    float m_fontSize{16.0f};
    TextAlignment m_alignment{TextAlignment::Left};
};

// Image primitive - displays a texture or texture region
class Image : public UIElement {
public:
    explicit Image(ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~Image() override;

    Image(Image&&) noexcept;
    Image& operator=(Image&&) noexcept;

    // Texture source
    [[nodiscard]] rendering::TextureID textureId() const noexcept { return m_textureId; }
    void setTextureId(rendering::TextureID id) noexcept { m_textureId = id; invalidateLayout(); }

    [[nodiscard]] const std::string& regionName() const noexcept { return m_regionName; }
    void setRegionName(std::string name) { m_regionName = std::move(name); invalidateLayout(); }

    // Tint colour
    [[nodiscard]] const Colour& tint() const noexcept { return m_tint; }
    void setTint(const Colour& tint) noexcept { m_tint = tint; }

    // Stretch mode
    enum class Stretch : std::uint8_t {
        None,           // Original size
        Fill,           // Fill bounds, may distort
        Uniform,        // Maintain aspect ratio, fit within bounds
        UniformToFill   // Maintain aspect ratio, fill bounds (may crop)
    };

    [[nodiscard]] Stretch stretch() const noexcept { return m_stretch; }
    void setStretch(Stretch stretch) noexcept { m_stretch = stretch; invalidateLayout(); }

    // Source dimensions (set when texture is loaded)
    void setSourceSize(float width, float height) noexcept;
    [[nodiscard]] float sourceWidth() const noexcept { return m_sourceWidth; }
    [[nodiscard]] float sourceHeight() const noexcept { return m_sourceHeight; }

    void measure(float availableWidth, float availableHeight) override;

private:
    rendering::TextureID m_textureId{rendering::INVALID_TEXTURE_ID};
    std::string m_regionName;
    Colour m_tint{Colour::white()};
    Stretch m_stretch{Stretch::Uniform};
    float m_sourceWidth{0.0f};
    float m_sourceHeight{0.0f};
};

// Rectangle primitive - solid colour or border
class Rectangle : public UIElement {
public:
    explicit Rectangle(ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~Rectangle() override;

    Rectangle(Rectangle&&) noexcept;
    Rectangle& operator=(Rectangle&&) noexcept;

    // Fill
    [[nodiscard]] const Colour& fill() const noexcept { return m_fill; }
    void setFill(const Colour& fill) noexcept { m_fill = fill; }

    // Border
    [[nodiscard]] const Colour& borderColour() const noexcept { return m_borderColour; }
    void setBorderColour(const Colour& colour) noexcept { m_borderColour = colour; }

    [[nodiscard]] float borderThickness() const noexcept { return m_borderThickness; }
    void setBorderThickness(float thickness) noexcept { m_borderThickness = thickness; }

    // Corner radius for rounded rectangles
    [[nodiscard]] float cornerRadius() const noexcept { return m_cornerRadius; }
    void setCornerRadius(float radius) noexcept { m_cornerRadius = radius; }

private:
    Colour m_fill{Colour::white()};
    Colour m_borderColour{Colour::transparent()};
    float m_borderThickness{0.0f};
    float m_cornerRadius{0.0f};
};

} // namespace ui
