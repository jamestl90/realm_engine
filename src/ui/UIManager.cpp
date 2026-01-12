#include "../../include/ui/UIManager.hpp"
#include "../../include/ui/InputSurface.hpp"
#include "../../include/ui/TextBox.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>

namespace ui {

void UIManager::initialize(SDL_Window* window, float screenWidth, float screenHeight) noexcept {
    m_window = window;
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    m_layoutDirty = true;
    m_textInputActive = false;
}

void UIManager::setScreenSize(float width, float height) noexcept {
    if (m_screenWidth != width || m_screenHeight != height) {
        m_screenWidth = width;
        m_screenHeight = height;
        m_layoutDirty = true;
    }
}

void UIManager::setRoot(std::unique_ptr<UIElement> root) {
    m_root = std::move(root);
    m_layoutDirty = true;
    m_hoveredElement = nullptr;
    m_lastHoveredSurface = nullptr;
    m_capturedElement = nullptr;
    m_focusManager.clearFocus();
    updateTextInputState(nullptr);

    // Configure text measurement on new UI tree
    if (m_root) {
        configureTextMeasurement(m_root.get());
    }
}

void UIManager::update(float dt) {
    // Update animations
    m_animationManager.update(dt);
    m_animationManager.removeCompleted();

    // Perform layout if needed
    if (m_layoutDirty && m_root) {
        performLayout();
    }

    // Update UI tree
    if (m_root) {
        m_root->update(dt);
    }
}

void UIManager::performLayout() {
    if (!m_root) {
        return;
    }

    // Measure pass
    m_root->measure(m_screenWidth, m_screenHeight);

    // Arrange pass - use measured size, not full screen size
    Rect rootRect{0.0f, 0.0f, m_root->measuredWidth(), m_root->measuredHeight()};
    m_root->arrange(rootRect);

    m_layoutDirty = false;
}

bool UIManager::handleEvent(const SDL_Event& event) {
    if (!m_root) {
        return false;
    }

    switch (event.type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            return handleMouseButtonDown(event);

        case SDL_EVENT_MOUSE_BUTTON_UP:
            return handleMouseButtonUp(event);

        case SDL_EVENT_MOUSE_MOTION:
            return handleMouseMotion(event);

        case SDL_EVENT_MOUSE_WHEEL:
            return handleMouseWheel(event);

        case SDL_EVENT_KEY_DOWN:
            return handleKeyDown(event);

        case SDL_EVENT_KEY_UP:
            return handleKeyUp(event);

        case SDL_EVENT_TEXT_INPUT:
            return handleTextInput(event);

        default:
            return false;
    }
}

bool UIManager::handleMouseButtonDown(const SDL_Event& event) {
    float x = event.button.x;
    float y = event.button.y;
    m_mouseX = x;
    m_mouseY = y;

    MouseEventArgs args;
    args.x = x;
    args.y = y;
    args.button = toMouseButton(event.button.button);

    // If we have a captured element, send to it
    if (m_capturedElement) {
        m_capturedElement->onMouseDown(args);
        return args.handled;
    }

    // Hit test to find target
    UIElement* target = hitTest(x, y);
    if (!target) {
        // Click outside UI - clear focus
        m_focusManager.clearFocus();
        updateTextInputState(nullptr);
        return false;
    }

    // Try to find an InputSurface in the hierarchy
    UIElement* current = target;
    while (current) {
        auto* surface = dynamic_cast<InputSurface*>(current);
        if (surface) {
            surface->onMouseDown(args);

            // Handle focus for focusable controls
            auto* focusable = dynamic_cast<FocusableControl*>(surface);
            if (focusable && focusable->isFocusable()) {
                m_focusManager.setFocus(focusable);
                updateTextInputState(focusable);
            }

            return args.handled || true; // Consumed by UI
        }
        current = current->parent();
    }

    return true; // Hit UI element but no input handler
}

bool UIManager::handleMouseButtonUp(const SDL_Event& event) {
    float x = event.button.x;
    float y = event.button.y;
    m_mouseX = x;
    m_mouseY = y;

    MouseEventArgs args;
    args.x = x;
    args.y = y;
    args.button = toMouseButton(event.button.button);

    // If we have a captured element, send to it
    if (m_capturedElement) {
        m_capturedElement->onMouseUp(args);
        return args.handled;
    }

    // Hit test to find target
    UIElement* target = hitTest(x, y);
    if (!target) {
        return false;
    }

    // Try to find an InputSurface in the hierarchy
    UIElement* current = target;
    while (current) {
        auto* surface = dynamic_cast<InputSurface*>(current);
        if (surface) {
            surface->onMouseUp(args);
            return args.handled || true;
        }
        current = current->parent();
    }

    return true;
}

bool UIManager::handleMouseMotion(const SDL_Event& event) {
    float x = event.motion.x;
    float y = event.motion.y;
    m_mouseX = x;
    m_mouseY = y;

    // Update hover state
    updateHover(x, y);

    MouseEventArgs args;
    args.x = x;
    args.y = y;

    // If we have a captured element, send to it
    if (m_capturedElement) {
        m_capturedElement->onMouseMove(args);
        return args.handled;
    }

    // Send move event to hovered surface
    if (m_lastHoveredSurface) {
        m_lastHoveredSurface->onMouseMove(args);
        return args.handled;
    }

    return m_hoveredElement != nullptr;
}

bool UIManager::handleMouseWheel(const SDL_Event& event) {
    MouseWheelEventArgs args;
    args.x = m_mouseX;
    args.y = m_mouseY;
    args.deltaX = event.wheel.x;
    args.deltaY = event.wheel.y;

    // If we have a captured element, send to it
    if (m_capturedElement) {
        m_capturedElement->onMouseWheel(args);
        return args.handled;
    }

    // Send to hovered surface
    if (m_lastHoveredSurface) {
        m_lastHoveredSurface->onMouseWheel(args);
        return args.handled;
    }

    return false;
}

bool UIManager::handleKeyDown(const SDL_Event& event) {
    KeyEventArgs args;
    args.scancode = event.key.scancode;
    args.keycode = event.key.key;
    args.shift = (event.key.mod & SDL_KMOD_SHIFT) != 0;
    args.ctrl = (event.key.mod & SDL_KMOD_CTRL) != 0;
    args.alt = (event.key.mod & SDL_KMOD_ALT) != 0;

    // Handle Tab for focus navigation
    if (event.key.scancode == SDL_SCANCODE_TAB) {
        FocusDirection direction = args.shift ? FocusDirection::Previous : FocusDirection::Next;
        m_focusManager.moveFocus(direction, m_root.get());
        updateTextInputState(m_focusManager.focusedElement());
        return true;
    }

    // Send to focused element
    auto* focused = m_focusManager.focusedElement();
    if (focused) {
        focused->onKeyDown(args);
        return args.handled;
    }

    return false;
}

bool UIManager::handleKeyUp(const SDL_Event& event) {
    KeyEventArgs args;
    args.scancode = event.key.scancode;
    args.keycode = event.key.key;
    args.shift = (event.key.mod & SDL_KMOD_SHIFT) != 0;
    args.ctrl = (event.key.mod & SDL_KMOD_CTRL) != 0;
    args.alt = (event.key.mod & SDL_KMOD_ALT) != 0;

    // Send to focused element
    auto* focused = m_focusManager.focusedElement();
    if (focused) {
        focused->onKeyUp(args);
        return args.handled;
    }

    return false;
}

bool UIManager::handleTextInput(const SDL_Event& event) {
    TextInputEventArgs args;
    
    // Copy text safely
    const char* text = event.text.text;
    std::size_t len = std::strlen(text);
    if (len >= sizeof(args.text)) {
        len = sizeof(args.text) - 1;
    }
    std::memcpy(args.text, text, len);
    args.text[len] = '\0';

    // Send to focused element
    auto* focused = m_focusManager.focusedElement();
    if (focused) {
        focused->onTextInput(args);
        return args.handled;
    }

    return false;
}

void UIManager::updateHover(float x, float y) {
    UIElement* newHovered = hitTest(x, y);
    
    // Find the InputSurface for the new hovered element
    InputSurface* newSurface = nullptr;
    UIElement* current = newHovered;
    while (current) {
        auto* surface = dynamic_cast<InputSurface*>(current);
        if (surface) {
            newSurface = surface;
            break;
        }
        current = current->parent();
    }

    // Handle enter/leave events
    if (newSurface != m_lastHoveredSurface) {
        if (m_lastHoveredSurface) {
            m_lastHoveredSurface->onMouseLeave();
        }
        if (newSurface) {
            newSurface->onMouseEnter();
        }
        m_lastHoveredSurface = newSurface;
    }

    m_hoveredElement = newHovered;
}

void UIManager::updateTextInputState(FocusableControl* newFocused) {
    bool needsTextInput = newFocused && newFocused->wantsTextInput();

    if (needsTextInput && !m_textInputActive) {
        SDL_StartTextInput(m_window);
        m_textInputActive = true;
    } else if (!needsTextInput && m_textInputActive) {
        SDL_StopTextInput(m_window);
        m_textInputActive = false;
    }
}

UIElement* UIManager::hitTest(float x, float y) const {
    if (!m_root) {
        return nullptr;
    }
    return m_root->hitTestRecursive(x, y);
}

MouseButton UIManager::toMouseButton(std::uint8_t sdlButton) const noexcept {
    switch (sdlButton) {
        case SDL_BUTTON_LEFT:
            return MouseButton::Left;
        case SDL_BUTTON_MIDDLE:
            return MouseButton::Middle;
        case SDL_BUTTON_RIGHT:
            return MouseButton::Right;
        case SDL_BUTTON_X1:
            return MouseButton::X1;
        case SDL_BUTTON_X2:
            return MouseButton::X2;
        default:
            return MouseButton::Left;
    }
}

void UIManager::setFontManager(rendering::FontManager* fontManager, rendering::FontID defaultFont) noexcept {
    m_fontManager = fontManager;
    m_defaultFont = defaultFont;

    // Configure text measurement on existing UI tree
    if (m_root) {
        configureTextMeasurement(m_root.get());
    }
}

void UIManager::configureTextMeasurement(UIElement* element) {
    if (!element || !m_fontManager || m_defaultFont == rendering::INVALID_FONT_ID) {
        return;
    }

    // If this is a TextBox, set up the text measurer
    auto* textBox = dynamic_cast<TextBox*>(element);
    if (textBox) {
        // Capture fontManager and defaultFont by value for the lambda
        rendering::FontManager* fm = m_fontManager;
        rendering::FontID fontId = m_defaultFont;

        textBox->setTextMeasurer([fm, fontId](const std::string& text, float /*fontSize*/) -> float {
            int width = 0;
            if (fm->getTextSize(fontId, text.c_str(), &width, nullptr)) {
                return static_cast<float>(width);
            }
            return 0.0f;
        });
    }

    // Recursively configure children
    for (const auto& child : element->children()) {
        configureTextMeasurement(child.get());
    }
}

} // namespace ui
