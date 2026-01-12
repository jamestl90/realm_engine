#pragma once

#include "InputSurface.hpp"

namespace ui {

// Focus navigation direction
enum class FocusDirection : std::uint8_t {
    Next,
    Previous,
    Up,
    Down,
    Left,
    Right
};

// Base class for controls that can receive keyboard focus
class FocusableControl : public InputSurface {
public:
    explicit FocusableControl(ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~FocusableControl() override;

    FocusableControl(FocusableControl&&) noexcept;
    FocusableControl& operator=(FocusableControl&&) noexcept;

    // Focus state
    [[nodiscard]] bool isFocused() const noexcept { return m_focused; }
    [[nodiscard]] bool isFocusable() const noexcept { return m_focusable && isEnabled(); }
    void setFocusable(bool focusable) noexcept { m_focusable = focusable; }

    // Tab order for keyboard navigation
    [[nodiscard]] std::int32_t tabIndex() const noexcept { return m_tabIndex; }
    void setTabIndex(std::int32_t index) noexcept { m_tabIndex = index; }

    // Focus events (called by focus manager)
    virtual void onFocus();
    virtual void onBlur();

    // Focus callbacks
    using FocusCallback = std::function<void()>;
    void setOnFocus(FocusCallback callback) { m_onFocusCallback = std::move(callback); }
    void setOnBlur(FocusCallback callback) { m_onBlurCallback = std::move(callback); }

    // Text input support - override in controls that accept text input
    [[nodiscard]] virtual bool wantsTextInput() const noexcept { return false; }

    // Input handling with focus awareness
    void onMouseDown(MouseEventArgs& args) override;
    void onKeyDown(KeyEventArgs& args) override;

protected:
    // Called when Enter/Space is pressed while focused
    virtual void onActivate();

    // For focus manager to set focus state
    friend class FocusManager;
    void setFocused(bool focused) noexcept;

private:
    bool m_focused{false};
    bool m_focusable{true};
    std::int32_t m_tabIndex{0};

    FocusCallback m_onFocusCallback;
    FocusCallback m_onBlurCallback;
};

// Focus manager - tracks and manages keyboard focus
class FocusManager {
public:
    FocusManager() = default;
    ~FocusManager() = default;

    // Non-copyable
    FocusManager(const FocusManager&) = delete;
    FocusManager& operator=(const FocusManager&) = delete;

    // Current focus
    [[nodiscard]] FocusableControl* focusedElement() const noexcept { return m_focused; }

    // Focus operations
    void setFocus(FocusableControl* control);
    void clearFocus();

    // Navigation
    void moveFocus(FocusDirection direction, UIElement* root);

    // Register/unregister focusable controls
    void registerControl(FocusableControl* control);
    void unregisterControl(FocusableControl* control);

private:
    FocusableControl* m_focused{nullptr};
    std::vector<FocusableControl*> m_focusableControls;

    void collectFocusableControls(UIElement* root, std::vector<FocusableControl*>& controls);
    FocusableControl* findNextFocusable(FocusDirection direction);
};

} // namespace ui
