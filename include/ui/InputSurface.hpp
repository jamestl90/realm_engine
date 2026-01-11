#pragma once

#include "UIElement.hpp"
#include <functional>

namespace ui {

// Mouse button enumeration
enum class MouseButton : std::uint8_t {
    Left,
    Middle,
    Right,
    X1,
    X2
};

// Input event data structures
struct MouseEventArgs {
    float x{0.0f};
    float y{0.0f};
    float localX{0.0f};
    float localY{0.0f};
    MouseButton button{MouseButton::Left};
    bool handled{false};
};

struct MouseWheelEventArgs {
    float x{0.0f};
    float y{0.0f};
    float deltaX{0.0f};
    float deltaY{0.0f};
    bool handled{false};
};

struct KeyEventArgs {
    std::uint32_t scancode{0};
    std::uint32_t keycode{0};
    bool shift{false};
    bool ctrl{false};
    bool alt{false};
    bool handled{false};
};

struct TextInputEventArgs {
    char text[32]{};
    bool handled{false};
};

// Input surface - handles mouse and keyboard input for a UI element
class InputSurface : public UIElement {
public:
    explicit InputSurface(ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~InputSurface() override;

    InputSurface(InputSurface&&) noexcept;
    InputSurface& operator=(InputSurface&&) noexcept;

    // Mouse state
    [[nodiscard]] bool isHovered() const noexcept { return m_hovered; }
    [[nodiscard]] bool isPressed() const noexcept { return m_pressed; }

    // Event callbacks
    using MouseEventCallback = std::function<void(MouseEventArgs&)>;
    using MouseWheelCallback = std::function<void(MouseWheelEventArgs&)>;
    using KeyEventCallback = std::function<void(KeyEventArgs&)>;
    using TextInputCallback = std::function<void(TextInputEventArgs&)>;
    using HoverCallback = std::function<void(bool)>;

    void setOnMouseDown(MouseEventCallback callback) { m_onMouseDown = std::move(callback); }
    void setOnMouseUp(MouseEventCallback callback) { m_onMouseUp = std::move(callback); }
    void setOnMouseMove(MouseEventCallback callback) { m_onMouseMove = std::move(callback); }
    void setOnMouseWheel(MouseWheelCallback callback) { m_onMouseWheel = std::move(callback); }
    void setOnKeyDown(KeyEventCallback callback) { m_onKeyDown = std::move(callback); }
    void setOnKeyUp(KeyEventCallback callback) { m_onKeyUp = std::move(callback); }
    void setOnTextInput(TextInputCallback callback) { m_onTextInput = std::move(callback); }
    void setOnHoverChanged(HoverCallback callback) { m_onHoverChanged = std::move(callback); }

    // Input dispatch methods (called by UI system)
    virtual void onMouseDown(MouseEventArgs& args);
    virtual void onMouseUp(MouseEventArgs& args);
    virtual void onMouseMove(MouseEventArgs& args);
    virtual void onMouseWheel(MouseWheelEventArgs& args);
    virtual void onMouseEnter();
    virtual void onMouseLeave();
    virtual void onKeyDown(KeyEventArgs& args);
    virtual void onKeyUp(KeyEventArgs& args);
    virtual void onTextInput(TextInputEventArgs& args);

protected:
    void setHovered(bool hovered) noexcept;
    void setPressed(bool pressed) noexcept { m_pressed = pressed; }

    // Convert screen coordinates to local coordinates
    void toLocalCoordinates(MouseEventArgs& args) const;

private:
    bool m_hovered{false};
    bool m_pressed{false};

    MouseEventCallback m_onMouseDown;
    MouseEventCallback m_onMouseUp;
    MouseEventCallback m_onMouseMove;
    MouseWheelCallback m_onMouseWheel;
    KeyEventCallback m_onKeyDown;
    KeyEventCallback m_onKeyUp;
    TextInputCallback m_onTextInput;
    HoverCallback m_onHoverChanged;
};

} // namespace ui
