#include "../../include/audio/AudioSystem.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace audio {
namespace {

bool queue_audio_data(SDL_AudioStream* stream, const std::uint8_t* buffer, std::uint32_t length) {
    if (!stream || !buffer || length == 0) {
        return false;
    }

    std::uint32_t queued = 0;
    while (queued < length) {
        const auto remaining = length - queued;
        const auto chunk = static_cast<int>(std::min<std::uint32_t>(
            remaining,
            static_cast<std::uint32_t>(std::numeric_limits<int>::max())
        ));
        if (!SDL_PutAudioStreamData(stream, buffer + queued, chunk)) {
            return false;
        }
        queued += static_cast<std::uint32_t>(chunk);
    }
    return true;
}

} // namespace

struct PlaybackInstance {
    SDL_AudioStream* stream{nullptr};
    AudioClipID clip_id{INVALID_AUDIO_CLIP_ID};
    AudioClip* clip{nullptr};
    float volume{1.0f};
    bool looping{false};

    ~PlaybackInstance() {
        if (stream) {
            SDL_DestroyAudioStream(stream);
        }
    }
};

namespace {

void SDLCALL refill_looping_stream(
    void* userdata,
    SDL_AudioStream* stream,
    int additional_amount,
    int total_amount
) {
    auto* instance = static_cast<PlaybackInstance*>(userdata);
    if (!instance || !instance->clip || !instance->clip->buffer || instance->clip->length == 0) {
        return;
    }

    int queued_amount = total_amount;
    while (queued_amount < additional_amount) {
        if (!queue_audio_data(stream, instance->clip->buffer, instance->clip->length)) {
            return;
        }
        if (instance->clip->length > static_cast<std::uint32_t>(
                std::numeric_limits<int>::max() - queued_amount
            )) {
            queued_amount = additional_amount;
        } else {
            queued_amount += static_cast<int>(instance->clip->length);
        }
    }
}

} // namespace

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem() {
    shutdown();
}

AudioSystem::AudioSystem(AudioSystem&& other) noexcept
    : device_id_(std::exchange(other.device_id_, 0))
    , clips_(std::move(other.clips_))
    , path_to_id_(std::move(other.path_to_id_))
    , active_streams_(std::move(other.active_streams_))
    , next_id_(std::exchange(other.next_id_, 1))
    , master_volume_(std::exchange(other.master_volume_, 1.0f))
    , initialized_(std::exchange(other.initialized_, false))
    , owns_audio_subsystem_(std::exchange(other.owns_audio_subsystem_, false)) {
}

AudioSystem& AudioSystem::operator=(AudioSystem&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    shutdown();
    device_id_ = std::exchange(other.device_id_, 0);
    clips_ = std::move(other.clips_);
    path_to_id_ = std::move(other.path_to_id_);
    active_streams_ = std::move(other.active_streams_);
    next_id_ = std::exchange(other.next_id_, 1);
    master_volume_ = std::exchange(other.master_volume_, 1.0f);
    initialized_ = std::exchange(other.initialized_, false);
    owns_audio_subsystem_ = std::exchange(other.owns_audio_subsystem_, false);
    return *this;
}

bool AudioSystem::initialize() {
    if (initialized_) {
        return true;
    }

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to initialize SDL audio: %s", SDL_GetError());
        return false;
    }
    owns_audio_subsystem_ = true;

    device_id_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (device_id_ == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open default audio device: %s", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        owns_audio_subsystem_ = false;
        return false;
    }

    initialized_ = true;
    return true;
}

void AudioSystem::shutdown() noexcept {
    stop_all();

    for (auto& [id, clip] : clips_) {
        (void)id;
        if (clip && clip->buffer) {
            SDL_free(clip->buffer);
            clip->buffer = nullptr;
            clip->length = 0;
        }
    }
    clips_.clear();
    path_to_id_.clear();

    if (device_id_ != 0) {
        SDL_CloseAudioDevice(device_id_);
        device_id_ = 0;
    }
    if (owns_audio_subsystem_) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        owns_audio_subsystem_ = false;
    }

    initialized_ = false;
}

