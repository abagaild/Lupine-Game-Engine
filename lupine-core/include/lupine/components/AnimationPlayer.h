#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>

namespace Lupine {

class SkinnedMesh;

/**
 * @brief Animation player component for controlling skeletal animations
 *
 * AnimationPlayer component controls the playback of skeletal animations
 * on SkinnedMesh components. It provides state management, blending,
 * and animation control functionality.
 */
class AnimationPlayer : public Component {
public:
    /**
     * @brief Animation playback state
     */
    enum class PlaybackState {
        Stopped,
        Playing,
        Paused
    };
    
    /**
     * @brief Animation loop mode
     */
    enum class LoopMode {
        None,      // Play once and stop
        Loop,      // Loop continuously
        PingPong   // Play forward then backward
    };
    
    /**
     * @brief Constructor
     */
    AnimationPlayer();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~AnimationPlayer() = default;
    
    /**
     * @brief Play animation
     * @param animation_name Name of animation to play
     * @param loop_mode Loop mode for playback
     */
    void Play(const std::string& animation_name, LoopMode loop_mode = LoopMode::Loop);
    
    /**
     * @brief Stop animation playback
     */
    void Stop();
    
    /**
     * @brief Pause animation playback
     */
    void Pause();
    
    /**
     * @brief Resume animation playback
     */
    void Resume();
    
    /**
     * @brief Seek to specific time in animation
     * @param time Time in seconds
     */
    void Seek(float time);
    
    /**
     * @brief Get current playback state
     * @return Current playback state
     */
    PlaybackState GetPlaybackState() const { return m_playback_state; }
    
    /**
     * @brief Get current animation name
     * @return Current animation name
     */
    const std::string& GetCurrentAnimation() const { return m_current_animation; }
    
    /**
     * @brief Get current animation time
     * @return Current animation time in seconds
     */
    float GetCurrentTime() const { return m_current_time; }
    
    /**
     * @brief Get animation duration
     * @param animation_name Animation name (empty for current animation)
     * @return Animation duration in seconds
     */
    float GetAnimationDuration(const std::string& animation_name = "") const;
    
    /**
     * @brief Set playback speed
     * @param speed Playback speed multiplier (1.0 = normal speed)
     */
    void SetPlaybackSpeed(float speed) { m_playback_speed = speed; }
    
    /**
     * @brief Get playback speed
     * @return Current playback speed
     */
    float GetPlaybackSpeed() const { return m_playback_speed; }
    
    /**
     * @brief Set auto-play animation on ready
     * @param auto_play Whether to auto-play
     * @param animation_name Animation to auto-play (empty for first available)
     */
    void SetAutoPlay(bool auto_play, const std::string& animation_name = "");
    
    /**
     * @brief Get available animation names
     * @return Vector of animation names
     */
    std::vector<std::string> GetAvailableAnimations() const;
    
    /**
     * @brief Component lifecycle methods
     */
    void OnReady() override;
    void OnUpdate(float delta_time) override;
    
    /**
     * @brief Initialize export variables for editor
     */
    void InitializeExportVariables() override;

private:
    PlaybackState m_playback_state;
    LoopMode m_loop_mode;
    std::string m_current_animation;
    float m_current_time;
    float m_playback_speed;
    bool m_auto_play;
    std::string m_auto_play_animation;
    bool m_ping_pong_reverse;
    
    // Reference to SkinnedMesh component
    SkinnedMesh* m_skinned_mesh;
    
    /**
     * @brief Update from export variables
     */
    void UpdateFromExportVariables();
    
    /**
     * @brief Find SkinnedMesh component on same node
     */
    void FindSkinnedMeshComponent();
    
    /**
     * @brief Update animation time based on loop mode
     * @param delta_time Time delta
     * @param duration Animation duration
     */
    void UpdateAnimationTime(float delta_time, float duration);
};

} // namespace Lupine
