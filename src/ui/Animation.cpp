#include "../../include/ui/Animation.hpp"
#include <algorithm>

namespace ui {

// Animation base implementation
void Animation::start() {
    m_state = AnimationState::Running;
    m_elapsed = -m_delay;
    m_progress = 0.0f;
    m_reversed = false;
}

void Animation::stop() {
    m_state = AnimationState::Stopped;
    m_elapsed = 0.0f;
    m_progress = 0.0f;
}

void Animation::pause() {
    if (m_state == AnimationState::Running) {
        m_state = AnimationState::Paused;
    }
}

void Animation::resume() {
    if (m_state == AnimationState::Paused) {
        m_state = AnimationState::Running;
    }
}

void Animation::reset() {
    m_elapsed = -m_delay;
    m_progress = 0.0f;
    m_reversed = false;
}

bool Animation::update(float dt) {
    if (m_state != AnimationState::Running) {
        return m_state != AnimationState::Stopped && m_state != AnimationState::Completed;
    }

    m_elapsed += dt;

    // Still in delay period
    if (m_elapsed < 0.0f) {
        return true;
    }

    // Calculate progress
    if (m_duration > 0.0f) {
        m_progress = std::clamp(m_elapsed / m_duration, 0.0f, 1.0f);
    } else {
        m_progress = 1.0f;
    }

    // Handle auto-reverse
    float effectiveProgress = m_progress;
    if (m_autoReverse && m_reversed) {
        effectiveProgress = 1.0f - m_progress;
    }

    // Apply easing
    float easedProgress = m_easing ? m_easing(effectiveProgress) : effectiveProgress;

    // Apply the animated value
    applyValue(easedProgress);

    // Check for completion
    if (m_progress >= 1.0f) {
        if (m_autoReverse && !m_reversed) {
            // Start reverse
            m_reversed = true;
            m_elapsed = 0.0f;
            m_progress = 0.0f;
            return true;
        }

        if (m_looping) {
            // Loop
            m_elapsed = 0.0f;
            m_progress = 0.0f;
            m_reversed = false;
            if (m_onLoop) {
                m_onLoop();
            }
            return true;
        }

        // Complete
        m_state = AnimationState::Completed;
        if (m_onComplete) {
            m_onComplete();
        }
        return false;
    }

    return true;
}

// FloatAnimation implementation
FloatAnimation::FloatAnimation(Setter setter, float from, float to)
    : m_setter(std::move(setter))
    , m_from(from)
    , m_to(to) {
}

void FloatAnimation::applyValue(float easedProgress) {
    if (m_setter) {
        float value = m_from + (m_to - m_from) * easedProgress;
        m_setter(value);
    }
}

// ColourAnimation implementation
void ColourAnimation::setFrom(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) noexcept {
    m_fromR = r;
    m_fromG = g;
    m_fromB = b;
    m_fromA = a;
}

void ColourAnimation::setTo(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) noexcept {
    m_toR = r;
    m_toG = g;
    m_toB = b;
    m_toA = a;
}

void ColourAnimation::applyValue(float easedProgress) {
    if (m_setter) {
        auto lerp = [](std::uint8_t a, std::uint8_t b, float t) -> std::uint8_t {
            return static_cast<std::uint8_t>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t);
        };

        m_setter(
            lerp(m_fromR, m_toR, easedProgress),
            lerp(m_fromG, m_toG, easedProgress),
            lerp(m_fromB, m_toB, easedProgress),
            lerp(m_fromA, m_toA, easedProgress)
        );
    }
}

// AnimationManager implementation
Animation* AnimationManager::add(std::unique_ptr<Animation> animation) {
    if (!animation) {
        return nullptr;
    }
    Animation* ptr = animation.get();
    m_animations.push_back(std::move(animation));
    return ptr;
}

void AnimationManager::update(float dt) {
    for (auto& anim : m_animations) {
        if (anim) {
            anim->update(dt);
        }
    }
}

void AnimationManager::removeCompleted() {
    m_animations.erase(
        std::remove_if(m_animations.begin(), m_animations.end(),
            [](const std::unique_ptr<Animation>& anim) {
                return !anim || anim->state() == AnimationState::Completed;
            }),
        m_animations.end()
    );
}

void AnimationManager::clear() {
    for (auto& anim : m_animations) {
        if (anim) {
            anim->stop();
        }
    }
    m_animations.clear();
}

} // namespace ui
