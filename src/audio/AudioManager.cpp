#include "lupine/audio/AudioManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Lupine {

// Static member definitions
bool AudioManager::s_initialized = false;
SDL_AudioDeviceID AudioManager::s_audio_device = 0;
SDL_AudioSpec AudioManager::s_audio_spec = {};
float AudioManager::s_master_volume = 1.0f;
glm::vec3 AudioManager::s_listener_position = glm::vec3(0.0f);
glm::vec3 AudioManager::s_listener_forward = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 AudioManager::s_listener_up = glm::vec3(0.0f, 1.0f, 0.0f);
std::unordered_map<std::string, std::shared_ptr<AudioClip>> AudioManager::s_audio_clips;
std::vector<std::unique_ptr<AudioInstance>> AudioManager::s_audio_instances;
int AudioManager::s_next_instance_id = 1;

bool AudioManager::Initialize() {
    if (s_initialized) {
        return true;
    }

    std::cout << "Initializing Audio Manager..." << std::endl;

    // Initialize SDL audio subsystem
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL audio: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set up desired audio specification
    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_S16SYS;
    desired_spec.channels = 2;
    desired_spec.samples = 4096;
    desired_spec.callback = AudioCallback;
    desired_spec.userdata = nullptr;

    // Open audio device
    s_audio_device = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &s_audio_spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (s_audio_device == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }

    std::cout << "Audio device opened successfully:" << std::endl;
    std::cout << "  Frequency: " << s_audio_spec.freq << " Hz" << std::endl;
    std::cout << "  Format: " << s_audio_spec.format << std::endl;
    std::cout << "  Channels: " << (int)s_audio_spec.channels << std::endl;
    std::cout << "  Samples: " << s_audio_spec.samples << std::endl;

    // Start audio playback
    SDL_PauseAudioDevice(s_audio_device, 0);

    s_initialized = true;
    std::cout << "Audio Manager initialized successfully!" << std::endl;
    return true;
}

void AudioManager::Shutdown() {
    if (!s_initialized) {
        return;
    }

    std::cout << "Shutting down Audio Manager..." << std::endl;

    // Stop all audio instances
    s_audio_instances.clear();

    // Close audio device
    if (s_audio_device != 0) {
        SDL_CloseAudioDevice(s_audio_device);
        s_audio_device = 0;
    }

    // Clear audio clips
    s_audio_clips.clear();

    // Quit SDL audio subsystem
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    s_initialized = false;
    std::cout << "Audio Manager shut down." << std::endl;
}

void AudioManager::Update(float delta_time) {
    if (!s_initialized) {
        return;
    }

    // Remove finished audio instances
    s_audio_instances.erase(
        std::remove_if(s_audio_instances.begin(), s_audio_instances.end(),
            [](const std::unique_ptr<AudioInstance>& instance) {
                return !instance->playing && !instance->paused;
            }),
        s_audio_instances.end()
    );
}

std::shared_ptr<AudioClip> AudioManager::LoadAudioClip(const std::string& path) {
    if (!s_initialized) {
        std::cerr << "Audio Manager not initialized!" << std::endl;
        return nullptr;
    }

    // Check if already loaded
    auto it = s_audio_clips.find(path);
    if (it != s_audio_clips.end()) {
        return it->second;
    }

    // Load audio file
    SDL_AudioSpec file_spec;
    Uint8* audio_buffer;
    Uint32 audio_length;

    if (SDL_LoadWAV(path.c_str(), &file_spec, &audio_buffer, &audio_length) == nullptr) {
        std::cerr << "Failed to load audio file '" << path << "': " << SDL_GetError() << std::endl;
        return nullptr;
    }

    // Create audio clip
    auto clip = std::make_shared<AudioClip>();
    clip->path = path;
    clip->spec = file_spec;
    clip->length = audio_length;

    // Convert audio to our format if necessary
    SDL_AudioCVT cvt;
    if (SDL_BuildAudioCVT(&cvt, file_spec.format, file_spec.channels, file_spec.freq,
                          s_audio_spec.format, s_audio_spec.channels, s_audio_spec.freq) < 0) {
        std::cerr << "Failed to build audio converter: " << SDL_GetError() << std::endl;
        SDL_FreeWAV(audio_buffer);
        return nullptr;
    }

    if (cvt.needed) {
        cvt.len = audio_length;
        cvt.buf = (Uint8*)SDL_malloc(cvt.len * cvt.len_mult);
        SDL_memcpy(cvt.buf, audio_buffer, audio_length);

        if (SDL_ConvertAudio(&cvt) < 0) {
            std::cerr << "Failed to convert audio: " << SDL_GetError() << std::endl;
            SDL_free(cvt.buf);
            SDL_FreeWAV(audio_buffer);
            return nullptr;
        }

        clip->buffer = cvt.buf;
        clip->length = cvt.len_cvt;
        clip->spec = s_audio_spec;
        SDL_FreeWAV(audio_buffer);
    } else {
        clip->buffer = audio_buffer;
    }

    // Cache the clip
    s_audio_clips[path] = clip;

    std::cout << "Loaded audio clip: " << path << " (" << clip->length << " bytes)" << std::endl;
    return clip;
}

