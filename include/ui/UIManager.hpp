#pragma once

#include "UIElement.hpp"
#include "FocusableControl.hpp"
#include "Animation.hpp"
#include "../rendering/FontManager.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <memory>

namespace ui {

// Forward declarations
class InputSurface;
class TextBox;
class ComboBox;

// UIManager - central coordinator for the UI system
class UIManager {
public:
    UIManager() = default;
    ~UIManager() = default;

    // Non-copyable, non-movable (singleton-like usage)
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;
    UIManager(UIManager&&) = delete;
    UIManager& operator=(UIManager&&) = delete;

    // Initialise with window and logical dimensions
    void initialize(SDL_Window* window, float logicalWidth, float logicalHeight) noexcept;

    // Screen size management
    void setScreenSize(float width, float height) noexcept;
    [[nodiscard]] float screenWidth() const noexcept { return m_screenWidth; }
    [[nodiscard]] float screenHeight() const noexcept { return m_screenHeight; }

    // Root element management
    void setRoot(std::unique_ptr<UIElement> root);
    [[nodiscard]] UIElement* root() const noexcept { return m_root.get(); }

    // Update and layout
    void update(float dt);
    void performLayout();

    // Input handling - returns true if event was consumed by UI
    [[nodiscard]] bool handleEvent(const SDL_Event& event);

    // Focus management
    [[nodiscard]] FocusManager& focusManager() noexcept { return m_focusManager; }
    [[nodiscard]] const FocusManager& focusManager() const noexcept { return m_focusManager; }

    // Animation management
    [[nodiscard]] AnimationManager& animationManager() noexcept { return m_animationManager; }
    [[nodiscard]] const AnimationManager& animationManager() const noexcept { return m_animationManager; }

    // Hit testing
    [[nodiscard]] UIElement* hitTest(float x, float y) const;

    // Element under mouse
    [[nodiscard]] UIElement* hoveredElement() const noexcept { return m_hoveredElement; }

    // Captured element (receives all mouse events until released)
    void setCapture(InputSurface* element) noexcept { m_capturedElement = element; }
    void releaseCapture() noexcept { m_capturedElement = nullptr; }
    [[nodiscard]] InputSurface* capturedElement() const noexcept { return m_capturedElement; }

    // Debug
    [[nodiscard]] bool isDebugDrawEnabled() const noexcept { return m_debugDraw; }
    void setDebugDraw(bool enabled) noexcept { m_debugDraw = enabled; }

    // Font manager for text measurement
    void setFontManager(rendering::FontManager* fontManager, rendering::FontID defaultFont) noexcept;

private:
    // Configure renderer-backed text measurement on the UI tree.
    void configureTextMeasurement(UIElement* element);
    // Input handling helpers
    bool handleMouseButtonDown(const SDL_Event& event);
    bool handleMouseButtonUp(const SDL_Event& event);
    bool handleMouseMotion(const SDL_Event& event);
    bool handleMouseWheel(const SDL_Event& event);
    bool handleKeyDown(const SDL_Event& event);
    bool handleKeyUp(const SDL_Event& event);
    bool handleTextInput(const SDL_Event& event);

    // Update hover state
    void updateHover(float x, float y);

    // Convert SDL mouse button to UI mouse button
    [[nodiscard]] MouseButton toMouseButton(std::uint8_t sdlButton) const noexcept;

    // Convert screen coordinates to logical coordinates
    // Maps full window to logical coordinate space
    [[nodiscard]] bool screenToLogical(float screenX, float screenY, float& logicalX, float& logicalY) const noexcept;

    // Text input management
    void updateTextInputState(FocusableControl* newFocused);

    // Helper to find open ComboBoxes in the tree
    void findOpenComboBoxes(UIElement* element, std::vector<ComboBox*>& outComboBoxes) const;

    SDL_Window* m_window{nullptr};
    rendering::FontManager* m_fontManager{nullptr};
    rendering::FontID m_defaultFont{rendering::INVALID_FONT_ID};
    std::unique_ptr<UIElement> m_root;
    FocusManager m_focusManager;
    AnimationManager m_animationManager;

    // Logical coordinate space dimensions
    float m_screenWidth{0.0f};
    float m_screenHeight{0.0f};

    // Actual window dimensions for coordinate conversion
    int m_windowWidth{0};
    int m_windowHeight{0};

    // Current mouse position
    float m_mouseX{0.0f};
    float m_mouseY{0.0f};

    // Element currently under mouse
    UIElement* m_hoveredElement{nullptr};

    // Element that has captured mouse input
    InputSurface* m_capturedElement{nullptr};

    // Previous hovered input surface (for enter/leave events)
    InputSurface* m_lastHoveredSurface{nullptr};

    bool m_debugDraw{false};
    bool m_layoutDirty{true};
    bool m_textInputActive{false};
};

} // namespace ui
