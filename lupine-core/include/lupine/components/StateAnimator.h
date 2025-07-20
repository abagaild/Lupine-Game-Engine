#pragma once

#include "lupine/core/Component.h"
#include "lupine/resources/StateMachineResource.h"
#include "lupine/resources/AnimationResource.h"
#include <memory>
#include <string>
#include <map>

namespace Lupine {

/**
 * @brief Animation playback state
 */
enum class StateAnimatorPlaybackState {
    Stopped,
    Playing,
    Paused
};

/**
 * @brief StateAnimator component for Unity-style state machine animations
 *
 * The StateAnimator component integrates StateMachineResource with the node system,
 * providing Unity-like Animator Controller functionality. It manages state transitions,
 * parameter control, and animation playback based on state machine logic.
 *
 * Features:
 * - State machine resource loading (.statemachine files)
 * - Parameter control (bool, int, float, trigger)
 * - Automatic state transitions based on conditions
 * - Animation clip integration with states
 * - Layer support for complex animation blending
 * - Exit time calculations and transition timing
 */
class StateAnimator : public Component, public IAnimationDurationProvider {
public:
    /**
     * @brief Constructor
     */
    StateAnimator();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~StateAnimator() = default;
    
    // State machine resource management
    void SetStateMachineResource(const std::string& filepath);
    const std::string& GetStateMachineResourcePath() const { return m_state_machine_resource_path; }
    
    // Animation resource management (for animation clips referenced by states)
    void SetAnimationResource(const std::string& filepath);
    const std::string& GetAnimationResourcePath() const { return m_animation_resource_path; }
    
    // Playback control
    void Play();
    void Stop();
    void Pause();
    void Resume();
    
    // State control
    void PlayState(const std::string& state_name, const std::string& layer_name = "");
    std::string GetCurrentState(const std::string& layer_name = "") const;
    float GetCurrentStateTime(const std::string& layer_name = "") const;
    float GetCurrentStateNormalizedTime(const std::string& layer_name = "") const;
    bool IsInTransition(const std::string& layer_name = "") const;
    
    // Parameter control
    void SetBool(const std::string& name, bool value);
    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetTrigger(const std::string& name);
    
    bool GetBool(const std::string& name) const;
    int GetInt(const std::string& name) const;
    float GetFloat(const std::string& name) const;
    
    // State queries
    bool IsPlaying() const { return m_playback_state == StateAnimatorPlaybackState::Playing; }
    bool IsPaused() const { return m_playback_state == StateAnimatorPlaybackState::Paused; }
    bool IsStopped() const { return m_playback_state == StateAnimatorPlaybackState::Stopped; }
    
    // Auto-play settings
    void SetAutoPlay(bool auto_play) { m_auto_play = auto_play; }
    bool GetAutoPlay() const { return m_auto_play; }
    
    // Available states and parameters
    std::vector<std::string> GetAvailableStates(const std::string& layer_name = "") const;
    std::vector<std::string> GetAvailableParameters() const;
    std::vector<std::string> GetAvailableLayers() const;
    
    // Component interface
    std::string GetTypeName() const override { return "StateAnimator"; }
    std::string GetCategory() const override { return "Animation"; }
    
    // Lifecycle methods
    void OnReady() override;
    void OnUpdate(float delta_time) override;

    // IAnimationDurationProvider interface
    float GetAnimationDuration(const std::string& animation_clip) const override;

protected:
    void InitializeExportVariables() override;

private:
    // Resource paths
    std::string m_state_machine_resource_path;
    std::string m_animation_resource_path;
    
    // Resources
    std::shared_ptr<StateMachineResource> m_state_machine_resource;
    std::unique_ptr<StateMachineRuntime> m_state_machine_runtime;
    std::unique_ptr<TweenAnimationResource> m_animation_resource;
    
    // Playback state
    StateAnimatorPlaybackState m_playback_state;
    bool m_auto_play;
    
    // Current animation state per layer
    struct LayerAnimationState {
        std::string current_animation_clip;
        std::string next_animation_clip;
        float animation_time;
        float animation_speed;
        bool is_transitioning;
        float transition_progress; // 0.0 to 1.0
        float transition_duration; // Duration of current transition
    };
    std::map<std::string, LayerAnimationState> m_layer_animation_states;
    
    // Internal methods
    void LoadStateMachineResource();
    void LoadAnimationResource();
    void UpdateFromExportVariables();
    void UpdateStateMachine(float delta_time);
    void UpdateAnimations(float delta_time);
    void ApplyCurrentAnimations();
    void ApplyAnimationBlending(const std::string& layer_name, const LayerAnimationState& layer_state);
    
    // Animation integration
    void OnStateChanged(const std::string& layer_name, const std::string& old_state, const std::string& new_state);
    void StartStateAnimation(const std::string& layer_name, const std::string& state_name);
    void StartTransition(const std::string& layer_name, const std::string& from_state, const std::string& to_state, float duration);
    float CalculateNormalizedTime(const std::string& animation_clip, float time) const;
    
    // Animation application
    void ApplyAnimationAtTime(const std::string& animation_clip, float time, float weight = 1.0f);
    void ApplyTrackAtTime(const AnimationTrack& track, float time, float weight = 1.0f);
    void BlendAnimations(const std::string& clip_a, float time_a, const std::string& clip_b, float time_b, float blend_factor);

    // Node property access (inherited from Animator)
    Node* FindNodeByPath(const std::string& path);
    void SetNodeProperty(Node* node, const std::string& property_name, const AnimationValue& value, float weight = 1.0f);
    AnimationValue GetNodeProperty(Node* node, const std::string& property_name);
    AnimationValue BlendAnimationValues(const AnimationValue& a, const AnimationValue& b, float t);

    // Animation interpolation
    AnimationValue InterpolateKeyframes(const AnimationTrack& track, float time);
    AnimationValue InterpolateValues(const AnimationValue& a, const AnimationValue& b, float t, InterpolationType interpolation);
    float ApplyInterpolationCurve(float t, InterpolationType interpolation);
};

} // namespace Lupine
