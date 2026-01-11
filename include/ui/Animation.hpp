#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <cmath>

namespace ui {

// Easing functions
namespace easing {

[[nodiscard]] inline float linear(float t) noexcept { return t; }

[[nodiscard]] inline float easeInQuad(float t) noexcept { return t * t; }
[[nodiscard]] inline float easeOutQuad(float t) noexcept { return t * (2.0f - t); }
[[nodiscard]] inline float easeInOutQuad(float t) noexcept {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

[[nodiscard]] inline float easeInCubic(float t) noexcept { return t * t * t; }
[[nodiscard]] inline float easeOutCubic(float t) noexcept {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}
[[nodiscard]] inline float easeInOutCubic(float t) noexcept {
    return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

[[nodiscard]] inline float easeInElastic(float t) noexcept {
    if (t == 0.0f || t == 1.0f) return t;
    return -std::pow(2.0f, 10.0f * (t - 1.0f)) * std::sin((t - 1.1f) * 5.0f * 3.14159265f);
}

[[nodiscard]] inline float easeOutElastic(float t) noexcept {
    if (t == 0.0f || t == 1.0f) return t;
    return std::pow(2.0f, -10.0f * t) * std::sin((t - 0.1f) * 5.0f * 3.14159265f) + 1.0f;
}

[[nodiscard]] inline float easeOutBounce(float t) noexcept {
    if (t < 1.0f / 2.75f) {
        return 7.5625f * t * t;
    } else if (t < 2.0f / 2.75f) {
        t -= 1.5f / 2.75f;
        return 7.5625f * t * t + 0.75f;
    } else if (t < 2.5f / 2.75f) {
        t -= 2.25f / 2.75f;
        return 7.5625f * t * t + 0.9375f;
    } else {
        t -= 2.625f / 2.75f;
        return 7.5625f * t * t + 0.984375f;
    }
}

} // namespace easing

// Easing function type
using EasingFunction = float(*)(float);

// Animation state
enum class AnimationState : std::uint8_t {
    Stopped,
    Running,
    Paused,
    Completed
};

// Base animation class
class Animation {
public:
    Animation() = default;
    virtual ~Animation() = default;

    Animation(const Animation&) = delete;
    Animation& operator=(const Animation&) = delete;
    Animation(Animation&&) noexcept = default;
    Animation& operator=(Animation&&) noexcept = default;

    // Control
    virtual void start();
    virtual void stop();
    virtual void pause();
    virtual void resume();
    virtual void reset();

    // Update (returns true if still running)
    [[nodiscard]] virtual bool update(float dt);

    // Configuration
    void setDuration(float seconds) noexcept { m_duration = seconds; }
    [[nodiscard]] float duration() const noexcept { return m_duration; }

    void setDelay(float seconds) noexcept { m_delay = seconds; }
    [[nodiscard]] float delay() const noexcept { return m_delay; }

    void setEasing(EasingFunction easing) noexcept { m_easing = easing; }

    void setLooping(bool loop) noexcept { m_looping = loop; }
    [[nodiscard]] bool isLooping() const noexcept { return m_looping; }

    void setAutoReverse(bool reverse) noexcept { m_autoReverse = reverse; }
    [[nodiscard]] bool isAutoReverse() const noexcept { return m_autoReverse; }

    // State
    [[nodiscard]] AnimationState state() const noexcept { return m_state; }
    [[nodiscard]] float progress() const noexcept { return m_progress; }

    // Callbacks
    using Callback = std::function<void()>;
    void setOnComplete(Callback callback) { m_onComplete = std::move(callback); }
    void setOnLoop(Callback callback) { m_onLoop = std::move(callback); }

protected:
    virtual void applyValue(float easedProgress) = 0;

    AnimationState m_state{AnimationState::Stopped};
    float m_duration{1.0f};
    float m_delay{0.0f};
    float m_elapsed{0.0f};
    float m_progress{0.0f};
    bool m_looping{false};
    bool m_autoReverse{false};
    bool m_reversed{false};
    EasingFunction m_easing{easing::linear};

    Callback m_onComplete;
    Callback m_onLoop;
};

// Float property animation
class FloatAnimation : public Animation {
public:
    using Setter = std::function<void(float)>;
    using Getter = std::function<float()>;

    FloatAnimation() = default;
    FloatAnimation(Setter setter, float from, float to);

    void setTarget(Setter setter) { m_setter = std::move(setter); }
    void setFrom(float value) noexcept { m_from = value; }
    void setTo(float value) noexcept { m_to = value; }

    [[nodiscard]] float from() const noexcept { return m_from; }
    [[nodiscard]] float to() const noexcept { return m_to; }

protected:
    void applyValue(float easedProgress) override;

private:
    Setter m_setter;
    float m_from{0.0f};
    float m_to{1.0f};
};

// Colour animation
class ColourAnimation : public Animation {
public:
    using Setter = std::function<void(std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t)>;

    ColourAnimation() = default;

    void setTarget(Setter setter) { m_setter = std::move(setter); }
    void setFrom(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) noexcept;
    void setTo(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) noexcept;

protected:
    void applyValue(float easedProgress) override;

private:
    Setter m_setter;
    std::uint8_t m_fromR{255}, m_fromG{255}, m_fromB{255}, m_fromA{255};
    std::uint8_t m_toR{255}, m_toG{255}, m_toB{255}, m_toA{255};
};

// Animation manager - updates all active animations
class AnimationManager {
public:
    AnimationManager() = default;
    ~AnimationManager() = default;

    AnimationManager(const AnimationManager&) = delete;
    AnimationManager& operator=(const AnimationManager&) = delete;

    // Add animation (takes ownership)
    Animation* add(std::unique_ptr<Animation> animation);

    // Create and add in one step
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        auto anim = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = anim.get();
        add(std::move(anim));
        return ptr;
    }

    // Update all animations
    void update(float dt);

    // Remove completed animations
    void removeCompleted();

    // Stop and remove all animations
    void clear();

    // Get animation count
    [[nodiscard]] std::size_t count() const noexcept { return m_animations.size(); }

private:
    std::vector<std::unique_ptr<Animation>> m_animations;
};

} // namespace ui
