#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace audio {

// Audio clip handle
using AudioClipID = std::uint32_t;
constexpr AudioClipID INVALID_AUDIO_CLIP_ID = 0;

// Audio clip resource
struct AudioClip {
    SDL_AudioSpec spec;
    std::uint8_t* buffer{nullptr};
    std::uint32_t length{0};
    AudioClipID id{INVALID_AUDIO_CLIP_ID};
};

struct PlaybackInstance;

// Audio system - manages playback via SDL3
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    // Non-copyable, movable
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    AudioSystem(AudioSystem&&) noexcept;
    AudioSystem& operator=(AudioSystem&&) noexcept;

    // Initialize audio subsystem
    [[nodiscard]] bool initialize();

    // Shutdown audio
    void shutdown() noexcept;

    // Load audio clip from file
    [[nodiscard]] AudioClipID load_clip(const char* path);

    // Unload audio clip
    void unload_clip(AudioClipID id) noexcept;

    // Play audio clip
    void play(AudioClipID id, float volume = 1.0f, bool looping = false);

    // Stop all audio
    void stop_all() noexcept;

    // Master volume control
    void set_master_volume(float volume) noexcept;
    [[nodiscard]] float master_volume() const noexcept { return master_volume_; }

private:
    SDL_AudioDeviceID device_id_{0};
    std::unordered_map<AudioClipID, std::unique_ptr<AudioClip>> clips_;
    std::unordered_map<std::string, AudioClipID> path_to_id_;
    std::vector<std::unique_ptr<PlaybackInstance>> active_streams_;
    AudioClipID next_id_{1};
    float master_volume_{1.0f};
    bool initialized_{false};
    bool owns_audio_subsystem_{false};
};

} // namespace audio
