#include "ui/RepeatButton.hpp"
#include <iostream>

#if !defined(REALM_TEST_BUILD)
#error "RepeatButtonTests.cpp must only be compiled for test builds"
#endif

namespace {

class RepeatButtonHarness : public ui::RepeatButton {
public:
    using ui::RepeatButton::RepeatButton;
    using ui::RepeatButton::onMouseDown;
    using ui::RepeatButton::onMouseUp;
};

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

ui::MouseEventArgs left_mouse_event() {
    ui::MouseEventArgs args;
    args.button = ui::MouseButton::Left;
    return args;
}

void press(RepeatButtonHarness& button) {
    button.onMouseEnter();
    auto args = left_mouse_event();
    button.onMouseDown(args);
}

void release(RepeatButtonHarness& button) {
    auto args = left_mouse_event();
    button.onMouseUp(args);
}

bool test_immediate_activation_and_initial_delay() {
    RepeatButtonHarness button("+");
    button.setInitialDelay(0.4f);
    button.setRepeatInterval(0.1f);

    int activations = 0;
    button.setOnRepeat([&activations]() {
        ++activations;
        return true;
    });

    press(button);
    bool ok = require(activations == 1, "press activates immediately");
    button.update(0.39f);
    ok &= require(activations == 1, "holding does not repeat before the initial delay");
    button.update(0.02f);
    ok &= require(activations == 2, "holding repeats after the initial delay");
    release(button);
    button.update(1.0f);
    return ok && require(activations == 2, "release stops repeating");
}

bool test_repeat_cadence_handles_coarse_frames() {
    RepeatButtonHarness button("+");
    button.setInitialDelay(0.25f);
    button.setRepeatInterval(0.1f);

    int activations = 0;
    button.setOnRepeat([&activations]() {
        ++activations;
        return true;
    });

    press(button);
    button.update(0.56f);
    release(button);
    return require(
        activations == 5,
        "a coarse frame preserves every elapsed repeat interval"
    );
}

bool test_short_press_and_pointer_cancellation() {
    RepeatButtonHarness button("-");
    int activations = 0;
    button.setOnRepeat([&activations]() {
        ++activations;
        return true;
    });

    press(button);
    release(button);
    button.update(1.0f);
    bool ok = require(activations == 1, "a short press activates exactly once");

    press(button);
    button.cancelPointerInteraction();
    button.update(1.0f);
    ok &= require(activations == 2, "pointer cancellation stops a held repeat");

    press(button);
    button.onMouseLeave();
    button.update(1.0f);
    return ok && require(activations == 3, "pointer leave cancels a held repeat");
}

bool test_disabled_state_and_bounds_stop_repeating() {
    RepeatButtonHarness disabledButton("+");
    int disabledActivations = 0;
    disabledButton.setOnRepeat([&disabledActivations]() {
        ++disabledActivations;
        return true;
    });
    disabledButton.setEnabled(false);
    press(disabledButton);
    disabledButton.update(1.0f);

    bool ok = require(disabledActivations == 0, "disabled repeat buttons do not activate");

    RepeatButtonHarness boundedButton("+");
    boundedButton.setInitialDelay(0.2f);
    boundedButton.setRepeatInterval(0.05f);
    int value = 1;
    int valueChanges = 0;
    boundedButton.setOnRepeat([&value, &valueChanges]() {
        constexpr int maximum = 2;
        if (value >= maximum) {
            return false;
        }
        ++value;
        ++valueChanges;
        return true;
    });

    press(boundedButton);
    boundedButton.update(1.0f);
    boundedButton.update(1.0f);
    release(boundedButton);
    return ok
        && require(value == 2, "repeat actions respect their value bound")
        && require(valueChanges == 1, "no value changes are emitted after reaching a bound");
}

bool test_invalid_timing_values_are_sanitized() {
    RepeatButtonHarness button("+");
    button.setInitialDelay(-1.0f);
    button.setRepeatInterval(0.0f);
    return require(button.initialDelay() == 0.0f, "negative initial delay is clamped")
        && require(button.repeatInterval() > 0.0f, "repeat interval remains positive");
}

} // namespace

int main() {
    bool ok = true;
    ok &= test_immediate_activation_and_initial_delay();
    ok &= test_repeat_cadence_handles_coarse_frames();
    ok &= test_short_press_and_pointer_cancellation();
    ok &= test_disabled_state_and_bounds_stop_repeating();
    ok &= test_invalid_timing_values_are_sanitized();

    if (!ok) {
        return 1;
    }

    std::cout << "UI repeat button tests passed.\n";
    return 0;
}
