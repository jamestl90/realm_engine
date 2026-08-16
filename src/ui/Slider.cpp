#include "../../include/ui/Slider.hpp"
#include <algorithm>
#include <cmath>

namespace ui {

Slider::Slider(ElementID id) noexcept
    : StyledControl(id) {
    setFocusable(true);
}

Slider::~Slider() = default;

Slider::Slider(Slider&& other) noexcept
    : StyledControl(std::move(other))
    , m_value(other.m_value)
    , m_minimum(other.m_minimum)
    , m_maximum(other.m_maximum)
    , m_step(other.m_step)
    , m_dragging(other.m_dragging)
    , m_onValueChanged(std::move(other.m_onValueChanged))
    , m_trackColour(other.m_trackColour)
    , m_fillColour(other.m_fillColour)
    , m_thumbColour(other.m_thumbColour)
    , m_hoverThumbColour(other.m_hoverThumbColour)
    , m_pressedThumbColour(other.m_pressedThumbColour)
    , m_borderColour(other.m_borderColour)
    , m_borderThickness(other.m_borderThickness)
    , m_trackHeight(other.m_trackHeight)
    , m_thumbWidth(other.m_thumbWidth)
    , m_thumbHeight(other.m_thumbHeight) {
    other.m_dragging = false;
}

Slider& Slider::operator=(Slider&& other) noexcept {
    if (this != &other) {
        StyledControl::operator=(std::move(other));
        m_value = other.m_value;
        m_minimum = other.m_minimum;
        m_maximum = other.m_maximum;
        m_step = other.m_step;
        m_dragging = other.m_dragging;
        m_onValueChanged = std::move(other.m_onValueChanged);
        m_trackColour = other.m_trackColour;
        m_fillColour = other.m_fillColour;
        m_thumbColour = other.m_thumbColour;
        m_hoverThumbColour = other.m_hoverThumbColour;
        m_pressedThumbColour = other.m_pressedThumbColour;
        m_borderColour = other.m_borderColour;
        m_borderThickness = other.m_borderThickness;
        m_trackHeight = other.m_trackHeight;
        m_thumbWidth = other.m_thumbWidth;
        m_thumbHeight = other.m_thumbHeight;
        other.m_dragging = false;
    }
    return *this;
}

float Slider::normalizedValue() const noexcept {
    const float span = m_maximum - m_minimum;
    if (span <= 0.0f) {
        return 0.0f;
    }
    return std::clamp((m_value - m_minimum) / span, 0.0f, 1.0f);
}

void Slider::setRange(float minimum, float maximum) noexcept {
    if (!std::isfinite(minimum) || !std::isfinite(maximum)) {
        return;
    }
    if (maximum < minimum) {
        std::swap(minimum, maximum);
    }
    m_minimum = minimum;
    m_maximum = maximum;
    setValue(m_value);
}

void Slider::setStep(float step) noexcept {
    m_step = std::isfinite(step) && step > 0.0f ? step : 0.0f;
    setValue(m_value);
}

void Slider::setValue(float value) noexcept {
    assignValue(value, false);
}

float Slider::valueFromLocalX(float localX) const noexcept {
    const float trackWidth = usableTrackWidth();
    if (trackWidth <= 0.0f) {
        return m_minimum;
    }

    const float trackStart = m_thumbWidth * 0.5f;
    const float ratio = std::clamp((localX - trackStart) / trackWidth, 0.0f, 1.0f);
    return clampAndSnap(m_minimum + ratio * (m_maximum - m_minimum));
}

void Slider::setTrackHeight(float height) noexcept {
    if (std::isfinite(height) && height > 0.0f) {
        m_trackHeight = height;
        invalidateLayout();
    }
}

void Slider::setThumbWidth(float width) noexcept {
    if (std::isfinite(width) && width > 0.0f) {
        m_thumbWidth = width;
        invalidateLayout();
    }
}

void Slider::setThumbHeight(float height) noexcept {
    if (std::isfinite(height) && height > 0.0f) {
        m_thumbHeight = height;
        invalidateLayout();
    }
}

Colour Slider::currentThumbColour() const noexcept {
    if (!isEnabled()) {
        return Colour{132, 138, 136, 180};
    }
    if (isPressed() || m_dragging) {
        return m_pressedThumbColour;
    }
    if (isHovered()) {
        return m_hoverThumbColour;
    }
    return m_thumbColour;
}

void Slider::measure(float availableWidth, float availableHeight) {
    const auto& constraints = sizeConstraints();

    m_measuredWidth = std::clamp(
        std::max(160.0f, constraints.preferred_width),
        constraints.min_width,
        std::min(constraints.max_width, availableWidth)
    );
    m_measuredHeight = std::clamp(
        std::max(24.0f, std::max(m_thumbHeight, constraints.preferred_height)),
        constraints.min_height,
        std::min(constraints.max_height, availableHeight)
    );
}

void Slider::cancelPointerInteraction() noexcept {
    StyledControl::cancelPointerInteraction();
    m_dragging = false;
}

void Slider::onMouseDown(MouseEventArgs& args) {
    StyledControl::onMouseDown(args);
    if (!isEnabled() || args.button != MouseButton::Left) {
        return;
    }

    m_dragging = true;
    updateFromPointer(args.localX);
    args.handled = true;
}

void Slider::onMouseMove(MouseEventArgs& args) {
    StyledControl::onMouseMove(args);
    if (!isEnabled() || !m_dragging) {
        return;
    }

    updateFromPointer(args.localX);
    args.handled = true;
}

void Slider::onMouseUp(MouseEventArgs& args) {
    const bool wasDragging = m_dragging;
    if (wasDragging) {
        updateFromPointer(args.x - bounds().x);
    }

    StyledControl::onMouseUp(args);
    m_dragging = false;

    if (wasDragging) {
        args.handled = true;
    }
}

void Slider::onActivate() {
}

float Slider::clampAndSnap(float value) const noexcept {
    if (!std::isfinite(value)) {
        return m_value;
    }

    float clamped = std::clamp(value, m_minimum, m_maximum);
    if (m_step > 0.0f && m_maximum > m_minimum) {
        clamped = m_minimum + std::round((clamped - m_minimum) / m_step) * m_step;
        clamped = std::clamp(clamped, m_minimum, m_maximum);
    }
    return clamped;
}

bool Slider::assignValue(float value, bool notify) noexcept {
    const float snapped = clampAndSnap(value);
    if (std::abs(snapped - m_value) < 0.0001f) {
        return false;
    }

    m_value = snapped;
    if (notify && m_onValueChanged) {
        m_onValueChanged(m_value);
    }
    return true;
}

void Slider::updateFromPointer(float localX) noexcept {
    assignValue(valueFromLocalX(localX), true);
}

float Slider::usableTrackWidth() const noexcept {
    return std::max(0.0f, bounds().width - m_thumbWidth);
}

} // namespace ui