AudioClipID AudioSystem::load_clip(const char* path) {
    if (!path || path[0] == '\0') {
        return INVALID_AUDIO_CLIP_ID;
    }

    const std::string key(path);
    if (const auto cached = path_to_id_.find(key); cached != path_to_id_.end()) {
        if (clips_.contains(cached->second)) {
            return cached->second;
        }
        path_to_id_.erase(cached);
    }

    SDL_AudioSpec spec;
    std::uint8_t* buffer = nullptr;
    std::uint32_t length = 0;
    if (!SDL_LoadWAV(path, &spec, &buffer, &length)) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to load audio clip '%s': %s", path, SDL_GetError());
        return INVALID_AUDIO_CLIP_ID;
    }

    const AudioClipID id = next_id_++;
    auto clip = std::make_unique<AudioClip>();
    clip->spec = spec;
    clip->buffer = buffer;
    clip->length = length;
    clip->id = id;

    clips_[id] = std::move(clip);
    path_to_id_[key] = id;
    return id;
}

void AudioSystem::unload_clip(AudioClipID id) noexcept {
    const auto clip_it = clips_.find(id);
    if (clip_it == clips_.end()) {
        return;
    }

    active_streams_.erase(
        std::remove_if(
            active_streams_.begin(),
            active_streams_.end(),
            [id](const std::unique_ptr<PlaybackInstance>& instance) {
                return !instance || instance->clip_id == id;
            }
        ),
        active_streams_.end()
    );

    for (auto it = path_to_id_.begin(); it != path_to_id_.end();) {
        if (it->second == id) {
            it = path_to_id_.erase(it);
        } else {
            ++it;
        }
    }

    if (clip_it->second && clip_it->second->buffer) {
        SDL_free(clip_it->second->buffer);
        clip_it->second->buffer = nullptr;
        clip_it->second->length = 0;
    }
    clips_.erase(clip_it);
}

void AudioSystem::play(AudioClipID id, float volume, bool looping) {
    if (!initialized_ && !initialize()) {
        return;
    }

    active_streams_.erase(
        std::remove_if(
            active_streams_.begin(),
            active_streams_.end(),
            [](const std::unique_ptr<PlaybackInstance>& instance) {
                return !instance
                    || (!instance->looping
                        && instance->stream
                        && SDL_GetAudioStreamQueued(instance->stream) == 0);
            }
        ),
        active_streams_.end()
    );

    const auto clip_it = clips_.find(id);
    if (clip_it == clips_.end() || !clip_it->second || !clip_it->second->buffer) {
        return;
    }

    auto instance = std::make_unique<PlaybackInstance>();
    instance->clip_id = id;
    instance->clip = clip_it->second.get();
    instance->volume = std::clamp(volume, 0.0f, 1.0f);
    instance->looping = looping;
    instance->stream = SDL_CreateAudioStream(&instance->clip->spec, nullptr);
    if (!instance->stream) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to create audio stream: %s", SDL_GetError());
        return;
    }

    SDL_SetAudioStreamGain(instance->stream, instance->volume * master_volume_);
    if (!queue_audio_data(instance->stream, instance->clip->buffer, instance->clip->length)) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to queue audio clip: %s", SDL_GetError());
        return;
    }
    if (looping) {
        SDL_SetAudioStreamGetCallback(instance->stream, refill_looping_stream, instance.get());
    } else {
        SDL_FlushAudioStream(instance->stream);
    }
    if (!SDL_BindAudioStream(device_id_, instance->stream)) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to bind audio stream: %s", SDL_GetError());
        return;
    }

    active_streams_.push_back(std::move(instance));
}

void AudioSystem::stop_all() noexcept {
    active_streams_.clear();
}

void AudioSystem::set_master_volume(float volume) noexcept {
    master_volume_ = std::clamp(volume, 0.0f, 1.0f);
    for (const auto& instance : active_streams_) {
        if (instance && instance->stream) {
            SDL_SetAudioStreamGain(instance->stream, instance->volume * master_volume_);
        }
    }
}

} // namespace audio
