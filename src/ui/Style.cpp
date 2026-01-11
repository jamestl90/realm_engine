#include "../../include/ui/Style.hpp"

namespace ui {

void Style::setProperty(StyleProperty property, const StyleValue& value, VisualState state) {
    m_properties[state][property] = value;
}

const StyleValue* Style::getProperty(StyleProperty property, VisualState state) const {
    // Try to find in requested state
    auto stateIt = m_properties.find(state);
    if (stateIt != m_properties.end()) {
        auto propIt = stateIt->second.find(property);
        if (propIt != stateIt->second.end()) {
            return &propIt->second;
        }
    }

    // Fall back to Normal state if not found
    if (state != VisualState::Normal) {
        stateIt = m_properties.find(VisualState::Normal);
        if (stateIt != m_properties.end()) {
            auto propIt = stateIt->second.find(property);
            if (propIt != stateIt->second.end()) {
                return &propIt->second;
            }
        }
    }

    return nullptr;
}

// StyledControl implementation
StyledControl::StyledControl(ElementID id) noexcept
    : FocusableControl(id) {
}

StyledControl::~StyledControl() = default;

StyledControl::StyledControl(StyledControl&& other) noexcept
    : FocusableControl(std::move(other))
    , m_style(std::move(other.m_style))
    , m_margin(other.m_margin)
    , m_lastVisualState(other.m_lastVisualState) {
}

StyledControl& StyledControl::operator=(StyledControl&& other) noexcept {
    if (this != &other) {
        FocusableControl::operator=(std::move(other));
        m_style = std::move(other.m_style);
        m_margin = other.m_margin;
        m_lastVisualState = other.m_lastVisualState;
    }
    return *this;
}

VisualState StyledControl::visualState() const noexcept {
    if (!isEnabled()) {
        return VisualState::Disabled;
    }
    if (isPressed()) {
        return VisualState::Pressed;
    }
    if (isFocused()) {
        return VisualState::Focused;
    }
    if (isHovered()) {
        return VisualState::Hovered;
    }
    return VisualState::Normal;
}

void StyledControl::update(float dt) {
    FocusableControl::update(dt);

    VisualState currentState = visualState();
    if (currentState != m_lastVisualState) {
        onVisualStateChanged(m_lastVisualState, currentState);
        m_lastVisualState = currentState;
    }
}

void StyledControl::onVisualStateChanged(VisualState oldState, VisualState newState) {
    (void)oldState;
    (void)newState;
    // Override in derived classes for custom state change handling
}

// Default styles
std::shared_ptr<Style> DefaultStyles::button() {
    auto style = std::make_shared<Style>();

    // Normal state
    style->setProperty(StyleProperty::BackgroundColour, Colour{60, 60, 60, 255}, VisualState::Normal);
    style->setProperty(StyleProperty::ForegroundColour, Colour::white(), VisualState::Normal);
    style->setProperty(StyleProperty::BorderColour, Colour{100, 100, 100, 255}, VisualState::Normal);
    style->setProperty(StyleProperty::BorderThickness, 1.0f, VisualState::Normal);
    style->setProperty(StyleProperty::CornerRadius, 4.0f, VisualState::Normal);
    style->setProperty(StyleProperty::Padding, Thickness{12.0f, 8.0f}, VisualState::Normal);

    // Hovered state
    style->setProperty(StyleProperty::BackgroundColour, Colour{80, 80, 80, 255}, VisualState::Hovered);
    style->setProperty(StyleProperty::BorderColour, Colour{120, 120, 120, 255}, VisualState::Hovered);

    // Pressed state
    style->setProperty(StyleProperty::BackgroundColour, Colour{40, 40, 40, 255}, VisualState::Pressed);

    // Focused state
    style->setProperty(StyleProperty::BorderColour, Colour{100, 149, 237, 255}, VisualState::Focused);
    style->setProperty(StyleProperty::BorderThickness, 2.0f, VisualState::Focused);

    // Disabled state
    style->setProperty(StyleProperty::BackgroundColour, Colour{45, 45, 45, 255}, VisualState::Disabled);
    style->setProperty(StyleProperty::ForegroundColour, Colour{128, 128, 128, 255}, VisualState::Disabled);

    return style;
}

std::shared_ptr<Style> DefaultStyles::textBox() {
    auto style = std::make_shared<Style>();

    // Normal state
    style->setProperty(StyleProperty::BackgroundColour, Colour{30, 30, 30, 255}, VisualState::Normal);
    style->setProperty(StyleProperty::ForegroundColour, Colour::white(), VisualState::Normal);
    style->setProperty(StyleProperty::BorderColour, Colour{80, 80, 80, 255}, VisualState::Normal);
    style->setProperty(StyleProperty::BorderThickness, 1.0f, VisualState::Normal);
    style->setProperty(StyleProperty::Padding, Thickness{8.0f, 6.0f}, VisualState::Normal);

    // Focused state
    style->setProperty(StyleProperty::BorderColour, Colour{100, 149, 237, 255}, VisualState::Focused);
    style->setProperty(StyleProperty::BorderThickness, 2.0f, VisualState::Focused);

    // Disabled state
    style->setProperty(StyleProperty::BackgroundColour, Colour{25, 25, 25, 255}, VisualState::Disabled);
    style->setProperty(StyleProperty::ForegroundColour, Colour{100, 100, 100, 255}, VisualState::Disabled);

    return style;
}

std::shared_ptr<Style> DefaultStyles::panel() {
    auto style = std::make_shared<Style>();

    style->setProperty(StyleProperty::BackgroundColour, Colour{45, 45, 45, 200}, VisualState::Normal);
    style->setProperty(StyleProperty::BorderColour, Colour{70, 70, 70, 255}, VisualState::Normal);
    style->setProperty(StyleProperty::BorderThickness, 1.0f, VisualState::Normal);
    style->setProperty(StyleProperty::CornerRadius, 6.0f, VisualState::Normal);
    style->setProperty(StyleProperty::Padding, Thickness{16.0f}, VisualState::Normal);

    return style;
}

} // namespace ui
