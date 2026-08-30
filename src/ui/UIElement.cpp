#include "../../include/ui/UIElement.hpp"
#include <algorithm>

namespace ui {

UIElement::UIElement(ElementID id) noexcept
    : m_id(id == INVALID_ELEMENT_ID ? ElementIDGenerator::next() : id) {
}

UIElement::~UIElement() {
    clearChildren();
}

UIElement::UIElement(UIElement&& other) noexcept
    : m_id(other.m_id)
    , m_name(std::move(other.m_name))
    , m_parent(other.m_parent)
    , m_children(std::move(other.m_children))
    , m_bounds(other.m_bounds)
    , m_localBounds(other.m_localBounds)
    , m_sizeConstraints(other.m_sizeConstraints)
    , m_textMeasurer(std::move(other.m_textMeasurer))
    , m_visibility(other.m_visibility)
    , m_enabled(other.m_enabled)
    , m_layoutDirty(other.m_layoutDirty)
    , m_measuredWidth(other.m_measuredWidth)
    , m_measuredHeight(other.m_measuredHeight) {
    // Update children's parent pointer
    for (auto& child : m_children) {
        if (child) {
            child->setParent(this);
        }
    }
    other.m_id = INVALID_ELEMENT_ID;
    other.m_parent = nullptr;
}

UIElement& UIElement::operator=(UIElement&& other) noexcept {
    if (this != &other) {
        clearChildren();

        m_id = other.m_id;
        m_name = std::move(other.m_name);
        m_parent = other.m_parent;
        m_children = std::move(other.m_children);
        m_bounds = other.m_bounds;
        m_localBounds = other.m_localBounds;
        m_sizeConstraints = other.m_sizeConstraints;
        m_textMeasurer = std::move(other.m_textMeasurer);
        m_visibility = other.m_visibility;
        m_enabled = other.m_enabled;
        m_layoutDirty = other.m_layoutDirty;
        m_measuredWidth = other.m_measuredWidth;
        m_measuredHeight = other.m_measuredHeight;

        for (auto& child : m_children) {
            if (child) {
                child->setParent(this);
            }
        }

        other.m_id = INVALID_ELEMENT_ID;
        other.m_parent = nullptr;
    }
    return *this;
}

void UIElement::setTextMeasurer(TextMeasureCallback callback) {
    m_textMeasurer = std::move(callback);
    invalidateLayout();

    for (auto& child : m_children) {
        if (child) {
            child->setTextMeasurer(m_textMeasurer);
        }
    }
}

TextMetrics UIElement::measureText(const std::string& text, float fontSize) const {
    if (m_textMeasurer) {
        return m_textMeasurer(text, fontSize);
    }

    return TextMetrics{
        static_cast<float>(text.length()) * fontSize * 0.6f,
        fontSize * 1.2f
    };
}

void UIElement::addChild(std::unique_ptr<UIElement> child) {
    if (!child) {
        return;
    }

    // Remove from previous parent
    if (child->m_parent) {
        child->m_parent->removeChild(child.get());
    }

    child->setParent(this);
    if (m_textMeasurer) {
        child->setTextMeasurer(m_textMeasurer);
    }
    m_children.push_back(std::move(child));
    invalidateLayout();
}

std::unique_ptr<UIElement> UIElement::removeChild(UIElement* child) {
    if (!child) {
        return nullptr;
    }

    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const std::unique_ptr<UIElement>& ptr) {
            return ptr.get() == child;
        });

    if (it != m_children.end()) {
        std::unique_ptr<UIElement> removed = std::move(*it);
        m_children.erase(it);
        removed->setParent(nullptr);
        invalidateLayout();
        return removed;
    }

    return nullptr;
}

void UIElement::clearChildren() {
    for (auto& child : m_children) {
        if (child) {
            child->setParent(nullptr);
        }
    }
    m_children.clear();
    invalidateLayout();
}

void UIElement::setBounds(const Rect& bounds) noexcept {
    m_bounds = bounds;
}

void UIElement::setLocalBounds(const Rect& bounds) noexcept {
    m_localBounds = bounds;
}

void UIElement::invalidateLayout() noexcept {
    m_layoutDirty = true;
    if (m_parent) {
        m_parent->invalidateLayout();
    }
}

void UIElement::measure(float availableWidth, float availableHeight) {
    // Default: use preferred size or available space
    m_measuredWidth = std::clamp(
        m_sizeConstraints.preferred_width > 0 ? m_sizeConstraints.preferred_width : availableWidth,
        m_sizeConstraints.min_width,
        std::min(m_sizeConstraints.max_width, availableWidth)
    );

    m_measuredHeight = std::clamp(
        m_sizeConstraints.preferred_height > 0 ? m_sizeConstraints.preferred_height : availableHeight,
        m_sizeConstraints.min_height,
        std::min(m_sizeConstraints.max_height, availableHeight)
    );

    // Measure children
    for (auto& child : m_children) {
        if (child && child->visibility() != Visibility::Collapsed) {
            child->measure(m_measuredWidth, m_measuredHeight);
        }
    }
}

void UIElement::arrange(const Rect& finalRect) {
    m_localBounds = finalRect;

    // Calculate screen-space bounds
    if (m_parent) {
        m_bounds.x = m_parent->m_bounds.x + finalRect.x;
        m_bounds.y = m_parent->m_bounds.y + finalRect.y;
    } else {
        m_bounds.x = finalRect.x;
        m_bounds.y = finalRect.y;
    }
    m_bounds.width = finalRect.width;
    m_bounds.height = finalRect.height;

    // Default: arrange children at origin with their measured size
    for (auto& child : m_children) {
        if (child && child->visibility() != Visibility::Collapsed) {
            Rect childRect{0, 0, child->m_measuredWidth, child->m_measuredHeight};
            child->arrange(childRect);
        }
    }

    markLayoutClean();
}

void UIElement::update(float dt) {
    for (auto& child : m_children) {
        if (child && child->isVisible()) {
            child->update(dt);
        }
    }
}

bool UIElement::hitTest(float x, float y) const {
    if (m_visibility != Visibility::Visible || !m_enabled) {
        return false;
    }
    return m_bounds.contains(x, y);
}

UIElement* UIElement::hitTestRecursive(float x, float y) {
    if (!hitTest(x, y)) {
        return nullptr;
    }

    // Check children in reverse order (top-most first)
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if (*it) {
            UIElement* hit = (*it)->hitTestRecursive(x, y);
            if (hit) {
                return hit;
            }
        }
    }

    return this;
}

} // namespace ui
