#pragma once

#include "UIElement.hpp"
#include "Primitives.hpp"

namespace ui {

// Alignment options
enum class HorizontalAlignment : std::uint8_t {
    Left,
    Center,
    Right,
    Stretch
};

enum class VerticalAlignment : std::uint8_t {
    Top,
    Center,
    Bottom,
    Stretch
};

// Padding/margin structure
struct Thickness {
    float left{0.0f};
    float top{0.0f};
    float right{0.0f};
    float bottom{0.0f};

    constexpr Thickness() noexcept = default;
    constexpr explicit Thickness(float uniform) noexcept
        : left(uniform), top(uniform), right(uniform), bottom(uniform) {}
    constexpr Thickness(float horizontal, float vertical) noexcept
        : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    constexpr Thickness(float l, float t, float r, float b) noexcept
        : left(l), top(t), right(r), bottom(b) {}

    [[nodiscard]] constexpr float horizontalSum() const noexcept { return left + right; }
    [[nodiscard]] constexpr float verticalSum() const noexcept { return top + bottom; }
};

// Base class for layout containers
class LayoutContainer : public UIElement {
public:
    explicit LayoutContainer(ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~LayoutContainer() override;

    LayoutContainer(LayoutContainer&&) noexcept;
    LayoutContainer& operator=(LayoutContainer&&) noexcept;

    // Padding inside the container
    [[nodiscard]] const Thickness& padding() const noexcept { return m_padding; }
    void setPadding(const Thickness& padding) noexcept;

    // Background colour
    [[nodiscard]] const Colour& backgroundColour() const noexcept { return m_backgroundColour; }
    void setBackgroundColour(const Colour& colour) noexcept { m_backgroundColour = colour; }

protected:
    [[nodiscard]] Rect getContentArea() const noexcept;

    Thickness m_padding{};
    Colour m_backgroundColour{Colour::transparent()};
};

// Stack panel - arranges children in a line
enum class Orientation : std::uint8_t {
    Horizontal,
    Vertical
};

class StackPanel : public LayoutContainer {
public:
    explicit StackPanel(Orientation orientation = Orientation::Vertical, ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~StackPanel() override;

    StackPanel(StackPanel&&) noexcept;
    StackPanel& operator=(StackPanel&&) noexcept;

    [[nodiscard]] Orientation orientation() const noexcept { return m_orientation; }
    void setOrientation(Orientation orientation) noexcept;

    [[nodiscard]] float spacing() const noexcept { return m_spacing; }
    void setSpacing(float spacing) noexcept;

    void measure(float availableWidth, float availableHeight) override;
    void arrange(const Rect& finalRect) override;

private:
    Orientation m_orientation{Orientation::Vertical};
    float m_spacing{0.0f};
};

// Grid panel - arranges children in rows and columns
struct GridDefinition {
    float size{0.0f};       // Absolute size, or 0 for auto
    float proportion{1.0f}; // For proportional sizing (like CSS fr units)
    bool isAuto{true};      // Auto-size to content
};

class GridPanel : public LayoutContainer {
public:
    explicit GridPanel(ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~GridPanel() override;

    GridPanel(GridPanel&&) noexcept;
    GridPanel& operator=(GridPanel&&) noexcept;

    void addRowDefinition(const GridDefinition& def);
    void addColumnDefinition(const GridDefinition& def);
    void clearDefinitions();

    // Set grid position for a child (call after adding child)
    static void setRow(UIElement* element, std::uint32_t row);
    static void setColumn(UIElement* element, std::uint32_t column);
    static void setRowSpan(UIElement* element, std::uint32_t span);
    static void setColumnSpan(UIElement* element, std::uint32_t span);

    void measure(float availableWidth, float availableHeight) override;
    void arrange(const Rect& finalRect) override;

private:
    std::vector<GridDefinition> m_rowDefinitions;
    std::vector<GridDefinition> m_columnDefinitions;
    std::vector<float> m_rowHeights;
    std::vector<float> m_columnWidths;

    void calculateRowHeights(float availableHeight);
    void calculateColumnWidths(float availableWidth);
};

// Attached properties for grid positioning (stored in element name as metadata for simplicity)
// In a full implementation, you'd use a proper attached property system
struct GridAttachedProperties {
    std::uint32_t row{0};
    std::uint32_t column{0};
    std::uint32_t rowSpan{1};
    std::uint32_t columnSpan{1};
};

} // namespace ui
