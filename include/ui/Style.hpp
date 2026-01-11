#pragma once

#include "FocusableControl.hpp"
#include "Layout.hpp"
#include "Primitives.hpp"
#include <optional>
#include <unordered_map>
#include <variant>

namespace ui {

// Visual states for controls
enum class VisualState : std::uint8_t {
    Normal,
    Hovered,
    Pressed,
    Focused,
    Disabled
};

// Style property keys
enum class StyleProperty : std::uint32_t {
    // Colours
    BackgroundColour,
    ForegroundColour,
    BorderColour,

    // Dimensions
    BorderThickness,
    CornerRadius,
    FontSize,
    Padding,
    Margin,

    // Alignment
    HorizontalContentAlignment,
    VerticalContentAlignment,
    TextAlignment,

    // Font
    FontFamily
};

} // namespace ui

// Hash specialisations for enum keys - must be defined before use in unordered_map
template<>
struct std::hash<ui::VisualState> {
    std::size_t operator()(ui::VisualState state) const noexcept {
        return static_cast<std::size_t>(state);
    }
};

template<>
struct std::hash<ui::StyleProperty> {
    std::size_t operator()(ui::StyleProperty prop) const noexcept {
        return static_cast<std::size_t>(prop);
    }
};

namespace ui {

// Style property types
using StyleValue = std::variant<
    float,
    Colour,
    std::string,
    Thickness,
    HorizontalAlignment,
    VerticalAlignment,
    TextAlignment
>;

// Style definition - maps properties to values for each visual state
class Style {
public:
    Style() = default;
    ~Style() = default;

    Style(const Style&) = default;
    Style& operator=(const Style&) = default;
    Style(Style&&) noexcept = default;
    Style& operator=(Style&&) noexcept = default;

    // Set property for a specific state
    void setProperty(StyleProperty property, const StyleValue& value, VisualState state = VisualState::Normal);

    // Get property for a specific state (falls back to Normal if not found)
    [[nodiscard]] const StyleValue* getProperty(StyleProperty property, VisualState state = VisualState::Normal) const;

    // Convenience getters with type safety
    template<typename T>
    [[nodiscard]] std::optional<T> get(StyleProperty property, VisualState state = VisualState::Normal) const {
        const StyleValue* value = getProperty(property, state);
        if (value) {
            if (auto* ptr = std::get_if<T>(value)) {
                return *ptr;
            }
        }
        return std::nullopt;
    }

private:
    // Map of state -> (property -> value)
    std::unordered_map<VisualState, std::unordered_map<StyleProperty, StyleValue>> m_properties;
};

// Styled control base - applies styles based on visual state
class StyledControl : public FocusableControl {
public:
    explicit StyledControl(ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~StyledControl() override;

    StyledControl(StyledControl&&) noexcept;
    StyledControl& operator=(StyledControl&&) noexcept;

    // Style
    void setStyle(std::shared_ptr<Style> style) { m_style = std::move(style); }
    [[nodiscard]] const std::shared_ptr<Style>& style() const noexcept { return m_style; }

    // Current visual state
    [[nodiscard]] VisualState visualState() const noexcept;

    // Margin (space outside the element)
    [[nodiscard]] const Thickness& margin() const noexcept { return m_margin; }
    void setMargin(const Thickness& margin) noexcept { m_margin = margin; invalidateLayout(); }

    void update(float dt) override;

protected:
    // Get style property with fallback
    template<typename T>
    [[nodiscard]] T getStyleProperty(StyleProperty property, const T& defaultValue) const {
        if (m_style) {
            auto value = m_style->get<T>(property, visualState());
            if (value) {
                return *value;
            }
        }
        return defaultValue;
    }

    // Called when visual state changes
    virtual void onVisualStateChanged(VisualState oldState, VisualState newState);

private:
    std::shared_ptr<Style> m_style;
    Thickness m_margin{};
    VisualState m_lastVisualState{VisualState::Normal};
};

// Default styles factory
class DefaultStyles {
public:
    [[nodiscard]] static std::shared_ptr<Style> button();
    [[nodiscard]] static std::shared_ptr<Style> textBox();
    [[nodiscard]] static std::shared_ptr<Style> panel();
};

} // namespace ui
