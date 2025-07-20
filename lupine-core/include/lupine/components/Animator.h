#pragma once

#include "lupine/core/Component.h"
#include "lupine/resources/AnimationResource.h"
#include <memory>
#include <string>
#include <map>

namespace Lupine {

/**
 * @brief Animation playback state
 */
enum class AnimationPlaybackState {
    Stopped,
    Playing,
    Paused
};

/**
 * @brief Animation loop mode
 */
enum class AnimationLoopMode {
    None,      // Play once and stop
    Loop,      // Loop continuously
    PingPong   // Play forward then backward
};

/**
 * @brief Animator component for playing tween animations
 * 
 * The Animator component loads .anim resources and orchestrates animations
 * for different nodes in the scene. It provides Godot-like animation
 * functionality with keyframe interpolation and property animation.
 */
class Animator : public Component {
public:
    /**
     * @brief Constructor
     */
    Animator();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Animator() = default;
    
    // Animation resource management
    void SetAnimationResource(const std::string& filepath);
    const std::string& GetAnimationResourcePath() const { return m_animation_resource_path; }
    
    // Animation playback control
    void Play(const std::string& clip_name = "", AnimationLoopMode loop_mode = AnimationLoopMode::Loop);
    void Stop();
    void Pause();
    void Resume();
    
    // Animation state queries
    bool IsPlaying() const { return m_playback_state == AnimationPlaybackState::Playing; }
    bool IsPaused() const { return m_playback_state == AnimationPlaybackState::Paused; }
    bool IsStopped() const { return m_playback_state == AnimationPlaybackState::Stopped; }
    
    // Current animation info
    const std::string& GetCurrentClip() const { return m_current_clip; }
    float GetCurrentTime() const { return m_current_time; }
    float GetCurrentNormalizedTime() const;
    float GetClipDuration(const std::string& clip_name = "") const;
    
    // Animation control
    void SetTime(float time);
    void SetNormalizedTime(float normalized_time);
    void SetPlaybackSpeed(float speed) { m_playback_speed = speed; }
    float GetPlaybackSpeed() const { return m_playback_speed; }
    
    // Auto-play settings
    void SetAutoPlay(bool auto_play) { m_auto_play = auto_play; }
    bool GetAutoPlay() const { return m_auto_play; }
    
    void SetAutoPlayClip(const std::string& clip_name) { m_auto_play_clip = clip_name; }
    const std::string& GetAutoPlayClip() const { return m_auto_play_clip; }
    
    // Available animations
    std::vector<std::string> GetAvailableClips() const;
    
    // Component interface
    std::string GetTypeName() const override { return "Animator"; }
    std::string GetCategory() const override { return "Animation"; }
    
    // Lifecycle methods
    void OnReady() override;
    void OnUpdate(float delta_time) override;

protected:
    void InitializeExportVariables() override;

private:
    // Animation resource
    std::string m_animation_resource_path;
    std::unique_ptr<TweenAnimationResource> m_animation_resource;
    
    // Playback state
    AnimationPlaybackState m_playback_state;
    std::string m_current_clip;
    float m_current_time;
    float m_playback_speed;
    AnimationLoopMode m_loop_mode;
    bool m_ping_pong_reverse;
    
    // Auto-play settings
    bool m_auto_play;
    std::string m_auto_play_clip;
    
    // Internal methods
    void LoadAnimationResource();
    void UpdateFromExportVariables();
    void UpdateAnimationTime(float delta_time, float duration);
    void ApplyAnimationAtTime(float time);
    void ApplyTrackAtTime(const AnimationTrack& track, float time);
    AnimationValue InterpolateKeyframes(const AnimationTrack& track, float time);
    AnimationValue InterpolateValues(const AnimationValue& a, const AnimationValue& b, float t, InterpolationType interpolation);
    float ApplyInterpolationCurve(float t, InterpolationType interpolation);
    
    // Node property access
    Node* FindNodeByPath(const std::string& path);
    void SetNodeProperty(Node* node, const std::string& property_name, const AnimationValue& value);
    AnimationValue GetNodeProperty(Node* node, const std::string& property_name);
};

} // namespace Lupine
