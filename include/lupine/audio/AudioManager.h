#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace Lupine {

// Forward declarations
class AudioSource;

/**
 * @brief Audio clip data structure
 */
struct AudioClip {
    std::string path;
    Uint8* buffer;
    Uint32 length;
    SDL_AudioSpec spec;

    AudioClip() : buffer(nullptr), length(0) {}
    ~AudioClip() {
        if (buffer) {
            SDL_free(buffer);
        }
    }
};

/**
 * @brief Audio playback instance
 */
struct AudioInstance {
    std::shared_ptr<AudioClip> clip;
    Uint32 position;
    float volume;
    float pitch;
    bool looping;
    bool playing;
    bool paused;
    AudioSource* source; // Reference to the component that owns this instance

    AudioInstance() : position(0), volume(1.0f), pitch(1.0f), looping(false), playing(false), paused(false), source(nullptr) {}
};

/**
 * @brief Main audio manager class
 *
 * Manages audio initialization, loading, and playback using SDL2 audio.
 * Supports 3D positional audio and multiple simultaneous audio streams.
 */
class AudioManager {
public:
    /**
     * @brief Initialize the audio system
     * @return True if initialization succeeded
     */
    static bool Initialize();

    /**
     * @brief Shutdown the audio system
     */
    static void Shutdown();

    /**
     * @brief Update the audio system (call every frame)
     * @param delta_time Time since last frame
     */
    static void Update(float delta_time);

    /**
     * @brief Load an audio clip from file
     * @param path Path to audio file
     * @return Shared pointer to audio clip, nullptr if failed
     */
    static std::shared_ptr<AudioClip> LoadAudioClip(const std::string& path);

    /**
     * @brief Play an audio clip
     * @param clip Audio clip to play
     * @param volume Volume (0.0 to 1.0)
     * @param pitch Pitch multiplier (1.0 = normal)
     * @param looping Whether to loop the audio
     * @param source AudioSource component that owns this playback
     * @return Instance ID for controlling playback
     */
    static int PlayAudio(std::shared_ptr<AudioClip> clip, float volume = 1.0f, float pitch = 1.0f, bool looping = false, AudioSource* source = nullptr);

    /**
     * @brief Stop audio playback
     * @param instance_id Instance ID returned by PlayAudio
     */
    static void StopAudio(int instance_id);

    /**
     * @brief Pause audio playback
     * @param instance_id Instance ID returned by PlayAudio
     */
    static void PauseAudio(int instance_id);

    /**
     * @brief Resume audio playback
     * @param instance_id Instance ID returned by PlayAudio
     */
    static void ResumeAudio(int instance_id);

    /**
     * @brief Set volume for audio instance
     * @param instance_id Instance ID
     * @param volume Volume (0.0 to 1.0)
     */
    static void SetVolume(int instance_id, float volume);

    /**
     * @brief Set pitch for audio instance
     * @param instance_id Instance ID
     * @param pitch Pitch multiplier (1.0 = normal)
     */
    static void SetPitch(int instance_id, float pitch);

    /**
     * @brief Set master volume
     * @param volume Master volume (0.0 to 1.0)
     */
    static void SetMasterVolume(float volume);

    /**
     * @brief Get master volume
     * @return Master volume (0.0 to 1.0)
     */
    static float GetMasterVolume();

    /**
     * @brief Set listener position for 3D audio
     * @param position Listener position
     */
    static void SetListenerPosition(const glm::vec3& position);

    /**
     * @brief Set listener orientation for 3D audio
     * @param forward Forward vector
     * @param up Up vector
     */
    static void SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up);

    /**
     * @brief Check if audio system is initialized
     * @return True if initialized
     */
    static bool IsInitialized();

    /**
     * @brief Calculate 3D audio volume based on distance
     * @param source_position Source position in world space
     * @param max_distance Maximum distance for audio falloff
     * @return Volume multiplier (0.0 to 1.0)
     */
    static float Calculate3DVolume(const glm::vec3& source_position, float max_distance);

private:
    static bool s_initialized;
    static SDL_AudioDeviceID s_audio_device;
    static SDL_AudioSpec s_audio_spec;
    static float s_master_volume;
    static glm::vec3 s_listener_position;
    static glm::vec3 s_listener_forward;
    static glm::vec3 s_listener_up;

    static std::unordered_map<std::string, std::shared_ptr<AudioClip>> s_audio_clips;
    static std::vector<std::unique_ptr<AudioInstance>> s_audio_instances;
    static int s_next_instance_id;

    /**
     * @brief SDL audio callback function
     */
    static void AudioCallback(void* userdata, Uint8* stream, int len);

    /**
     * @brief Mix audio instance into output stream
     */
    static void MixAudioInstance(AudioInstance* instance, Uint8* stream, int len, float master_volume);
};

} // namespace Lupine
