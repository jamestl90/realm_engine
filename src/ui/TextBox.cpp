#include "../../include/ui/TextBox.hpp"
#include <SDL3/SDL_scancode.h>
#include <algorithm>
#include <cstring>

namespace ui {

TextBox::TextBox(ElementID id) noexcept
    : StyledControl(id) {
    setFocusable(true);
}

TextBox::~TextBox() = default;

TextBox::TextBox(TextBox&& other) noexcept
    : StyledControl(std::move(other))
    , m_text(std::move(other.m_text))
    , m_placeholder(std::move(other.m_placeholder))
    , m_cursorPosition(other.m_cursorPosition)
    , m_selectionStart(other.m_selectionStart)
    , m_selectionEnd(other.m_selectionEnd)
    , m_maxLength(other.m_maxLength)
    , m_readOnly(other.m_readOnly)
    , m_passwordMode(other.m_passwordMode)
    , m_passwordChar(other.m_passwordChar)
    , m_cursorBlinkTimer(other.m_cursorBlinkTimer)
    , m_cursorVisible(other.m_cursorVisible)
    , m_onTextChanged(std::move(other.m_onTextChanged))
    , m_onSubmit(std::move(other.m_onSubmit))
    , m_backgroundColour(other.m_backgroundColour)
    , m_textColour(other.m_textColour)
    , m_placeholderColour(other.m_placeholderColour)
    , m_borderColour(other.m_borderColour)
    , m_focusBorderColour(other.m_focusBorderColour)
    , m_selectionColour(other.m_selectionColour)
    , m_cursorColour(other.m_cursorColour)
    , m_borderThickness(other.m_borderThickness)
    , m_fontSize(other.m_fontSize)
    , m_padding(other.m_padding) {
}

TextBox& TextBox::operator=(TextBox&& other) noexcept {
    if (this != &other) {
        StyledControl::operator=(std::move(other));
        m_text = std::move(other.m_text);
        m_placeholder = std::move(other.m_placeholder);
        m_cursorPosition = other.m_cursorPosition;
        m_selectionStart = other.m_selectionStart;
        m_selectionEnd = other.m_selectionEnd;
        m_maxLength = other.m_maxLength;
        m_readOnly = other.m_readOnly;
        m_passwordMode = other.m_passwordMode;
        m_passwordChar = other.m_passwordChar;
        m_cursorBlinkTimer = other.m_cursorBlinkTimer;
        m_cursorVisible = other.m_cursorVisible;
        m_onTextChanged = std::move(other.m_onTextChanged);
        m_onSubmit = std::move(other.m_onSubmit);
        m_backgroundColour = other.m_backgroundColour;
        m_textColour = other.m_textColour;
        m_placeholderColour = other.m_placeholderColour;
        m_borderColour = other.m_borderColour;
        m_focusBorderColour = other.m_focusBorderColour;
        m_selectionColour = other.m_selectionColour;
        m_cursorColour = other.m_cursorColour;
        m_borderThickness = other.m_borderThickness;
        m_fontSize = other.m_fontSize;
        m_padding = other.m_padding;
    }
    return *this;
}

void TextBox::setText(std::string text) {
    if (m_maxLength > 0 && text.length() > m_maxLength) {
        text.resize(m_maxLength);
    }
    if (m_text != text) {
        m_text = std::move(text);
        m_cursorPosition = std::min(m_cursorPosition, m_text.length());
        clearSelection();
        notifyTextChanged();
    }
}

void TextBox::setCursorPosition(std::size_t position) noexcept {
    m_cursorPosition = std::min(position, m_text.length());
    m_cursorBlinkTimer = 0.0f;
    m_cursorVisible = true;
}

void TextBox::setSelection(std::size_t start, std::size_t end) noexcept {
    m_selectionStart = std::min(start, m_text.length());
    m_selectionEnd = std::min(end, m_text.length());
}

void TextBox::clearSelection() noexcept {
    m_selectionStart = m_cursorPosition;
    m_selectionEnd = m_cursorPosition;
}

std::string TextBox::selectedText() const {
    if (!hasSelection()) {
        return {};
    }

    std::size_t start = std::min(m_selectionStart, m_selectionEnd);
    std::size_t end = std::max(m_selectionStart, m_selectionEnd);
    return m_text.substr(start, end - start);
}

Colour TextBox::currentBorderColour() const noexcept {
    if (!isEnabled()) {
        return Colour{60, 60, 60, 128};
    }
    if (isFocused()) {
        return m_focusBorderColour;
    }
    return m_borderColour;
}

std::string TextBox::displayText() const {
    if (m_passwordMode) {
        return std::string(m_text.length(), m_passwordChar);
    }
    return m_text;
}

void TextBox::measure(float availableWidth, float availableHeight) {
    // Default size for text box
    m_measuredWidth = 200.0f;
    m_measuredHeight = m_fontSize + m_padding.verticalSum();

    // Apply constraints
    const auto& constraints = sizeConstraints();
    if (constraints.preferred_width > 0) {
        m_measuredWidth = constraints.preferred_width;
    }
    m_measuredWidth = std::clamp(m_measuredWidth, constraints.min_width,
        std::min(constraints.max_width, availableWidth));
    m_measuredHeight = std::clamp(m_measuredHeight, constraints.min_height,
        std::min(constraints.max_height, availableHeight));

    // Ensure minimum height
    m_measuredHeight = std::max(m_measuredHeight, 24.0f);
}

void TextBox::update(float dt) {
    StyledControl::update(dt);

    // Cursor blink
    if (isFocused()) {
        m_cursorBlinkTimer += dt;
        if (m_cursorBlinkTimer >= CURSOR_BLINK_RATE) {
            m_cursorBlinkTimer = 0.0f;
            m_cursorVisible = !m_cursorVisible;
        }
    }
}

void TextBox::onFocus() {
    StyledControl::onFocus();
    m_cursorBlinkTimer = 0.0f;
    m_cursorVisible = true;
}

void TextBox::onBlur() {
    StyledControl::onBlur();
    clearSelection();
}

void TextBox::onKeyDown(KeyEventArgs& args) {
    StyledControl::onKeyDown(args);

    if (!isFocused()) {
        return;
    }

    bool shift = args.shift;
    bool ctrl = args.ctrl;

    switch (args.scancode) {
        case SDL_SCANCODE_LEFT:
            if (ctrl) {
                // Move to previous word (simplified: just move one character)
                moveCursor(-1, shift);
            } else {
                moveCursor(-1, shift);
            }
            args.handled = true;
            break;
        case SDL_SCANCODE_RIGHT:
            if (ctrl) {
                moveCursor(1, shift);
            } else {
                moveCursor(1, shift);
            }
            args.handled = true;
            break;
        case SDL_SCANCODE_HOME:
            moveCursorToStart(shift);
            args.handled = true;
            break;
        case SDL_SCANCODE_END:
            moveCursorToEnd(shift);
            args.handled = true;
            break;
        case SDL_SCANCODE_BACKSPACE:
            if (!m_readOnly) {
                if (hasSelection()) {
                    deleteSelection();
                } else {
                    deleteCharacter(false);
                }
            }
            args.handled = true;
            break;
        case SDL_SCANCODE_DELETE:
            if (!m_readOnly) {
                if (hasSelection()) {
                    deleteSelection();
                } else {
                    deleteCharacter(true);
                }
            }
            args.handled = true;
            break;
        case SDL_SCANCODE_A:
            if (ctrl) {
                selectAll();
                args.handled = true;
            }
            break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            if (m_onSubmit) {
                m_onSubmit(m_text);
            }
            args.handled = true;
            break;
        default:
            break;
    }

    // Reset cursor blink on any key press
    m_cursorBlinkTimer = 0.0f;
    m_cursorVisible = true;
}

void TextBox::onTextInput(TextInputEventArgs& args) {
    StyledControl::onTextInput(args);

    if (!isFocused() || m_readOnly) {
        return;
    }

    insertText(args.text);
    args.handled = true;
}

void TextBox::onMouseDown(MouseEventArgs& args) {
    StyledControl::onMouseDown(args);

    // Calculate cursor position from click
    float localX = args.x - m_bounds.x - m_padding.left;
    float charWidth = m_fontSize * 0.6f;

    std::size_t clickPos = 0;
    if (localX > 0 && charWidth > 0) {
        clickPos = static_cast<std::size_t>(localX / charWidth);
    }

    clickPos = std::min(clickPos, m_text.length());
    setCursorPosition(clickPos);
    clearSelection();
    args.handled = true;
}

void TextBox::insertText(const std::string& text) {
    if (text.empty()) {
        return;
    }

    // Delete selection first if any
    if (hasSelection()) {
        deleteSelection();
    }

    // Check max length
    std::string toInsert = text;
    if (m_maxLength > 0) {
        std::size_t available = m_maxLength - m_text.length();
        if (toInsert.length() > available) {
            toInsert.resize(available);
        }
    }

    if (toInsert.empty()) {
        return;
    }

    m_text.insert(m_cursorPosition, toInsert);
    m_cursorPosition += toInsert.length();
    clearSelection();
    notifyTextChanged();
}

void TextBox::deleteSelection() {
    if (!hasSelection()) {
        return;
    }

    std::size_t start = std::min(m_selectionStart, m_selectionEnd);
    std::size_t end = std::max(m_selectionStart, m_selectionEnd);

    m_text.erase(start, end - start);
    m_cursorPosition = start;
    clearSelection();
    notifyTextChanged();
}

void TextBox::deleteCharacter(bool forward) {
    if (forward) {
        if (m_cursorPosition < m_text.length()) {
            m_text.erase(m_cursorPosition, 1);
            notifyTextChanged();
        }
    } else {
        if (m_cursorPosition > 0) {
            --m_cursorPosition;
            m_text.erase(m_cursorPosition, 1);
            notifyTextChanged();
        }
    }
}

void TextBox::moveCursor(std::int32_t delta, bool extendSelection) {
    std::size_t newPos = m_cursorPosition;

    if (delta < 0) {
        std::size_t absDelta = static_cast<std::size_t>(-delta);
        newPos = (m_cursorPosition > absDelta) ? m_cursorPosition - absDelta : 0;
    } else {
        newPos = std::min(m_cursorPosition + static_cast<std::size_t>(delta), m_text.length());
    }

    if (extendSelection) {
        if (!hasSelection()) {
            m_selectionStart = m_cursorPosition;
        }
        m_selectionEnd = newPos;
    } else {
        clearSelection();
    }

    m_cursorPosition = newPos;
    m_cursorBlinkTimer = 0.0f;
    m_cursorVisible = true;
}

void TextBox::moveCursorToStart(bool extendSelection) {
    if (extendSelection) {
        if (!hasSelection()) {
            m_selectionStart = m_cursorPosition;
        }
        m_selectionEnd = 0;
    } else {
        clearSelection();
    }
    m_cursorPosition = 0;
    m_cursorBlinkTimer = 0.0f;
    m_cursorVisible = true;
}

void TextBox::moveCursorToEnd(bool extendSelection) {
    if (extendSelection) {
        if (!hasSelection()) {
            m_selectionStart = m_cursorPosition;
        }
        m_selectionEnd = m_text.length();
    } else {
        clearSelection();
    }
    m_cursorPosition = m_text.length();
    m_cursorBlinkTimer = 0.0f;
    m_cursorVisible = true;
}

void TextBox::selectAll() {
    m_selectionStart = 0;
    m_selectionEnd = m_text.length();
    m_cursorPosition = m_text.length();
}

void TextBox::notifyTextChanged() {
    if (m_onTextChanged) {
        m_onTextChanged(m_text);
    }
}

} // namespace ui