int AudioManager::PlayAudio(std::shared_ptr<AudioClip> clip, float volume, float pitch, bool looping, AudioSource* source) {
    if (!s_initialized || !clip) {
        return -1;
    }

    // Create new audio instance
    auto instance = std::make_unique<AudioInstance>();
    instance->clip = clip;
    instance->position = 0;
    instance->volume = std::clamp(volume, 0.0f, 1.0f);
    instance->pitch = std::max(pitch, 0.1f); // Prevent negative or zero pitch
    instance->looping = looping;
    instance->playing = true;
    instance->paused = false;
    instance->source = source;

    int instance_id = s_next_instance_id++;
    s_audio_instances.push_back(std::move(instance));

    return instance_id;
}

void AudioManager::StopAudio(int instance_id) {
    if (!s_initialized) {
        return;
    }

    // Find and stop the audio instance
    for (auto& instance : s_audio_instances) {
        if (instance && instance->playing) {
            // For simplicity, we'll mark it as not playing
            // In a more sophisticated system, you'd track instance IDs
            instance->playing = false;
            break;
        }
    }
}

void AudioManager::PauseAudio(int instance_id) {
    if (!s_initialized) {
        return;
    }

    for (auto& instance : s_audio_instances) {
        if (instance && instance->playing) {
            instance->paused = true;
            break;
        }
    }
}

void AudioManager::ResumeAudio(int instance_id) {
    if (!s_initialized) {
        return;
    }

    for (auto& instance : s_audio_instances) {
        if (instance && instance->paused) {
            instance->paused = false;
            break;
        }
    }
}

void AudioManager::SetVolume(int instance_id, float volume) {
    if (!s_initialized) {
        return;
    }

    for (auto& instance : s_audio_instances) {
        if (instance) {
            instance->volume = std::clamp(volume, 0.0f, 1.0f);
            break;
        }
    }
}

void AudioManager::SetPitch(int instance_id, float pitch) {
    if (!s_initialized) {
        return;
    }

    for (auto& instance : s_audio_instances) {
        if (instance) {
            instance->pitch = std::max(pitch, 0.1f);
            break;
        }
    }
}

void AudioManager::SetMasterVolume(float volume) {
    s_master_volume = std::clamp(volume, 0.0f, 1.0f);
}

float AudioManager::GetMasterVolume() {
    return s_master_volume;
}

void AudioManager::SetListenerPosition(const glm::vec3& position) {
    s_listener_position = position;
}

void AudioManager::SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up) {
    s_listener_forward = glm::normalize(forward);
    s_listener_up = glm::normalize(up);
}

bool AudioManager::IsInitialized() {
    return s_initialized;
}

void AudioManager::AudioCallback(void* userdata, Uint8* stream, int len) {
    // Clear the output buffer
    SDL_memset(stream, 0, len);

    if (!s_initialized) {
        return;
    }

    // Mix all active audio instances
    for (auto& instance : s_audio_instances) {
        if (instance && instance->playing && !instance->paused) {
            MixAudioInstance(instance.get(), stream, len, s_master_volume);
        }
    }
}

void AudioManager::MixAudioInstance(AudioInstance* instance, Uint8* stream, int len, float master_volume) {
    if (!instance || !instance->clip || !instance->clip->buffer) {
        return;
    }

    AudioClip* clip = instance->clip.get();
    Uint32 remaining = clip->length - instance->position;

    if (remaining == 0) {
        if (instance->looping) {
            instance->position = 0;
            remaining = clip->length;
        } else {
            instance->playing = false;
            return;
        }
    }

    // Calculate how much audio to mix
    Uint32 to_mix = std::min((Uint32)len, remaining);

    // Calculate final volume (instance volume * master volume)
    float final_volume = instance->volume * master_volume;

    // Mix the audio with volume control
    SDL_MixAudioFormat(stream,
                      clip->buffer + instance->position,
                      s_audio_spec.format,
                      to_mix,
                      (int)(final_volume * SDL_MIX_MAXVOLUME));

    // Update position (accounting for pitch)
    instance->position += (Uint32)(to_mix * instance->pitch);

    // Handle looping
    if (instance->position >= clip->length) {
        if (instance->looping) {
            instance->position = 0;
        } else {
            instance->playing = false;
        }
    }
}

float AudioManager::Calculate3DVolume(const glm::vec3& source_position, float max_distance) {
    float distance = glm::length(source_position - s_listener_position);

    if (distance >= max_distance) {
        return 0.0f;
    }

    // Simple linear falloff
    return 1.0f - (distance / max_distance);
}

} // namespace Lupine
