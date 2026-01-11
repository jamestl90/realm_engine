#pragma once

#include "Style.hpp"
#include "Primitives.hpp"
#include <functional>
#include <string>

namespace ui {

// Button control - clickable button with text label
class Button : public StyledControl {
public:

    explicit Button(ElementID id = INVALID_ELEMENT_ID) noexcept;
    explicit Button(std::string text, ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~Button() override;
    Button(Button&&) noexcept;
    Button& operator=(Button&&) noexcept;

    // Text content
    [[nodiscard]] const std::string& text() const noexcept { return m_text; }
    void setText(std::string text);

    // Click callback
    using ClickCallback = std::function<void()>;
    void setOnClick(ClickCallback callback) { m_onClick = std::move(callback); }

    // Appearance
    [[nodiscard]] const Colour& backgroundColour() const noexcept { return m_backgroundColour; }
    void setBackgroundColour(const Colour& colour) noexcept { m_backgroundColour = colour; }

    [[nodiscard]] const Colour& textColour() const noexcept { return m_textColour; }
    void setTextColour(const Colour& colour) noexcept { m_textColour = colour; }

    [[nodiscard]] const Colour& borderColour() const noexcept { return m_borderColour; }
    void setBorderColour(const Colour& colour) noexcept { m_borderColour = colour; }

    [[nodiscard]] const Colour& hoverColour() const noexcept { return m_hoverColour; }
    void setHoverColour(const Colour& colour) noexcept { m_hoverColour = colour; }

    [[nodiscard]] const Colour& pressedColour() const noexcept { return m_pressedColour; }
    void setPressedColour(const Colour& colour) noexcept { m_pressedColour = colour; }

    [[nodiscard]] float borderThickness() const noexcept { return m_borderThickness; }
    void setBorderThickness(float thickness) noexcept { m_borderThickness = thickness; }

    [[nodiscard]] float cornerRadius() const noexcept { return m_cornerRadius; }
    void setCornerRadius(float radius) noexcept { m_cornerRadius = radius; }

    [[nodiscard]] float fontSize() const noexcept { return m_fontSize; }
    void setFontSize(float size) noexcept { m_fontSize = size; invalidateLayout(); }

    [[nodiscard]] const Thickness& padding() const noexcept { return m_padding; }
    void setPadding(const Thickness& padding) noexcept { m_padding = padding; invalidateLayout(); }

    // Get current background colour based on state
    [[nodiscard]] Colour currentBackgroundColour() const noexcept;

    // Layout
    void measure(float availableWidth, float availableHeight) override;

protected:
    void onMouseDown(MouseEventArgs& args) override;
    void onMouseUp(MouseEventArgs& args) override;
    void onActivate() override;
    void onVisualStateChanged(VisualState oldState, VisualState newState) override;

private:

    std::string m_text;
    ClickCallback m_onClick;

    // Appearance
    Colour m_backgroundColour{80, 80, 80, 255};
    Colour m_textColour{255, 255, 255, 255};
    Colour m_borderColour{120, 120, 120, 255};
    Colour m_hoverColour{100, 100, 100, 255};
    Colour m_pressedColour{60, 60, 60, 255};
    float m_borderThickness{1.0f};
    float m_cornerRadius{4.0f};
    float m_fontSize{14.0f};
    Thickness m_padding{12.0f, 8.0f, 12.0f, 8.0f};
};

} // namespace ui
