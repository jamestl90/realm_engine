#include "../../include/ui/InputSurface.hpp"

namespace ui {

InputSurface::InputSurface(ElementID id) noexcept
    : UIElement(id) {
}

InputSurface::~InputSurface() = default;

InputSurface::InputSurface(InputSurface&& other) noexcept
    : UIElement(std::move(other))
    , m_hovered(other.m_hovered)
    , m_pressed(other.m_pressed)
    , m_onMouseDown(std::move(other.m_onMouseDown))
    , m_onMouseUp(std::move(other.m_onMouseUp))
    , m_onMouseMove(std::move(other.m_onMouseMove))
    , m_onMouseWheel(std::move(other.m_onMouseWheel))
    , m_onKeyDown(std::move(other.m_onKeyDown))
    , m_onKeyUp(std::move(other.m_onKeyUp))
    , m_onTextInput(std::move(other.m_onTextInput))
    , m_onHoverChanged(std::move(other.m_onHoverChanged)) {
    other.m_hovered = false;
    other.m_pressed = false;
}

InputSurface& InputSurface::operator=(InputSurface&& other) noexcept {
    if (this != &other) {
        UIElement::operator=(std::move(other));
        m_hovered = other.m_hovered;
        m_pressed = other.m_pressed;
        m_onMouseDown = std::move(other.m_onMouseDown);
        m_onMouseUp = std::move(other.m_onMouseUp);
        m_onMouseMove = std::move(other.m_onMouseMove);
        m_onMouseWheel = std::move(other.m_onMouseWheel);
        m_onKeyDown = std::move(other.m_onKeyDown);
        m_onKeyUp = std::move(other.m_onKeyUp);
        m_onTextInput = std::move(other.m_onTextInput);
        m_onHoverChanged = std::move(other.m_onHoverChanged);

        other.m_hovered = false;
        other.m_pressed = false;
    }
    return *this;
}

void InputSurface::setHovered(bool hovered) noexcept {
    if (m_hovered != hovered) {
        m_hovered = hovered;
        if (m_onHoverChanged) {
            m_onHoverChanged(hovered);
        }
    }
}

void InputSurface::toLocalCoordinates(MouseEventArgs& args) const {
    args.localX = args.x - bounds().x;
    args.localY = args.y - bounds().y;
}

void InputSurface::onMouseDown(MouseEventArgs& args) {
    if (!isEnabled()) {
        return;
    }

    toLocalCoordinates(args);
    m_pressed = true;

    if (m_onMouseDown) {
        m_onMouseDown(args);
    }
}

void InputSurface::onMouseUp(MouseEventArgs& args) {
    if (!isEnabled()) {
        return;
    }

    toLocalCoordinates(args);
    m_pressed = false;

    if (m_onMouseUp) {
        m_onMouseUp(args);
    }
}

void InputSurface::onMouseMove(MouseEventArgs& args) {
    if (!isEnabled()) {
        return;
    }

    toLocalCoordinates(args);

    if (m_onMouseMove) {
        m_onMouseMove(args);
    }
}

void InputSurface::onMouseWheel(MouseWheelEventArgs& args) {
    if (!isEnabled()) {
        return;
    }

    if (m_onMouseWheel) {
        m_onMouseWheel(args);
    }
}

void InputSurface::onMouseEnter() {
    setHovered(true);
}

void InputSurface::onMouseLeave() {
    setHovered(false);
    m_pressed = false;
}

void InputSurface::onKeyDown(KeyEventArgs& args) {
    if (!isEnabled()) {
        return;
    }

    if (m_onKeyDown) {
        m_onKeyDown(args);
    }
}

void InputSurface::onKeyUp(KeyEventArgs& args) {
    if (!isEnabled()) {
        return;
    }

    if (m_onKeyUp) {
        m_onKeyUp(args);
    }
}

void InputSurface::onTextInput(TextInputEventArgs& args) {
    if (!isEnabled()) {
        return;
    }

    if (m_onTextInput) {
        m_onTextInput(args);
    }
}

} // namespace ui
