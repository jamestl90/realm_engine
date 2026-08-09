#include "../../include/ui/ComboBox.hpp"
#include <SDL3/SDL.h>
#include <algorithm>

namespace ui {

ComboBox::ComboBox(ElementID id) noexcept
    : StyledControl(id) {
    setFocusable(true);
}

ComboBox::~ComboBox() = default;

ComboBox::ComboBox(ComboBox&& other) noexcept
    : StyledControl(std::move(other))
    , m_items(std::move(other.m_items))
    , m_selectedIndex(other.m_selectedIndex)
    , m_placeholder(std::move(other.m_placeholder))
    , m_onSelectionChanged(std::move(other.m_onSelectionChanged))
    , m_isOpen(other.m_isOpen)
    , m_headerHovered(other.m_headerHovered)
    , m_backgroundColour(other.m_backgroundColour)
    , m_textColour(other.m_textColour)
    , m_borderColour(other.m_borderColour)
    , m_hoverColour(other.m_hoverColour)
    , m_dropdownBackgroundColour(other.m_dropdownBackgroundColour)
    , m_itemHoverColour(other.m_itemHoverColour)
    , m_borderThickness(other.m_borderThickness)
    , m_fontSize(other.m_fontSize)
    , m_padding(other.m_padding)
    , m_headerHeight(other.m_headerHeight)
    , m_dropdownHeight(other.m_dropdownHeight)
    , m_itemHeight(other.m_itemHeight)
    , m_hoveredItemIndex(other.m_hoveredItemIndex) {
}

ComboBox& ComboBox::operator=(ComboBox&& other) noexcept {
    if (this != &other) {
        StyledControl::operator=(std::move(other));
        m_items = std::move(other.m_items);
        m_selectedIndex = other.m_selectedIndex;
        m_placeholder = std::move(other.m_placeholder);
        m_onSelectionChanged = std::move(other.m_onSelectionChanged);
        m_isOpen = other.m_isOpen;
        m_headerHovered = other.m_headerHovered;
        m_backgroundColour = other.m_backgroundColour;
        m_textColour = other.m_textColour;
        m_borderColour = other.m_borderColour;
        m_hoverColour = other.m_hoverColour;
        m_dropdownBackgroundColour = other.m_dropdownBackgroundColour;
        m_itemHoverColour = other.m_itemHoverColour;
        m_borderThickness = other.m_borderThickness;
        m_fontSize = other.m_fontSize;
        m_padding = other.m_padding;
        m_headerHeight = other.m_headerHeight;
        m_dropdownHeight = other.m_dropdownHeight;
        m_itemHeight = other.m_itemHeight;
        m_hoveredItemIndex = other.m_hoveredItemIndex;
    }
    return *this;
}

void ComboBox::addItem(std::string item) {
    m_items.push_back(std::move(item));
    invalidateLayout();
}

void ComboBox::clearItems() {
    m_items.clear();
    m_selectedIndex = -1;
    m_isOpen = false;
    invalidateLayout();
}

void ComboBox::setSelectedIndex(int index) {
    if (index >= -1 && index < static_cast<int>(m_items.size())) {
        if (m_selectedIndex != index) {
            selectItem(index);
        }
    }
}

const std::string& ComboBox::selectedItem() const noexcept {
    static const std::string empty;
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
        return m_items[m_selectedIndex];
    }
    return empty;
}

void ComboBox::setPlaceholder(std::string text) {
    m_placeholder = std::move(text);
    invalidateLayout();
}

void ComboBox::setBackgroundColour(const Colour& colour) noexcept {
    m_backgroundColour = colour;
}

void ComboBox::setFontSize(float size) noexcept {
    m_fontSize = size;
    m_itemHeight = size + 12.0f; // Font size + some padding
    invalidateLayout();
}

void ComboBox::setPadding(const Thickness& padding) noexcept {
    m_padding = padding;
    invalidateLayout();
}

void ComboBox::open() {
    if (!m_isOpen && !m_items.empty()) {
        //SDL_Log("ComboBox::open() - opening dropdown");
        m_isOpen = true;
        invalidateLayout();
        //SDL_Log("ComboBox bounds before layout: x=%.2f, y=%.2f, w=%.2f, h=%.2f",
        //        m_bounds.x, m_bounds.y, m_bounds.width, m_bounds.height);
    }
}

void ComboBox::close() {
    if (m_isOpen) {
        //SDL_Log("ComboBox::close() - closing dropdown");
        m_isOpen = false;
        m_hoveredItemIndex = -1;
        invalidateLayout();
    }
}

