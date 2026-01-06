#pragma once

#include <cstdint>
#include <chrono>

namespace core {

// High-precision time management
class Time {
public:
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::duration<double>;
    using TimePoint = std::chrono::time_point<Clock>;

    Time() = default;

    // Initialize/reset time
    void reset() noexcept;

    // Update time (call once per frame)
    void tick() noexcept;

    // Time queries
    [[nodiscard]] double delta_time() const noexcept { return delta_time_; }
    [[nodiscard]] double total_time() const noexcept { return total_time_; }
    [[nodiscard]] double unscaled_delta_time() const noexcept { return unscaled_delta_time_; }
    [[nodiscard]] std::uint64_t frame_count() const noexcept { return frame_count_; }

    // Time scale control
    void set_time_scale(double scale) noexcept { time_scale_ = scale; }
    [[nodiscard]] double time_scale() const noexcept { return time_scale_; }

    // Pause control
    void set_paused(bool paused) noexcept { paused_ = paused; }
    [[nodiscard]] bool is_paused() const noexcept { return paused_; }

    // Fixed timestep support
    [[nodiscard]] double fixed_delta_time() const noexcept { return fixed_delta_time_; }
    void set_fixed_delta_time(double dt) noexcept { fixed_delta_time_ = dt; }

private:
    TimePoint start_time_{Clock::now()};
    TimePoint last_frame_time_{Clock::now()};
    
    double delta_time_{0.0};
    double unscaled_delta_time_{0.0};
    double total_time_{0.0};
    double time_scale_{1.0};
    double fixed_delta_time_{1.0 / 60.0}; // 60 Hz default
    
    std::uint64_t frame_count_{0};
    bool paused_{false};
};

} // namespace core
