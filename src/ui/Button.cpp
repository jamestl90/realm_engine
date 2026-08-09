#include "../../include/ui/Button.hpp"
#include <algorithm>

namespace ui {

Button::Button(ElementID id) noexcept
    : StyledControl(id) {
    setFocusable(true);
}

Button::Button(std::string text, ElementID id) noexcept
    : StyledControl(id)
    , m_text(std::move(text)) {
    setFocusable(true);
}

Button::~Button() = default;

Button::Button(Button&& other) noexcept
    : StyledControl(std::move(other))
    , m_text(std::move(other.m_text))
    , m_onClick(std::move(other.m_onClick))
    , m_backgroundColour(other.m_backgroundColour)
    , m_textColour(other.m_textColour)
    , m_borderColour(other.m_borderColour)
    , m_hoverColour(other.m_hoverColour)
    , m_pressedColour(other.m_pressedColour)
    , m_borderThickness(other.m_borderThickness)
    , m_cornerRadius(other.m_cornerRadius)
    , m_fontSize(other.m_fontSize)
    , m_padding(other.m_padding) {
}

Button& Button::operator=(Button&& other) noexcept {
    if (this != &other) {
        StyledControl::operator=(std::move(other));
        m_text = std::move(other.m_text);
        m_onClick = std::move(other.m_onClick);
        m_backgroundColour = other.m_backgroundColour;
        m_textColour = other.m_textColour;
        m_borderColour = other.m_borderColour;
        m_hoverColour = other.m_hoverColour;
        m_pressedColour = other.m_pressedColour;
        m_borderThickness = other.m_borderThickness;
        m_cornerRadius = other.m_cornerRadius;
        m_fontSize = other.m_fontSize;
        m_padding = other.m_padding;
    }
    return *this;
}



void Button::setText(std::string text) {
    if (m_text != text) {
        m_text = std::move(text);
        invalidateLayout();
    }
}



Colour Button::currentBackgroundColour() const noexcept {
    if (!isEnabled()) {
        return Colour{60, 60, 60, 128};
    }

    if (isPressed()) {
        return m_pressedColour;
    }

    if (isHovered()) {

        return m_hoverColour;
    }
    return m_backgroundColour;
}



void Button::measure(float availableWidth, float availableHeight) {
    const TextMetrics textMetrics = measureText(m_text, m_fontSize);

    // Add padding
    m_measuredWidth = textMetrics.width + m_padding.horizontalSum();
    m_measuredHeight = textMetrics.height + m_padding.verticalSum();

    // Apply constraints
    const auto& constraints = sizeConstraints();

    m_measuredWidth = std::clamp(m_measuredWidth, constraints.min_width,
        std::min(constraints.max_width, availableWidth));

    m_measuredHeight = std::clamp(m_measuredHeight, constraints.min_height,
        std::min(constraints.max_height, availableHeight));

    // Ensure minimum clickable size
    m_measuredWidth = std::max(m_measuredWidth, 40.0f);
    m_measuredHeight = std::max(m_measuredHeight, 24.0f);
}



void Button::onMouseDown(MouseEventArgs& args) {
    FocusableControl::onMouseDown(args);
    setPressed(true);
    args.handled = true;
}

void Button::onMouseUp(MouseEventArgs& args) {
    // Check pressed state before base class clears it
    const bool wasPressed = isPressed();

    FocusableControl::onMouseUp(args);

    if (wasPressed && isHovered()) {
        if (m_onClick) {
            m_onClick();
        }
    }

    setPressed(false);
    args.handled = true;
}

void Button::onActivate() {

    if (m_onClick) {
        m_onClick();
    }
}

void Button::onVisualStateChanged(VisualState oldState, VisualState newState) {

    StyledControl::onVisualStateChanged(oldState, newState);

    // Could trigger animations here
}

} // namespace ui
