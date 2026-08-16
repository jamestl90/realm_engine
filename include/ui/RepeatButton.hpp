#pragma once

#include "Button.hpp"
#include <functional>

namespace ui {

// Button that activates immediately, then repeats while held.
class RepeatButton : public Button {
public:
    // Return false when the action cannot make another change, such as at a bound.
    using RepeatCallback = std::function<bool()>;

    explicit RepeatButton(ElementID id = INVALID_ELEMENT_ID) noexcept;
    explicit RepeatButton(std::string text, ElementID id = INVALID_ELEMENT_ID) noexcept;
    ~RepeatButton() override;

    RepeatButton(RepeatButton&&) noexcept;
    RepeatButton& operator=(RepeatButton&&) noexcept;

    void setOnRepeat(RepeatCallback callback) { m_onRepeat = std::move(callback); }

    [[nodiscard]] float initialDelay() const noexcept { return m_initialDelay; }
    void setInitialDelay(float seconds) noexcept;

    [[nodiscard]] float repeatInterval() const noexcept { return m_repeatInterval; }
    void setRepeatInterval(float seconds) noexcept;

    void update(float dt) override;
    void cancelPointerInteraction() noexcept override;

protected:
    void onMouseDown(MouseEventArgs& args) override;
    void onMouseUp(MouseEventArgs& args) override;
    void onActivate() override;

private:
    [[nodiscard]] bool invokeRepeat();
    void stopRepeating() noexcept;

    RepeatCallback m_onRepeat;
    float m_initialDelay{0.4f};
    float m_repeatInterval{0.075f};
    float m_timeUntilRepeat{0.0f};
    bool m_repeatActive{false};
};

} // namespace ui
