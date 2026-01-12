#pragma once

#include "Style.hpp"
#include "Primitives.hpp"
#include <functional>
#include <string>

namespace ui {

// TextBox control - single-line text input
class TextBox : public StyledControl {
public:
    explicit TextBox(ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~TextBox() override;
    TextBox(TextBox&&) noexcept;
    TextBox& operator=(TextBox&&) noexcept;

    // Text content
    [[nodiscard]] const std::string& text() const noexcept { return m_text; }
    void setText(std::string text);

    // Placeholder text (shown when empty)
    [[nodiscard]] const std::string& placeholder() const noexcept { return m_placeholder; }
    void setPlaceholder(std::string placeholder) { m_placeholder = std::move(placeholder); }

    // Selection
    [[nodiscard]] std::size_t cursorPosition() const noexcept { return m_cursorPosition; }
    void setCursorPosition(std::size_t position) noexcept;

    [[nodiscard]] std::size_t selectionStart() const noexcept { return m_selectionStart; }
    [[nodiscard]] std::size_t selectionEnd() const noexcept { return m_selectionEnd; }

    void setSelection(std::size_t start, std::size_t end) noexcept;
    void clearSelection() noexcept;
    [[nodiscard]] bool hasSelection() const noexcept { return m_selectionStart != m_selectionEnd; }
    [[nodiscard]] std::string selectedText() const;

    // Max length (0 = unlimited)
    [[nodiscard]] std::size_t maxLength() const noexcept { return m_maxLength; }
    void setMaxLength(std::size_t length) noexcept { m_maxLength = length; }

    // Read-only mode
    [[nodiscard]] bool isReadOnly() const noexcept { return m_readOnly; }
    void setReadOnly(bool readOnly) noexcept { m_readOnly = readOnly; }

    // Password mode (masks characters)
    [[nodiscard]] bool isPasswordMode() const noexcept { return m_passwordMode; }
    void setPasswordMode(bool enabled) noexcept { m_passwordMode = enabled; }

    [[nodiscard]] char passwordChar() const noexcept { return m_passwordChar; }
    void setPasswordChar(char c) noexcept { m_passwordChar = c; }

    // Callbacks
    using TextChangedCallback = std::function<void(const std::string&)>;
    using SubmitCallback = std::function<void(const std::string&)>;
    using TextMeasureCallback = std::function<float(const std::string& text, float fontSize)>;

    void setOnTextChanged(TextChangedCallback callback) { m_onTextChanged = std::move(callback); }
    void setOnSubmit(SubmitCallback callback) { m_onSubmit = std::move(callback); }
    void setTextMeasurer(TextMeasureCallback callback) { m_textMeasurer = std::move(callback); }

    // Appearance
    [[nodiscard]] const Colour& backgroundColour() const noexcept { return m_backgroundColour; }
    void setBackgroundColour(const Colour& colour) noexcept { m_backgroundColour = colour; }

    [[nodiscard]] const Colour& textColour() const noexcept { return m_textColour; }
    void setTextColour(const Colour& colour) noexcept { m_textColour = colour; }

    [[nodiscard]] const Colour& placeholderColour() const noexcept { return m_placeholderColour; }
    void setPlaceholderColour(const Colour& colour) noexcept { m_placeholderColour = colour; }

    [[nodiscard]] const Colour& borderColour() const noexcept { return m_borderColour; }
    void setBorderColour(const Colour& colour) noexcept { m_borderColour = colour; }

    [[nodiscard]] const Colour& focusBorderColour() const noexcept { return m_focusBorderColour; }
    void setFocusBorderColour(const Colour& colour) noexcept { m_focusBorderColour = colour; }

    [[nodiscard]] const Colour& selectionColour() const noexcept { return m_selectionColour; }
    void setSelectionColour(const Colour& colour) noexcept { m_selectionColour = colour; }

    [[nodiscard]] const Colour& cursorColour() const noexcept { return m_cursorColour; }
    void setCursorColour(const Colour& colour) noexcept { m_cursorColour = colour; }

    [[nodiscard]] float borderThickness() const noexcept { return m_borderThickness; }
    void setBorderThickness(float thickness) noexcept { m_borderThickness = thickness; }

    [[nodiscard]] float fontSize() const noexcept { return m_fontSize; }
    void setFontSize(float size) noexcept { m_fontSize = size; invalidateLayout(); }

    [[nodiscard]] const Thickness& padding() const noexcept { return m_padding; }
    void setPadding(const Thickness& padding) noexcept { m_padding = padding; invalidateLayout(); }

    // Get current border colour based on focus state
    [[nodiscard]] Colour currentBorderColour() const noexcept;

    // Get display text (handles password masking)
    [[nodiscard]] std::string displayText() const;

    // Cursor blink state
    [[nodiscard]] bool isCursorVisible() const noexcept { return m_cursorVisible; }

    // Text input support
    [[nodiscard]] bool wantsTextInput() const noexcept override { return !m_readOnly; }

    // Layout
    void measure(float availableWidth, float availableHeight) override;
    void update(float dt) override;

protected:
    void onFocus() override;
    void onBlur() override;
    void onKeyDown(KeyEventArgs& args) override;
    void onTextInput(TextInputEventArgs& args) override;
    void onMouseDown(MouseEventArgs& args) override;

private:
    void insertText(const std::string& text);
    void deleteSelection();
    void deleteCharacter(bool forward);
    void moveCursor(std::int32_t delta, bool extendSelection);
    void moveCursorToStart(bool extendSelection);
    void moveCursorToEnd(bool extendSelection);
    void selectAll();
    void notifyTextChanged();

    std::string m_text;
    std::string m_placeholder;

    std::size_t m_cursorPosition{0};
    std::size_t m_selectionStart{0};
    std::size_t m_selectionEnd{0};
    std::size_t m_maxLength{0};

    bool m_readOnly{false};
    bool m_passwordMode{false};
    char m_passwordChar{'*'};

    // Cursor blink
    float m_cursorBlinkTimer{0.0f};
    bool m_cursorVisible{true};
    static constexpr float CURSOR_BLINK_RATE = 0.53f;

    // Callbacks
    TextChangedCallback m_onTextChanged;
    SubmitCallback m_onSubmit;
    TextMeasureCallback m_textMeasurer;

    // Appearance
    Colour m_backgroundColour{40, 40, 40, 255};
    Colour m_textColour{255, 255, 255, 255};
    Colour m_placeholderColour{128, 128, 128, 255};
    Colour m_borderColour{80, 80, 80, 255};
    Colour m_focusBorderColour{100, 150, 255, 255};
    Colour m_selectionColour{60, 120, 200, 180};
    Colour m_cursorColour{255, 255, 255, 255};
    float m_borderThickness{1.0f};
    float m_fontSize{14.0f};
    Thickness m_padding{8.0f, 6.0f, 8.0f, 6.0f};
};


} // namespace ui