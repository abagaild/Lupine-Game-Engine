#pragma once

#include "lupine/core/Component.h"
#include "lupine/audio/AudioManager.h"
#include <string>
#include <memory>
#include <glm/glm.hpp>

namespace Lupine {

/**
 * @brief Audio source component for playing audio clips
 * 
 * This component allows nodes to play audio clips with various properties
 * like volume, pitch, looping, and 3D positioning.
 */
class AudioSource : public Component {
public:
    /**
     * @brief Constructor
     */
    AudioSource();
    
    /**
     * @brief Destructor
     */
    ~AudioSource();
    
    /**
     * @brief Component lifecycle - called when node enters the scene tree
     */
    void OnReady() override;
    
    /**
     * @brief Component lifecycle - called every frame
     * @param delta_time Time since last frame
     */
    void OnUpdate(float delta_time) override;
    
    /**
     * @brief Component lifecycle - called when component is removed
     */
    void OnDestroy() override;
    
    /**
     * @brief Get component category for editor
     * @return Category string
     */
    std::string GetCategory() const override { return "Audio"; }
    
    // Audio clip management
    
    /**
     * @brief Set the audio clip to play
     * @param path Path to audio file
     */
    void SetAudioClip(const std::string& path);
    
    /**
     * @brief Get the current audio clip path
     * @return Audio clip path
     */
    const std::string& GetAudioClip() const { return m_audio_clip_path; }
    
    // Playback control
    
    /**
     * @brief Play the audio clip
     */
    void Play();
    
    /**
     * @brief Stop audio playback
     */
    void Stop();
    
    /**
     * @brief Pause audio playback
     */
    void Pause();
    
    /**
     * @brief Resume audio playback
     */
    void Resume();
    
    /**
     * @brief Check if audio is currently playing
     * @return True if playing
     */
    bool IsPlaying() const { return m_is_playing; }
    
    /**
     * @brief Check if audio is paused
     * @return True if paused
     */
    bool IsPaused() const { return m_is_paused; }
    
    // Audio properties
    
    /**
     * @brief Set volume (0.0 to 1.0)
     * @param volume Volume level
     */
    void SetVolume(float volume);
    
    /**
     * @brief Get volume
     * @return Volume level (0.0 to 1.0)
     */
    float GetVolume() const { return m_volume; }
    
    /**
     * @brief Set pitch multiplier (1.0 = normal)
     * @param pitch Pitch multiplier
     */
    void SetPitch(float pitch);
    
    /**
     * @brief Get pitch
     * @return Pitch multiplier
     */
    float GetPitch() const { return m_pitch; }
    
    /**
     * @brief Set whether audio should loop
     * @param looping True to loop
     */
    void SetLooping(bool looping);
    
    /**
     * @brief Check if audio is set to loop
     * @return True if looping
     */
    bool IsLooping() const { return m_looping; }
    
    /**
     * @brief Set whether to play on start
     * @param play_on_start True to play automatically
     */
    void SetPlayOnStart(bool play_on_start);
    
    /**
     * @brief Check if set to play on start
     * @return True if play on start is enabled
     */
    bool GetPlayOnStart() const { return m_play_on_start; }
    
    // 3D Audio properties
    
    /**
     * @brief Set whether this is a 3D audio source
     * @param is_3d True for 3D audio
     */
    void SetIs3D(bool is_3d);
    
    /**
     * @brief Check if this is a 3D audio source
     * @return True if 3D audio
     */
    bool Is3D() const { return m_is_3d; }
    
    /**
     * @brief Set maximum distance for 3D audio falloff
     * @param max_distance Maximum distance
     */
    void SetMaxDistance(float max_distance);
    
    /**
     * @brief Get maximum distance for 3D audio
     * @return Maximum distance
     */
    float GetMaxDistance() const { return m_max_distance; }

    /**
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "AudioSource"; }

protected:
    /**
     * @brief Initialize export variables for editor
     */
    void InitializeExportVariables() override;

    /**
     * @brief Update internal state from export variables
     */
    void UpdateFromExportVariables();

private:
    // Audio clip
    std::string m_audio_clip_path;
    std::shared_ptr<AudioClip> m_audio_clip;
    
    // Playback state
    int m_instance_id;
    bool m_is_playing;
    bool m_is_paused;
    
    // Audio properties
    float m_volume;
    float m_pitch;
    bool m_looping;
    bool m_play_on_start;
    
    // 3D Audio properties
    bool m_is_3d;
    float m_max_distance;
    
    /**
     * @brief Update 3D audio properties based on node position
     */
    void Update3DAudio();
};

} // namespace Lupine
