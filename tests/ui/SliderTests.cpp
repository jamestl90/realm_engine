#include "ui/Slider.hpp"
#include <cmath>
#include <iostream>

#if !defined(REALM_TEST_BUILD)
#error "SliderTests.cpp must only be compiled for test builds"
#endif

namespace {

class SliderHarness : public ui::Slider {
public:
    using ui::Slider::onMouseDown;
    using ui::Slider::onMouseMove;
    using ui::Slider::onMouseUp;
};

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool nearly_equal(float left, float right) {
    return std::abs(left - right) < 0.0001f;
}

ui::MouseEventArgs left_mouse_event(float x, float y = 0.0f) {
    ui::MouseEventArgs args;
    args.x = x;
    args.y = y;
    args.button = ui::MouseButton::Left;
    return args;
}

SliderHarness make_test_slider() {
    SliderHarness slider;
    slider.setRange(0.0f, 100.0f);
    slider.setStep(25.0f);
    slider.setThumbWidth(10.0f);
    slider.setBounds(ui::Rect{10.0f, 20.0f, 110.0f, 26.0f});
    return slider;
}

bool test_value_clamps_and_snaps_to_step() {
    SliderHarness slider;
    slider.setRange(10.0f, 20.0f);
    slider.setStep(2.0f);

    slider.setValue(15.1f);
    bool ok = require(nearly_equal(slider.value(), 16.0f), "value snaps to nearest configured step");

    slider.setValue(-5.0f);
    ok &= require(nearly_equal(slider.value(), 10.0f), "value clamps to minimum");

    slider.setValue(100.0f);
    return ok && require(nearly_equal(slider.value(), 20.0f), "value clamps to maximum");
}

bool test_local_position_maps_to_value_range() {
    SliderHarness slider = make_test_slider();

    bool ok = require(nearly_equal(slider.valueFromLocalX(5.0f), 0.0f), "track start maps to minimum");
    ok &= require(nearly_equal(slider.valueFromLocalX(55.0f), 50.0f), "track midpoint maps to midpoint value");
    ok &= require(nearly_equal(slider.valueFromLocalX(105.0f), 100.0f), "track end maps to maximum");
    ok &= require(nearly_equal(slider.valueFromLocalX(-20.0f), 0.0f), "positions before track clamp low");
    return ok && require(nearly_equal(slider.valueFromLocalX(300.0f), 100.0f), "positions after track clamp high");
}

bool test_dragging_emits_callbacks() {
    SliderHarness slider = make_test_slider();
    int callbacks = 0;
    float lastValue = -1.0f;
    slider.setOnValueChanged([&callbacks, &lastValue](float value) {
        ++callbacks;
        lastValue = value;
    });

    auto down = left_mouse_event(65.0f, 30.0f);
    slider.onMouseDown(down);
    bool ok = require(down.handled, "mouse down is handled");
    ok &= require(callbacks == 1, "mouse down on a new value emits callback");
    ok &= require(nearly_equal(lastValue, 50.0f), "mouse down callback reports mapped value");

    auto move = left_mouse_event(90.0f, 30.0f);
    slider.onMouseMove(move);
    ok &= require(move.handled, "drag move is handled");
    ok &= require(callbacks == 2, "drag move emits callback when value changes");
    ok &= require(nearly_equal(lastValue, 75.0f), "drag move callback reports snapped value");

    auto up = left_mouse_event(115.0f, 30.0f);
    slider.onMouseUp(up);
    ok &= require(up.handled, "mouse up completes active drag");
    return ok && require(nearly_equal(slider.value(), 100.0f), "mouse up applies final pointer value");
}

bool test_pointer_cancellation_stops_dragging() {
    SliderHarness slider = make_test_slider();
    int callbacks = 0;
    slider.setOnValueChanged([&callbacks](float) {
        ++callbacks;
    });

    auto down = left_mouse_event(65.0f, 30.0f);
    slider.onMouseDown(down);
    slider.cancelPointerInteraction();

    auto move = left_mouse_event(115.0f, 30.0f);
    slider.onMouseMove(move);
    return require(callbacks == 1, "cancelled slider ignores later drag motion")
        && require(nearly_equal(slider.value(), 50.0f), "cancelled slider preserves last committed value");
}

bool test_disabled_slider_ignores_pointer_input() {
    SliderHarness slider = make_test_slider();
    slider.setEnabled(false);
    int callbacks = 0;
    slider.setOnValueChanged([&callbacks](float) {
        ++callbacks;
    });

    auto down = left_mouse_event(115.0f, 30.0f);
    slider.onMouseDown(down);
    return require(callbacks == 0, "disabled slider does not emit callbacks")
        && require(nearly_equal(slider.value(), 0.0f), "disabled slider value is unchanged");
}

} // namespace

int main() {
    bool ok = true;
    ok &= test_value_clamps_and_snaps_to_step();
    ok &= test_local_position_maps_to_value_range();
    ok &= test_dragging_emits_callbacks();
    ok &= test_pointer_cancellation_stops_dragging();
    ok &= test_disabled_slider_ignores_pointer_input();

    if (!ok) {
        return 1;
    }

    std::cout << "UI slider tests passed.\n";
    return 0;
}
