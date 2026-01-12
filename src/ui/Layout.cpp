#include "../../include/ui/Layout.hpp"
#include <algorithm>
#include <limits>
#include <unordered_map>

namespace ui {

// Static storage for grid attached properties
static std::unordered_map<ElementID, GridAttachedProperties> s_gridProperties;

// LayoutContainer implementation
LayoutContainer::LayoutContainer(ElementID id) noexcept
    : UIElement(id) {
}

LayoutContainer::~LayoutContainer() = default;

LayoutContainer::LayoutContainer(LayoutContainer&& other) noexcept
    : UIElement(std::move(other))
    , m_padding(other.m_padding)
    , m_backgroundColour(other.m_backgroundColour) {
}

LayoutContainer& LayoutContainer::operator=(LayoutContainer&& other) noexcept {
    if (this != &other) {
        UIElement::operator=(std::move(other));
        m_padding = other.m_padding;
        m_backgroundColour = other.m_backgroundColour;
    }
    return *this;
}

void LayoutContainer::setPadding(const Thickness& padding) noexcept {
    m_padding = padding;
    invalidateLayout();
}

Rect LayoutContainer::getContentArea() const noexcept {
    return Rect{
        m_padding.left,
        m_padding.top,
        std::max(0.0f, m_localBounds.width - m_padding.horizontalSum()),
        std::max(0.0f, m_localBounds.height - m_padding.verticalSum())
    };
}

// StackPanel implementation
StackPanel::StackPanel(Orientation orientation, ElementID id) noexcept
    : LayoutContainer(id)
    , m_orientation(orientation) {
}

StackPanel::~StackPanel() = default;

StackPanel::StackPanel(StackPanel&& other) noexcept
    : LayoutContainer(std::move(other))
    , m_orientation(other.m_orientation)
    , m_spacing(other.m_spacing) {
}

StackPanel& StackPanel::operator=(StackPanel&& other) noexcept {
    if (this != &other) {
        LayoutContainer::operator=(std::move(other));
        m_orientation = other.m_orientation;
        m_spacing = other.m_spacing;
    }
    return *this;
}

void StackPanel::setOrientation(Orientation orientation) noexcept {
    if (m_orientation != orientation) {
        m_orientation = orientation;
        invalidateLayout();
    }
}

void StackPanel::setSpacing(float spacing) noexcept {
    if (m_spacing != spacing) {
        m_spacing = spacing;
        invalidateLayout();
    }
}

void StackPanel::measure(float availableWidth, float availableHeight) {
    // Apply our own size constraints to limit available space for children
    const auto& constraints = sizeConstraints();

    // If we have a preferred or constrained width, use it to limit child available space
    float constrainedWidth = availableWidth;
    if (constraints.preferred_width > 0.0f) {
        constrainedWidth = std::min(constrainedWidth, constraints.preferred_width);
    }
    if (constraints.max_width < std::numeric_limits<float>::max()) {
        constrainedWidth = std::min(constrainedWidth, constraints.max_width);
    }
    constrainedWidth = std::max(constrainedWidth, constraints.min_width);

    // Same for height
    float constrainedHeight = availableHeight;
    if (constraints.preferred_height > 0.0f) {
        constrainedHeight = std::min(constrainedHeight, constraints.preferred_height);
    }
    if (constraints.max_height < std::numeric_limits<float>::max()) {
        constrainedHeight = std::min(constrainedHeight, constraints.max_height);
    }
    constrainedHeight = std::max(constrainedHeight, constraints.min_height);

    float contentWidth = constrainedWidth - m_padding.horizontalSum();
    float contentHeight = constrainedHeight - m_padding.verticalSum();

    float totalMain = 0.0f;
    float maxCross = 0.0f;
    std::size_t visibleCount = 0;

    for (auto& child : children()) {
        if (!child || child->visibility() == Visibility::Collapsed) {
            continue;
        }

        if (m_orientation == Orientation::Vertical) {
            child->measure(contentWidth, contentHeight - totalMain);
            totalMain += child->measuredHeight();
            maxCross = std::max(maxCross, child->measuredWidth());
        } else {
            child->measure(contentWidth - totalMain, contentHeight);
            totalMain += child->measuredWidth();
            maxCross = std::max(maxCross, child->measuredHeight());
        }
        ++visibleCount;
    }

    // Add spacing
    if (visibleCount > 1) {
        totalMain += m_spacing * static_cast<float>(visibleCount - 1);
    }

    if (m_orientation == Orientation::Vertical) {
        m_measuredWidth = maxCross + m_padding.horizontalSum();
        m_measuredHeight = totalMain + m_padding.verticalSum();
    } else {
        m_measuredWidth = totalMain + m_padding.horizontalSum();
        m_measuredHeight = maxCross + m_padding.verticalSum();
    }

    // Apply constraints to final measured size
    m_measuredWidth = std::clamp(m_measuredWidth, constraints.min_width,
        std::min(constraints.max_width, availableWidth));
    m_measuredHeight = std::clamp(m_measuredHeight, constraints.min_height,
        std::min(constraints.max_height, availableHeight));
}

void StackPanel::arrange(const Rect& finalRect) {
    m_localBounds = finalRect;

    if (parent()) {
        m_bounds.x = parent()->bounds().x + finalRect.x;
        m_bounds.y = parent()->bounds().y + finalRect.y;
    } else {
        m_bounds.x = finalRect.x;
        m_bounds.y = finalRect.y;
    }
    m_bounds.width = finalRect.width;
    m_bounds.height = finalRect.height;

    Rect content = getContentArea();
    float offset = 0.0f;

    for (auto& child : children()) {
        if (!child || child->visibility() == Visibility::Collapsed) {
            continue;
        }

        Rect childRect;
        if (m_orientation == Orientation::Vertical) {
            childRect.x = content.x;
            childRect.y = content.y + offset;
            childRect.width = content.width;
            childRect.height = child->measuredHeight();
            offset += child->measuredHeight() + m_spacing;
        } else {
            childRect.x = content.x + offset;
            childRect.y = content.y;
            childRect.width = child->measuredWidth();
            childRect.height = content.height;
            offset += child->measuredWidth() + m_spacing;
        }

        child->arrange(childRect);
    }

    markLayoutClean();
}

// GridPanel implementation
GridPanel::GridPanel(ElementID id) noexcept
    : LayoutContainer(id) {
}

GridPanel::~GridPanel() = default;

GridPanel::GridPanel(GridPanel&& other) noexcept
    : LayoutContainer(std::move(other))
    , m_rowDefinitions(std::move(other.m_rowDefinitions))
    , m_columnDefinitions(std::move(other.m_columnDefinitions))
    , m_rowHeights(std::move(other.m_rowHeights))
    , m_columnWidths(std::move(other.m_columnWidths)) {
}

GridPanel& GridPanel::operator=(GridPanel&& other) noexcept {
    if (this != &other) {
        LayoutContainer::operator=(std::move(other));
        m_rowDefinitions = std::move(other.m_rowDefinitions);
        m_columnDefinitions = std::move(other.m_columnDefinitions);
        m_rowHeights = std::move(other.m_rowHeights);
        m_columnWidths = std::move(other.m_columnWidths);
    }
    return *this;
}

void GridPanel::addRowDefinition(const GridDefinition& def) {
    m_rowDefinitions.push_back(def);
    invalidateLayout();
}

void GridPanel::addColumnDefinition(const GridDefinition& def) {
    m_columnDefinitions.push_back(def);
    invalidateLayout();
}

void GridPanel::clearDefinitions() {
    m_rowDefinitions.clear();
    m_columnDefinitions.clear();
    invalidateLayout();
}

void GridPanel::setRow(UIElement* element, std::uint32_t row) {
    if (element) {
        s_gridProperties[element->id()].row = row;
    }
}

void GridPanel::setColumn(UIElement* element, std::uint32_t column) {
    if (element) {
        s_gridProperties[element->id()].column = column;
    }
}

void GridPanel::setRowSpan(UIElement* element, std::uint32_t span) {
    if (element) {
        s_gridProperties[element->id()].rowSpan = std::max(1u, span);
    }
}

void GridPanel::setColumnSpan(UIElement* element, std::uint32_t span) {
    if (element) {
        s_gridProperties[element->id()].columnSpan = std::max(1u, span);
    }
}

void GridPanel::calculateRowHeights(float availableHeight) {
    std::size_t rowCount = std::max(std::size_t{1}, m_rowDefinitions.size());
    m_rowHeights.resize(rowCount, 0.0f);

    float fixedHeight = 0.0f;
    float totalProportion = 0.0f;

    for (std::size_t i = 0; i < m_rowDefinitions.size(); ++i) {
        const auto& def = m_rowDefinitions[i];
        if (!def.isAuto && def.size > 0) {
            m_rowHeights[i] = def.size;
            fixedHeight += def.size;
        } else if (!def.isAuto) {
            totalProportion += def.proportion;
        }
    }

    // Distribute remaining space proportionally
    float remaining = availableHeight - fixedHeight;
    if (remaining > 0 && totalProportion > 0) {
        for (std::size_t i = 0; i < m_rowDefinitions.size(); ++i) {
            const auto& def = m_rowDefinitions[i];
            if (!def.isAuto && def.size == 0) {
                m_rowHeights[i] = remaining * (def.proportion / totalProportion);
            }
        }
    }

    // Handle auto rows - measure children to determine size
    for (std::size_t i = 0; i < m_rowDefinitions.size(); ++i) {
        if (m_rowDefinitions[i].isAuto) {
            float maxHeight = 0.0f;
            for (const auto& child : children()) {
                if (!child || child->visibility() == Visibility::Collapsed) {
                    continue;
                }
                auto it = s_gridProperties.find(child->id());
                if (it != s_gridProperties.end() && it->second.row == i) {
                    maxHeight = std::max(maxHeight, child->measuredHeight());
                }
            }
            m_rowHeights[i] = maxHeight;
        }
    }

    // Default single row if no definitions
    if (m_rowDefinitions.empty()) {
        m_rowHeights[0] = availableHeight;
    }
}

void GridPanel::calculateColumnWidths(float availableWidth) {
    std::size_t colCount = std::max(std::size_t{1}, m_columnDefinitions.size());
    m_columnWidths.resize(colCount, 0.0f);

    float fixedWidth = 0.0f;
    float totalProportion = 0.0f;

    for (std::size_t i = 0; i < m_columnDefinitions.size(); ++i) {
        const auto& def = m_columnDefinitions[i];
        if (!def.isAuto && def.size > 0) {
            m_columnWidths[i] = def.size;
            fixedWidth += def.size;
        } else if (!def.isAuto) {
            totalProportion += def.proportion;
        }
    }

    // Distribute remaining space proportionally
    float remaining = availableWidth - fixedWidth;
    if (remaining > 0 && totalProportion > 0) {
        for (std::size_t i = 0; i < m_columnDefinitions.size(); ++i) {
            const auto& def = m_columnDefinitions[i];
            if (!def.isAuto && def.size == 0) {
                m_columnWidths[i] = remaining * (def.proportion / totalProportion);
            }
        }
    }

    // Handle auto columns
    for (std::size_t i = 0; i < m_columnDefinitions.size(); ++i) {
        if (m_columnDefinitions[i].isAuto) {
            float maxWidth = 0.0f;
            for (const auto& child : children()) {
                if (!child || child->visibility() == Visibility::Collapsed) {
                    continue;
                }
                auto it = s_gridProperties.find(child->id());
                if (it != s_gridProperties.end() && it->second.column == i) {
                    maxWidth = std::max(maxWidth, child->measuredWidth());
                }
            }
            m_columnWidths[i] = maxWidth;
        }
    }

    // Default single column if no definitions
    if (m_columnDefinitions.empty()) {
        m_columnWidths[0] = availableWidth;
    }
}

void GridPanel::measure(float availableWidth, float availableHeight) {
    float contentWidth = availableWidth - m_padding.horizontalSum();
    float contentHeight = availableHeight - m_padding.verticalSum();

    // First pass: measure all children
    for (auto& child : children()) {
        if (child && child->visibility() != Visibility::Collapsed) {
            child->measure(contentWidth, contentHeight);
        }
    }

    // Calculate row and column sizes
    calculateColumnWidths(contentWidth);
    calculateRowHeights(contentHeight);

    // Calculate total size
    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
    for (float w : m_columnWidths) totalWidth += w;
    for (float h : m_rowHeights) totalHeight += h;

    m_measuredWidth = totalWidth + m_padding.horizontalSum();
    m_measuredHeight = totalHeight + m_padding.verticalSum();

    const auto& constraints = sizeConstraints();
    m_measuredWidth = std::clamp(m_measuredWidth, constraints.min_width,
        std::min(constraints.max_width, availableWidth));
    m_measuredHeight = std::clamp(m_measuredHeight, constraints.min_height,
        std::min(constraints.max_height, availableHeight));
}

void GridPanel::arrange(const Rect& finalRect) {
    m_localBounds = finalRect;

    if (parent()) {
        m_bounds.x = parent()->bounds().x + finalRect.x;
        m_bounds.y = parent()->bounds().y + finalRect.y;
    } else {
        m_bounds.x = finalRect.x;
        m_bounds.y = finalRect.y;
    }
    m_bounds.width = finalRect.width;
    m_bounds.height = finalRect.height;

    Rect content = getContentArea();

    // Recalculate with final size
    calculateColumnWidths(content.width);
    calculateRowHeights(content.height);

    // Calculate row/column offsets
    std::vector<float> rowOffsets(m_rowHeights.size() + 1, 0.0f);
    std::vector<float> colOffsets(m_columnWidths.size() + 1, 0.0f);

    for (std::size_t i = 0; i < m_rowHeights.size(); ++i) {
        rowOffsets[i + 1] = rowOffsets[i] + m_rowHeights[i];
    }
    for (std::size_t i = 0; i < m_columnWidths.size(); ++i) {
        colOffsets[i + 1] = colOffsets[i] + m_columnWidths[i];
    }

    // Arrange children
    for (auto& child : children()) {
        if (!child || child->visibility() == Visibility::Collapsed) {
            continue;
        }

        GridAttachedProperties props;
        auto it = s_gridProperties.find(child->id());
        if (it != s_gridProperties.end()) {
            props = it->second;
        }

        std::uint32_t row = std::min(props.row, static_cast<std::uint32_t>(m_rowHeights.size() - 1));
        std::uint32_t col = std::min(props.column, static_cast<std::uint32_t>(m_columnWidths.size() - 1));
        std::uint32_t rowEnd = std::min(row + props.rowSpan, static_cast<std::uint32_t>(m_rowHeights.size()));
        std::uint32_t colEnd = std::min(col + props.columnSpan, static_cast<std::uint32_t>(m_columnWidths.size()));

        Rect childRect;
        childRect.x = content.x + colOffsets[col];
        childRect.y = content.y + rowOffsets[row];
        childRect.width = colOffsets[colEnd] - colOffsets[col];
        childRect.height = rowOffsets[rowEnd] - rowOffsets[row];

        child->arrange(childRect);
    }

    markLayoutClean();
}

} // namespace ui
