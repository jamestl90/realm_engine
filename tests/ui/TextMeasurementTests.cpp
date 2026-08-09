#include "ui/Button.hpp"
#include "ui/ComboBox.hpp"
#include "ui/Primitives.hpp"
#include "ui/TextBox.hpp"
#include <cmath>
#include <iostream>

namespace {

bool nearly_equal(float left, float right) {
    return std::abs(left - right) < 0.01f;
}

bool require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

bool test_text_block_uses_font_metrics() {
    ui::TextBlock text("Measured text");
    float requested_size = 0.0f;
    text.setFontSize(23.0f);
    text.setTextMeasurer([&requested_size](const std::string&, float fontSize) {
        requested_size = fontSize;
        return ui::TextMetrics{137.0f, 29.0f};
    });

    text.measure(500.0f, 500.0f);
    return require(nearly_equal(requested_size, 23.0f), "TextBlock requests its configured font size")
        && require(nearly_equal(text.measuredWidth(), 137.0f), "TextBlock uses measured text width")
        && require(nearly_equal(text.measuredHeight(), 29.0f), "TextBlock uses measured text height");
}

bool test_button_includes_measured_text_and_padding() {
    ui::Button button("Wide label");
    button.setFontSize(18.0f);
    button.setPadding(ui::Thickness(10.0f, 6.0f));
    button.setTextMeasurer([](const std::string&, float) {
        return ui::TextMetrics{164.0f, 21.0f};
    });

    button.measure(500.0f, 500.0f);
    return require(nearly_equal(button.measuredWidth(), 184.0f), "Button width includes measured label and padding")
        && require(nearly_equal(button.measuredHeight(), 33.0f), "Button height includes measured label and padding");
}

bool test_text_controls_use_measured_line_height() {
    ui::TextBox textBox;
    textBox.setFontSize(17.0f);
    textBox.setTextMeasurer([](const std::string&, float) {
        return ui::TextMetrics{40.0f, 25.0f};
    });
    textBox.measure(500.0f, 500.0f);

    ui::ComboBox comboBox;
    comboBox.setPlaceholder("Short");
    comboBox.addItem("Longest item");
    comboBox.setTextMeasurer([](const std::string& text, float) {
        return ui::TextMetrics{
            text == "Longest item" ? 180.0f : 60.0f,
            25.0f
        };
    });
    comboBox.measure(500.0f, 500.0f);

    return require(nearly_equal(textBox.measuredHeight(), 37.0f), "TextBox height uses measured line height")
        && require(nearly_equal(comboBox.measuredWidth(), 216.0f), "ComboBox width uses its longest measured item");
}

} // namespace

int main() {
    bool ok = true;
    ok &= test_text_block_uses_font_metrics();
    ok &= test_button_includes_measured_text_and_padding();
    ok &= test_text_controls_use_measured_line_height();

    if (!ok) {
        return 1;
    }

    std::cout << "UI text measurement tests passed.\n";
    return 0;
}
