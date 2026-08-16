#include "../../include/ui/RepeatButton.hpp"
#include <algorithm>
#include <cmath>

namespace ui {
namespace {

constexpr float MINIMUM_REPEAT_INTERVAL = 0.001f;

} // namespace

RepeatButton::RepeatButton(ElementID id) noexcept
    : Button(id) {
}

RepeatButton::RepeatButton(std::string text, ElementID id) noexcept
    : Button(std::move(text), id) {
}

RepeatButton::~RepeatButton() = default;

RepeatButton::RepeatButton(RepeatButton&& other) noexcept
    : Button(std::move(other))
    , m_onRepeat(std::move(other.m_onRepeat))
    , m_initialDelay(other.m_initialDelay)
    , m_repeatInterval(other.m_repeatInterval)
    , m_timeUntilRepeat(other.m_timeUntilRepeat)
    , m_repeatActive(other.m_repeatActive) {
    other.stopRepeating();
}

RepeatButton& RepeatButton::operator=(RepeatButton&& other) noexcept {
    if (this != &other) {
        Button::operator=(std::move(other));
        m_onRepeat = std::move(other.m_onRepeat);
        m_initialDelay = other.m_initialDelay;
        m_repeatInterval = other.m_repeatInterval;
        m_timeUntilRepeat = other.m_timeUntilRepeat;
        m_repeatActive = other.m_repeatActive;
        other.stopRepeating();
    }
    return *this;
}

void RepeatButton::setInitialDelay(float seconds) noexcept {
    m_initialDelay = std::isfinite(seconds) ? std::max(seconds, 0.0f) : 0.4f;
}

void RepeatButton::setRepeatInterval(float seconds) noexcept {
    m_repeatInterval = std::isfinite(seconds)
        ? std::max(seconds, MINIMUM_REPEAT_INTERVAL)
        : 0.075f;
}

void RepeatButton::update(float dt) {
    Button::update(dt);

    if (!m_repeatActive) {
        return;
    }

    if (!isEnabled() || !isPressed() || !std::isfinite(dt) || dt <= 0.0f) {
        if (!isEnabled() || !isPressed()) {
            cancelPointerInteraction();
        }
        return;
    }

    float remaining = dt;
    while (m_repeatActive && remaining >= m_timeUntilRepeat) {
        remaining -= m_timeUntilRepeat;
        if (!invokeRepeat()) {
            stopRepeating();
            return;
        }
        m_timeUntilRepeat = m_repeatInterval;
    }

    if (m_repeatActive) {
        m_timeUntilRepeat -= remaining;
    }
}

void RepeatButton::cancelPointerInteraction() noexcept {
    InputSurface::cancelPointerInteraction();
    stopRepeating();
}

void RepeatButton::onMouseDown(MouseEventArgs& args) {
    Button::onMouseDown(args);
    if (!isEnabled() || args.button != MouseButton::Left || !isPressed()) {
        return;
    }

    m_repeatActive = invokeRepeat();
    m_timeUntilRepeat = m_initialDelay;
}

void RepeatButton::onMouseUp(MouseEventArgs& args) {
    const bool wasPressed = isPressed();
    cancelPointerInteraction();
    if (wasPressed) {
        args.handled = true;
    }
}

void RepeatButton::onActivate() {
    (void)invokeRepeat();
}

bool RepeatButton::invokeRepeat() {
    return isEnabled() && m_onRepeat && m_onRepeat();
}

void RepeatButton::stopRepeating() noexcept {
    m_repeatActive = false;
    m_timeUntilRepeat = 0.0f;
}

} // namespace ui