void ComboBox::measure(float availableWidth, float availableHeight) {
    // Find longest text (selected item or placeholder)
    std::string displayText = m_placeholder;
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size())) {
        displayText = m_items[m_selectedIndex];
    }

    // Also consider all items for width calculation
    TextMetrics textMetrics = measureText(displayText, m_fontSize);
    float maxTextWidth = textMetrics.width;
    for (const auto& item : m_items) {
        const TextMetrics itemMetrics = measureText(item, m_fontSize);
        maxTextWidth = std::max(maxTextWidth, itemMetrics.width);
        textMetrics.height = std::max(textMetrics.height, itemMetrics.height);
    }

    // Add padding and arrow space
    const float arrowWidth = 20.0f;
    m_headerHeight = textMetrics.height + m_padding.verticalSum();
    m_itemHeight = textMetrics.height + 12.0f;

    m_measuredWidth = maxTextWidth + m_padding.horizontalSum() + arrowWidth;
    m_measuredHeight = m_headerHeight;

    // If open, add dropdown height
    if (m_isOpen) {
        m_dropdownHeight = m_itemHeight * static_cast<float>(m_items.size());
        m_measuredHeight += m_dropdownHeight;
    }

    // Apply constraints
    const auto& constraints = sizeConstraints();

    m_measuredWidth = std::clamp(m_measuredWidth, constraints.min_width,
        std::min(constraints.max_width, availableWidth));

    m_measuredHeight = std::clamp(m_measuredHeight, constraints.min_height,
        std::min(constraints.max_height, availableHeight));

    // Ensure minimum size
    m_measuredWidth = std::max(m_measuredWidth, 100.0f);
    m_headerHeight = std::max(m_headerHeight, 24.0f);
}

void ComboBox::arrange(const Rect& finalRect) {
    setBounds(finalRect);
    setLocalBounds(Rect{0.0f, 0.0f, finalRect.width, finalRect.height});
    markLayoutClean();
    //SDL_Log("ComboBox::arrange - bounds updated: x=%.2f, y=%.2f, w=%.2f, h=%.2f, isOpen=%d",
    //        finalRect.x, finalRect.y, finalRect.width, finalRect.height, m_isOpen);
}

void ComboBox::update(float dt) {
    StyledControl::update(dt);

    // Note: We can't handle "click outside to close" here without access to UIManager
    // The UIManager would need to handle this, or we'd need a way to detect
    // when the mouse is clicked outside our bounds
}

bool ComboBox::hitTest(float x, float y) const {
    if (!isVisible() || !isEnabled()) {
        return false;
    }

    // Calculate header height
    float headerHeight = m_fontSize + m_padding.verticalSum();
    headerHeight = std::max(headerHeight, 24.0f);

    //SDL_Log("ComboBox::hitTest - x=%.2f, y=%.2f, bounds(%.2f,%.2f,%.2f,%.2f), headerHeight=%.2f, isOpen=%d",
    //        x, y, m_bounds.x, m_bounds.y, m_bounds.width, m_bounds.height, headerHeight, m_isOpen);

    // Check header bounds only (not full m_bounds which might include dropdown)
    if (y >= m_bounds.y && y < m_bounds.y + headerHeight &&
        x >= m_bounds.x && x < m_bounds.x + m_bounds.width) {
        //SDL_Log("ComboBox::hitTest - header hit");
        return true;
    }

    // If dropdown is open, extend hit test to include dropdown area
    if (m_isOpen && !m_items.empty()) {
        float dropdownHeight = m_itemHeight * static_cast<float>(m_items.size());
        Rect dropdownBounds{
            m_bounds.x,
            m_bounds.y + headerHeight,
            m_bounds.width,
            dropdownHeight
        };

        bool hit = dropdownBounds.contains(x, y);
        //SDL_Log("ComboBox::hitTest - checking dropdown bounds(%.2f,%.2f,%.2f,%.2f), itemHeight=%.2f, hit=%d",
        //        dropdownBounds.x, dropdownBounds.y, dropdownBounds.width, dropdownBounds.height,
        //        m_itemHeight, hit);
        return hit;
    }

    return false;
}

UIElement* ComboBox::hitTestRecursive(float x, float y) {
    // Override hitTestRecursive to bypass parent bounds checking
    // This allows the dropdown to receive events even when it extends
    // outside the parent's bounds

    //SDL_Log("ComboBox::hitTestRecursive - checking x=%.2f, y=%.2f", x, y);

    if (hitTest(x, y)) {
        //SDL_Log("ComboBox::hitTestRecursive - HIT! Returning this ComboBox");
        return this;
    }

    //SDL_Log("ComboBox::hitTestRecursive - no hit");
    return nullptr;
}

