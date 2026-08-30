#include "../../include/ui/FocusableControl.hpp"
#include <algorithm>

namespace ui {

FocusableControl::FocusableControl(ElementID id) noexcept
    : InputSurface(id) {
}

FocusableControl::~FocusableControl() = default;

FocusableControl::FocusableControl(FocusableControl&& other) noexcept
    : InputSurface(std::move(other))
    , m_focused(other.m_focused)
    , m_focusable(other.m_focusable)
    , m_tabIndex(other.m_tabIndex)
    , m_onFocusCallback(std::move(other.m_onFocusCallback))
    , m_onBlurCallback(std::move(other.m_onBlurCallback)) {
    other.m_focused = false;
}

FocusableControl& FocusableControl::operator=(FocusableControl&& other) noexcept {
    if (this != &other) {
        InputSurface::operator=(std::move(other));
        m_focused = other.m_focused;
        m_focusable = other.m_focusable;
        m_tabIndex = other.m_tabIndex;
        m_onFocusCallback = std::move(other.m_onFocusCallback);
        m_onBlurCallback = std::move(other.m_onBlurCallback);

        other.m_focused = false;
    }
    return *this;
}

void FocusableControl::setFocused(bool focused) noexcept {
    m_focused = focused;
}

void FocusableControl::onFocus() {
    if (m_onFocusCallback) {
        m_onFocusCallback();
    }
}

void FocusableControl::onBlur() {
    if (m_onBlurCallback) {
        m_onBlurCallback();
    }
}

void FocusableControl::onMouseDown(MouseEventArgs& args) {
    InputSurface::onMouseDown(args);

    // Request focus on click (focus manager should handle this)
    // This is a hint that the control wants focus
}

void FocusableControl::onKeyDown(KeyEventArgs& args) {
    InputSurface::onKeyDown(args);

    if (!m_focused || args.handled) {
        return;
    }

    // Handle activation keys (Enter, Space)
    // SDL scancodes: Enter = 40, Space = 44
    if (args.scancode == 40 || args.scancode == 44) {
        onActivate();
        args.handled = true;
    }
}

void FocusableControl::onActivate() {
    // Override in derived classes for specific activation behaviour
}

// FocusManager implementation
void FocusManager::setFocus(FocusableControl* control) {
    if (m_focused == control) {
        return;
    }

    if (control && !control->isFocusable()) {
        return;
    }

    // Blur current
    if (m_focused) {
        m_focused->setFocused(false);
        m_focused->onBlur();
    }

    m_focused = control;

    // Focus new
    if (m_focused) {
        m_focused->setFocused(true);
        m_focused->onFocus();
    }
}

void FocusManager::clearFocus() {
    setFocus(nullptr);
}

void FocusManager::discardFocus() noexcept {
    m_focused = nullptr;
}

void FocusManager::registerControl(FocusableControl* control) {
    if (!control) {
        return;
    }

    auto it = std::find(m_focusableControls.begin(), m_focusableControls.end(), control);
    if (it == m_focusableControls.end()) {
        m_focusableControls.push_back(control);
    }
}

void FocusManager::unregisterControl(FocusableControl* control) {
    if (!control) {
        return;
    }

    if (m_focused == control) {
        clearFocus();
    }

    auto it = std::find(m_focusableControls.begin(), m_focusableControls.end(), control);
    if (it != m_focusableControls.end()) {
        m_focusableControls.erase(it);
    }
}

void FocusManager::collectFocusableControls(UIElement* root, std::vector<FocusableControl*>& controls) {
    if (!root || !root->isVisible()) {
        return;
    }

    auto* focusable = dynamic_cast<FocusableControl*>(root);
    if (focusable && focusable->isFocusable()) {
        controls.push_back(focusable);
    }

    for (const auto& child : root->children()) {
        if (child) {
            collectFocusableControls(child.get(), controls);
        }
    }
}

FocusableControl* FocusManager::findNextFocusable(FocusDirection direction) {
    if (m_focusableControls.empty()) {
        return nullptr;
    }

    // Sort by tab index
    std::vector<FocusableControl*> sorted = m_focusableControls;
    std::sort(sorted.begin(), sorted.end(),
        [](const FocusableControl* a, const FocusableControl* b) {
            return a->tabIndex() < b->tabIndex();
        });

    // Remove non-focusable
    sorted.erase(
        std::remove_if(sorted.begin(), sorted.end(),
            [](const FocusableControl* c) { return !c->isFocusable(); }),
        sorted.end());

    if (sorted.empty()) {
        return nullptr;
    }

    if (!m_focused) {
        return sorted.front();
    }

    auto it = std::find(sorted.begin(), sorted.end(), m_focused);
    if (it == sorted.end()) {
        return sorted.front();
    }

    if (direction == FocusDirection::Next) {
        ++it;
        if (it == sorted.end()) {
            it = sorted.begin();
        }
    } else if (direction == FocusDirection::Previous) {
        if (it == sorted.begin()) {
            it = sorted.end();
        }
        --it;
    }

    return *it;
}

void FocusManager::moveFocus(FocusDirection direction, UIElement* root) {
    // Rebuild focusable list from tree
    m_focusableControls.clear();
    if (root) {
        collectFocusableControls(root, m_focusableControls);
    }

    FocusableControl* next = findNextFocusable(direction);
    if (next) {
        setFocus(next);
    }
}

} // namespace ui
