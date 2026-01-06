#include "core/Time.hpp"

namespace core {

void Time::reset() noexcept {
    start_time_ = Clock::now();
    last_frame_time_ = start_time_;
    delta_time_ = 0.0;
    unscaled_delta_time_ = 0.0;
    total_time_ = 0.0;
    frame_count_ = 0;
}

void Time::tick() noexcept {
    const TimePoint current_time = Clock::now();
    const Duration frame_duration = current_time - last_frame_time_;
    
    unscaled_delta_time_ = frame_duration.count();
    
    if (paused_) {
        delta_time_ = 0.0;
    } else {
        delta_time_ = unscaled_delta_time_ * time_scale_;
        total_time_ += delta_time_;
    }
    
    last_frame_time_ = current_time;
    ++frame_count_;
}

} // namespace core
