#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <glm/glm.hpp>
#include "lupine/core/UUID.h"

namespace Lupine {

/**
 * @brief Condition types for state transitions
 */
enum class ConditionType {
    Bool,
    Int,
    Float,
    Trigger
};

/**
 * @brief Comparison operators for conditions
 */
enum class ComparisonOperator {
    Equals,
    NotEquals,
    Greater,
    GreaterEqual,
    Less,
    LessEqual
};

/**
 * @brief Parameter value variant
 */
struct ParameterValue {
    ConditionType type;
    union {
        bool bool_value;
        int int_value;
        float float_value;
    };
    
    ParameterValue() : type(ConditionType::Bool), bool_value(false) {}
    ParameterValue(bool value) : type(ConditionType::Bool), bool_value(value) {}
    ParameterValue(int value) : type(ConditionType::Int), int_value(value) {}
    ParameterValue(float value) : type(ConditionType::Float), float_value(value) {}
};

/**
 * @brief Animation parameter
 */
struct AnimationParameter {
    std::string name;
    ConditionType type;
    ParameterValue default_value;
    
    AnimationParameter() = default;
    AnimationParameter(const std::string& n, ConditionType t, const ParameterValue& def)
        : name(n), type(t), default_value(def) {}
};

/**
 * @brief Transition condition
 */
struct TransitionCondition {
    std::string parameter_name;
    ComparisonOperator operator_type;
    ParameterValue value;
    
    TransitionCondition() : operator_type(ComparisonOperator::Equals) {}
    TransitionCondition(const std::string& param, ComparisonOperator op, const ParameterValue& val)
        : parameter_name(param), operator_type(op), value(val) {}
};

/**
 * @brief State transition
 */
struct StateTransition {
    UUID id;
    std::string from_state;
    std::string to_state;
    std::vector<TransitionCondition> conditions;
    float transition_duration;      // Blend duration in seconds
    float exit_time;               // Exit time as percentage of animation (0-1)
    bool has_exit_time;            // Whether to use exit time
    bool can_transition_to_self;   // Whether this transition can go to the same state
    
    StateTransition() 
        : id(UUID::Generate())
        , transition_duration(0.25f)
        , exit_time(1.0f)
        , has_exit_time(false)
        , can_transition_to_self(false) {}
};

/**
 * @brief Animation state
 */
struct AnimationState {
    UUID id;
    std::string name;
    std::string animation_clip;     // Name of the animation clip to play
    float speed;                    // Playback speed multiplier
    bool looping;                   // Whether the animation loops
    glm::vec2 position;            // Position in the state machine graph (for editor)
    
    AnimationState() 
        : id(UUID::Generate())
        , speed(1.0f)
        , looping(true)
        , position(0.0f, 0.0f) {}
        
    AnimationState(const std::string& n, const std::string& clip)
        : id(UUID::Generate())
        , name(n)
        , animation_clip(clip)
        , speed(1.0f)
        , looping(true)
        , position(0.0f, 0.0f) {}
};

/**
 * @brief State machine layer
 */
struct StateMachineLayer {
    std::string name;
    float weight;                   // Layer weight for blending
    bool additive;                  // Whether this layer is additive
    std::string default_state;      // Default state name
    std::vector<AnimationState> states;
    std::vector<StateTransition> transitions;
    
    StateMachineLayer() : weight(1.0f), additive(false) {}
    StateMachineLayer(const std::string& n) : name(n), weight(1.0f), additive(false) {}
};

/**
 * @brief State machine animation resource (.statemachine files)
 */
class StateMachineResource {
public:
    StateMachineResource() = default;
    ~StateMachineResource() = default;
    
    // Parameter management
    void AddParameter(const AnimationParameter& parameter);
    void RemoveParameter(const std::string& name);
    AnimationParameter* GetParameter(const std::string& name);
    const AnimationParameter* GetParameter(const std::string& name) const;
    std::vector<std::string> GetParameterNames() const;
    
    // Layer management
    void AddLayer(const StateMachineLayer& layer);
    void RemoveLayer(const std::string& name);
    StateMachineLayer* GetLayer(const std::string& name);
    const StateMachineLayer* GetLayer(const std::string& name) const;
    std::vector<std::string> GetLayerNames() const;
    
    // State management (for specific layer)
    void AddState(const std::string& layer_name, const AnimationState& state);
    void RemoveState(const std::string& layer_name, const std::string& state_name);
    AnimationState* GetState(const std::string& layer_name, const std::string& state_name);
    
    // Transition management (for specific layer)
    void AddTransition(const std::string& layer_name, const StateTransition& transition);
    void RemoveTransition(const std::string& layer_name, const UUID& transition_id);
    StateTransition* GetTransition(const std::string& layer_name, const UUID& transition_id);
    
    // Utility functions
    std::vector<StateTransition*> GetTransitionsFromState(const std::string& layer_name, const std::string& state_name);
    std::vector<StateTransition*> GetTransitionsToState(const std::string& layer_name, const std::string& state_name);
    
    // Serialization
    bool SaveToFile(const std::string& filepath) const;
    bool LoadFromFile(const std::string& filepath);
    
    // JSON serialization
    std::string ToJSON() const;
    bool FromJSON(const std::string& json);
    
private:
    std::map<std::string, AnimationParameter> m_parameters;
    std::map<std::string, StateMachineLayer> m_layers;
};

/**
 * @brief Animation duration provider interface for state machine runtime
 */
class IAnimationDurationProvider {
public:
    virtual ~IAnimationDurationProvider() = default;
    virtual float GetAnimationDuration(const std::string& animation_clip) const = 0;
};

/**
 * @brief State machine runtime for executing state machines
 */
class StateMachineRuntime {
public:
    StateMachineRuntime() = default;
    ~StateMachineRuntime() = default;
    
    // Resource management
    void SetResource(std::shared_ptr<StateMachineResource> resource);
    std::shared_ptr<StateMachineResource> GetResource() const { return m_resource; }

    // Animation duration provider
    void SetAnimationDurationProvider(IAnimationDurationProvider* provider) { m_animation_duration_provider = provider; }
    
    // Parameter control
    void SetBool(const std::string& name, bool value);
    void SetInt(const std::string& name, int value);
    void SetFloat(const std::string& name, float value);
    void SetTrigger(const std::string& name);
    
    bool GetBool(const std::string& name) const;
    int GetInt(const std::string& name) const;
    float GetFloat(const std::string& name) const;
    
    // State control
    void Play(const std::string& layer_name = "");
    void Stop();
    void Pause();
    void Resume();
    
    // Current state info
    std::string GetCurrentState(const std::string& layer_name = "") const;
    float GetCurrentStateTime(const std::string& layer_name = "") const;
    float GetCurrentStateNormalizedTime(const std::string& layer_name = "") const;
    
    // Update
    void Update(float delta_time);
    
private:
    struct LayerRuntime {
        std::string current_state;
        std::string next_state;
        float state_time;
        float transition_time;
        float transition_duration;
        bool is_transitioning;
        bool is_playing;
    };
    
    std::shared_ptr<StateMachineResource> m_resource;
    std::map<std::string, ParameterValue> m_parameter_values;
    std::map<std::string, LayerRuntime> m_layer_runtimes;
    IAnimationDurationProvider* m_animation_duration_provider;
    
    bool EvaluateConditions(const std::vector<TransitionCondition>& conditions) const;
    bool EvaluateCondition(const TransitionCondition& condition) const;
    void CheckTransitions(const std::string& layer_name, LayerRuntime& runtime);
    void UpdateLayer(const std::string& layer_name, LayerRuntime& runtime, float delta_time);
};

} // namespace Lupine