void ComboBox::onMouseDown(MouseEventArgs& args) {
    StyledControl::onMouseDown(args);

    // Calculate header height (same calculation as in measure())
    float headerHeight = m_fontSize + m_padding.verticalSum();
    headerHeight = std::max(headerHeight, 24.0f);

    // Convert to local coordinates
    const float localX = args.x - m_bounds.x;
    const float localY = args.y - m_bounds.y;

    //SDL_Log("ComboBox::onMouseDown - args(%.2f,%.2f), bounds(%.2f,%.2f,%.2f,%.2f), localY: %.2f, headerHeight: %.2f, itemHeight: %.2f, isOpen: %d",
    //        args.x, args.y, m_bounds.x, m_bounds.y, m_bounds.width, m_bounds.height,
    //        localY, headerHeight, m_itemHeight, m_isOpen);

    // Check if clicking on header
    if (localY < headerHeight) {
        // Toggle dropdown
        //SDL_Log("Clicking on header, toggling dropdown");
        if (m_isOpen) {
            close();
        } else {
            open();
        }
        args.handled = true;
        return;
    }

    // Check if clicking on an item in the dropdown
    if (m_isOpen && localY >= headerHeight) {
        const float itemY = localY - headerHeight;
        const int itemIndex = static_cast<int>(itemY / m_itemHeight);

        //SDL_Log("Clicking on dropdown item, itemY: %.2f, itemIndex: %d (from %.2f / %.2f), numItems: %zu",
        //        itemY, itemIndex, itemY, m_itemHeight, m_items.size());

        if (itemIndex >= 0 && itemIndex < static_cast<int>(m_items.size())) {
            //SDL_Log("Selecting item: %s", m_items[itemIndex].c_str());
            selectItem(itemIndex);
            close();
            args.handled = true;
        } else {
            //SDL_Log("Item index %d out of range [0, %zu)", itemIndex, m_items.size());
        }
    } else {
        //SDL_Log("Not clicking on dropdown - isOpen: %d, localY >= headerHeight: %d",
        //        m_isOpen, localY >= headerHeight);
    }
}

void ComboBox::onMouseMove(MouseEventArgs& args) {
    StyledControl::onMouseMove(args);

    // Calculate header height (same calculation as in measure())
    float headerHeight = m_fontSize + m_padding.verticalSum();
    headerHeight = std::max(headerHeight, 24.0f);

    // Convert to local coordinates
    const float localX = args.x - m_bounds.x;
    const float localY = args.y - m_bounds.y;

    // Update header hover state
    m_headerHovered = (localY < headerHeight);

    // Update item hover state if dropdown is open
    if (m_isOpen && localY >= headerHeight) {
        const float itemY = localY - headerHeight;
        const int itemIndex = static_cast<int>(itemY / m_itemHeight);

        if (itemIndex >= 0 && itemIndex < static_cast<int>(m_items.size())) {
            //if (m_hoveredItemIndex != itemIndex) {
            //    SDL_Log("ComboBox hover changed to item %d: %s", itemIndex, m_items[itemIndex].c_str());
            //}
            m_hoveredItemIndex = itemIndex;
        } else {
            m_hoveredItemIndex = -1;
        }
    } else {
        m_hoveredItemIndex = -1;
    }
}

void ComboBox::onMouseLeave() {
    StyledControl::onMouseLeave();
    m_headerHovered = false;
    m_hoveredItemIndex = -1;
}

void ComboBox::onActivate() {
    // Toggle dropdown when activated via keyboard
    if (m_isOpen) {
        close();
    } else {
        open();
    }
}

void ComboBox::rebuildDropdown() {
    // This would be used if we were building actual child elements
    // For now, we're rendering directly
}

void ComboBox::selectItem(int index) {
    m_selectedIndex = index;

    //SDL_Log("ComboBox::selectItem called with index %d", index);
    if (m_onSelectionChanged && index >= 0 && index < static_cast<int>(m_items.size())) {
        //SDL_Log("Triggering onSelectionChanged callback with: %s", m_items[index].c_str());
        m_onSelectionChanged(m_items[index]);
    } else {
        //SDL_Log("Callback NOT triggered - callback exists: %d, index valid: %d",
        //        m_onSelectionChanged != nullptr,
        //        index >= 0 && index < static_cast<int>(m_items.size()));
    }
}

void ComboBox::updateHeaderText() {
    // This would update a text element if we had one
    // For now, the renderer will read the selected item directly
}

} // namespace ui
