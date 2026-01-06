#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <array>
#include <bitset>

namespace input {

// Keyboard state
class Keyboard {
public:
    void update(const SDL_Event& event) noexcept;
    void clear() noexcept;

    [[nodiscard]] bool is_key_down(SDL_Scancode key) const noexcept;
    [[nodiscard]] bool is_key_up(SDL_Scancode key) const noexcept;
    [[nodiscard]] bool was_key_pressed(SDL_Scancode key) const noexcept;
    [[nodiscard]] bool was_key_released(SDL_Scancode key) const noexcept;

private:
    static constexpr std::size_t NUM_SCANCODES = 512;
    std::bitset<NUM_SCANCODES> current_state_;
    std::bitset<NUM_SCANCODES> previous_state_;
};

// Mouse state
class Mouse {
public:
    void update(const SDL_Event& event) noexcept;
    void clear() noexcept;

    [[nodiscard]] bool is_button_down(std::uint8_t button) const noexcept;
    [[nodiscard]] bool was_button_pressed(std::uint8_t button) const noexcept;
    [[nodiscard]] bool was_button_released(std::uint8_t button) const noexcept;

    [[nodiscard]] std::int32_t x() const noexcept { return x_; }
    [[nodiscard]] std::int32_t y() const noexcept { return y_; }
    [[nodiscard]] std::int32_t delta_x() const noexcept { return delta_x_; }
    [[nodiscard]] std::int32_t delta_y() const noexcept { return delta_y_; }
    [[nodiscard]] std::int32_t wheel_delta() const noexcept { return wheel_delta_; }

private:
    std::int32_t x_{0}, y_{0};
    std::int32_t delta_x_{0}, delta_y_{0};
    std::int32_t wheel_delta_{0};
    std::uint32_t current_buttons_{0};
    std::uint32_t previous_buttons_{0};
};

// Input system - polls and manages input state
class InputSystem {
public:
    InputSystem() = default;

    // Process SDL events
    void process_event(const SDL_Event& event) noexcept;

    // End of frame - swap state buffers
    void end_frame() noexcept;

    // Clear all input state
    void clear() noexcept;

    [[nodiscard]] const Keyboard& keyboard() const noexcept { return keyboard_; }
    [[nodiscard]] const Mouse& mouse() const noexcept { return mouse_; }

private:
    Keyboard keyboard_;
    Mouse mouse_;
};

} // namespace input
