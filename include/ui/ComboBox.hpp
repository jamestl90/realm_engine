#pragma once

#include "Style.hpp"
#include "Primitives.hpp"
#include "Layout.hpp"
#include <functional>
#include <string>
#include <vector>

namespace ui {

// ComboBox control - dropdown selection control with a list of items
class ComboBox : public StyledControl {
public:
    explicit ComboBox(ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~ComboBox() override;

    ComboBox(ComboBox&&) noexcept;
    ComboBox& operator=(ComboBox&&) noexcept;

    // Items management
    void addItem(std::string item);
    void clearItems();
    [[nodiscard]] const std::vector<std::string>& items() const noexcept { return m_items; }

    // Selected item
    [[nodiscard]] int selectedIndex() const noexcept { return m_selectedIndex; }
    void setSelectedIndex(int index);
    [[nodiscard]] const std::string& selectedItem() const noexcept;

    // Placeholder text (shown when no item is selected)
    [[nodiscard]] const std::string& placeholder() const noexcept { return m_placeholder; }
    void setPlaceholder(std::string text);

    // Selection callback
    using SelectionCallback = std::function<void(const std::string&)>;
    void setOnSelectionChanged(SelectionCallback callback) { m_onSelectionChanged = std::move(callback); }

    // Appearance
    [[nodiscard]] const Colour& backgroundColour() const noexcept { return m_backgroundColour; }
    void setBackgroundColour(const Colour& colour) noexcept;

    [[nodiscard]] const Colour& textColour() const noexcept { return m_textColour; }
    void setTextColour(const Colour& colour) noexcept { m_textColour = colour; }

    [[nodiscard]] const Colour& borderColour() const noexcept { return m_borderColour; }
    void setBorderColour(const Colour& colour) noexcept { m_borderColour = colour; }

    [[nodiscard]] const Colour& hoverColour() const noexcept { return m_hoverColour; }
    void setHoverColour(const Colour& colour) noexcept { m_hoverColour = colour; }

    [[nodiscard]] const Colour& dropdownBackgroundColour() const noexcept { return m_dropdownBackgroundColour; }
    void setDropdownBackgroundColour(const Colour& colour) noexcept { m_dropdownBackgroundColour = colour; }

    [[nodiscard]] const Colour& itemHoverColour() const noexcept { return m_itemHoverColour; }
    void setItemHoverColour(const Colour& colour) noexcept { m_itemHoverColour = colour; }

    [[nodiscard]] float borderThickness() const noexcept { return m_borderThickness; }
    void setBorderThickness(float thickness) noexcept { m_borderThickness = thickness; }

    [[nodiscard]] float fontSize() const noexcept { return m_fontSize; }
    void setFontSize(float size) noexcept;

    [[nodiscard]] const Thickness& padding() const noexcept { return m_padding; }
    void setPadding(const Thickness& padding) noexcept;

    // Dropdown state
    [[nodiscard]] bool isOpen() const noexcept { return m_isOpen; }
    void open();
    void close();

    // Hover state for rendering
    [[nodiscard]] int hoveredItemIndex() const noexcept { return m_hoveredItemIndex; }

    // Layout
    void measure(float availableWidth, float availableHeight) override;
    void arrange(const Rect& finalRect) override;
    void update(float dt) override;

    // Hit testing - extend to include dropdown when open
    [[nodiscard]] bool hitTest(float x, float y) const override;
    [[nodiscard]] UIElement* hitTestRecursive(float x, float y);

protected:
    void onMouseDown(MouseEventArgs& args) override;
    void onMouseMove(MouseEventArgs& args) override;
    void onMouseLeave() override;
    void onActivate() override;

private:
    void rebuildDropdown();
    void selectItem(int index);
    void updateHeaderText();

    // Items
    std::vector<std::string> m_items;
    int m_selectedIndex{-1};
    std::string m_placeholder{"Select an item..."};

    // Callbacks
    SelectionCallback m_onSelectionChanged;

    // Visual state
    bool m_isOpen{false};
    bool m_headerHovered{false};

    // Appearance
    Colour m_backgroundColour{240, 240, 240, 255};
    Colour m_textColour{0, 0, 0, 255};
    Colour m_borderColour{180, 180, 180, 255};
    Colour m_hoverColour{230, 230, 230, 255};
    Colour m_dropdownBackgroundColour{250, 250, 250, 255};
    Colour m_itemHoverColour{220, 220, 220, 255};
    float m_borderThickness{1.0f};
    float m_fontSize{14.0f};
    Thickness m_padding{8.0f, 6.0f, 8.0f, 6.0f};

    // Internal elements (visual representation)
    // We'll render these ourselves rather than using actual child elements
    // to have more control over the dropdown behavior
    float m_headerHeight{0.0f};
    float m_dropdownHeight{0.0f};
    float m_itemHeight{24.0f};
    int m_hoveredItemIndex{-1};
};

} // namespace ui
