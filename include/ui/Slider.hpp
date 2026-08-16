#pragma once

#include "Style.hpp"
#include "Primitives.hpp"
#include <functional>

namespace ui {

class Slider : public StyledControl {
public:
    using ValueChangedCallback = std::function<void(float)>;

    explicit Slider(ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~Slider() override;

    Slider(Slider&&) noexcept;
    Slider& operator=(Slider&&) noexcept;

    [[nodiscard]] float value() const noexcept { return m_value; }
    [[nodiscard]] float minimum() const noexcept { return m_minimum; }
    [[nodiscard]] float maximum() const noexcept { return m_maximum; }
    [[nodiscard]] float step() const noexcept { return m_step; }
    [[nodiscard]] float normalizedValue() const noexcept;

    void setRange(float minimum, float maximum) noexcept;
    void setStep(float step) noexcept;
    void setValue(float value) noexcept;
    [[nodiscard]] float valueFromLocalX(float localX) const noexcept;

    void setOnValueChanged(ValueChangedCallback callback) { m_onValueChanged = std::move(callback); }

    [[nodiscard]] const Colour& trackColour() const noexcept { return m_trackColour; }
    void setTrackColour(const Colour& colour) noexcept { m_trackColour = colour; }

    [[nodiscard]] const Colour& fillColour() const noexcept { return m_fillColour; }
    void setFillColour(const Colour& colour) noexcept { m_fillColour = colour; }

    [[nodiscard]] const Colour& thumbColour() const noexcept { return m_thumbColour; }
    void setThumbColour(const Colour& colour) noexcept { m_thumbColour = colour; }

    [[nodiscard]] const Colour& hoverThumbColour() const noexcept { return m_hoverThumbColour; }
    void setHoverThumbColour(const Colour& colour) noexcept { m_hoverThumbColour = colour; }

    [[nodiscard]] const Colour& pressedThumbColour() const noexcept { return m_pressedThumbColour; }
    void setPressedThumbColour(const Colour& colour) noexcept { m_pressedThumbColour = colour; }

    [[nodiscard]] const Colour& borderColour() const noexcept { return m_borderColour; }
    void setBorderColour(const Colour& colour) noexcept { m_borderColour = colour; }

    [[nodiscard]] float borderThickness() const noexcept { return m_borderThickness; }
    void setBorderThickness(float thickness) noexcept { m_borderThickness = thickness; }

    [[nodiscard]] float trackHeight() const noexcept { return m_trackHeight; }
    void setTrackHeight(float height) noexcept;

    [[nodiscard]] float thumbWidth() const noexcept { return m_thumbWidth; }
    void setThumbWidth(float width) noexcept;

    [[nodiscard]] float thumbHeight() const noexcept { return m_thumbHeight; }
    void setThumbHeight(float height) noexcept;

    [[nodiscard]] Colour currentThumbColour() const noexcept;

    void measure(float availableWidth, float availableHeight) override;
    void cancelPointerInteraction() noexcept override;

protected:
    void onMouseDown(MouseEventArgs& args) override;
    void onMouseMove(MouseEventArgs& args) override;
    void onMouseUp(MouseEventArgs& args) override;
    void onActivate() override;

private:
    [[nodiscard]] float clampAndSnap(float value) const noexcept;
    bool assignValue(float value, bool notify) noexcept;
    void updateFromPointer(float localX) noexcept;
    [[nodiscard]] float usableTrackWidth() const noexcept;

    float m_value{0.0f};
    float m_minimum{0.0f};
    float m_maximum{1.0f};
    float m_step{0.01f};
    bool m_dragging{false};
    ValueChangedCallback m_onValueChanged;

    Colour m_trackColour{178, 188, 184, 255};
    Colour m_fillColour{46, 112, 86, 255};
    Colour m_thumbColour{250, 252, 250, 255};
    Colour m_hoverThumbColour{236, 244, 240, 255};
    Colour m_pressedThumbColour{214, 230, 220, 255};
    Colour m_borderColour{70, 86, 86, 255};
    float m_borderThickness{1.0f};
    float m_trackHeight{6.0f};
    float m_thumbWidth{12.0f};
    float m_thumbHeight{22.0f};
};

} // namespace ui
