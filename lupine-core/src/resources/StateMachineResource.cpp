#include "lupine/resources/StateMachineResource.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Lupine {

// StateMachineResource implementation

void StateMachineResource::AddParameter(const AnimationParameter& parameter) {
    m_parameters[parameter.name] = parameter;
}

void StateMachineResource::RemoveParameter(const std::string& name) {
    m_parameters.erase(name);
}

AnimationParameter* StateMachineResource::GetParameter(const std::string& name) {
    auto it = m_parameters.find(name);
    return (it != m_parameters.end()) ? &it->second : nullptr;
}

const AnimationParameter* StateMachineResource::GetParameter(const std::string& name) const {
    auto it = m_parameters.find(name);
    return (it != m_parameters.end()) ? &it->second : nullptr;
}

std::vector<std::string> StateMachineResource::GetParameterNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_parameters) {
        names.push_back(pair.first);
    }
    return names;
}

void StateMachineResource::AddLayer(const StateMachineLayer& layer) {
    m_layers[layer.name] = layer;
}

void StateMachineResource::RemoveLayer(const std::string& name) {
    m_layers.erase(name);
}

StateMachineLayer* StateMachineResource::GetLayer(const std::string& name) {
    auto it = m_layers.find(name);
    return (it != m_layers.end()) ? &it->second : nullptr;
}

const StateMachineLayer* StateMachineResource::GetLayer(const std::string& name) const {
    auto it = m_layers.find(name);
    return (it != m_layers.end()) ? &it->second : nullptr;
}

std::vector<std::string> StateMachineResource::GetLayerNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_layers) {
        names.push_back(pair.first);
    }
    return names;
}

void StateMachineResource::AddState(const std::string& layer_name, const AnimationState& state) {
    auto* layer = GetLayer(layer_name);
    if (layer) {
        layer->states.push_back(state);
    }
}

void StateMachineResource::RemoveState(const std::string& layer_name, const std::string& state_name) {
    auto* layer = GetLayer(layer_name);
    if (layer) {
        layer->states.erase(
            std::remove_if(layer->states.begin(), layer->states.end(),
                [&state_name](const AnimationState& state) {
                    return state.name == state_name;
                }),
            layer->states.end()
        );
        
        // Also remove transitions involving this state
        layer->transitions.erase(
            std::remove_if(layer->transitions.begin(), layer->transitions.end(),
                [&state_name](const StateTransition& transition) {
                    return transition.from_state == state_name || transition.to_state == state_name;
                }),
            layer->transitions.end()
        );
    }
}

AnimationState* StateMachineResource::GetState(const std::string& layer_name, const std::string& state_name) {
    auto* layer = GetLayer(layer_name);
    if (layer) {
        for (auto& state : layer->states) {
            if (state.name == state_name) {
                return &state;
            }
        }
    }
    return nullptr;
}

void StateMachineResource::AddTransition(const std::string& layer_name, const StateTransition& transition) {
    auto* layer = GetLayer(layer_name);
    if (layer) {
        layer->transitions.push_back(transition);
    }
}

void StateMachineResource::RemoveTransition(const std::string& layer_name, const UUID& transition_id) {
    auto* layer = GetLayer(layer_name);
    if (layer) {
        layer->transitions.erase(
            std::remove_if(layer->transitions.begin(), layer->transitions.end(),
                [&transition_id](const StateTransition& transition) {
                    return transition.id == transition_id;
                }),
            layer->transitions.end()
        );
    }
}

StateTransition* StateMachineResource::GetTransition(const std::string& layer_name, const UUID& transition_id) {
    auto* layer = GetLayer(layer_name);
    if (layer) {
        for (auto& transition : layer->transitions) {
            if (transition.id == transition_id) {
                return &transition;
            }
        }
    }
    return nullptr;
}

std::vector<StateTransition*> StateMachineResource::GetTransitionsFromState(const std::string& layer_name, const std::string& state_name) {
    std::vector<StateTransition*> transitions;
    auto* layer = GetLayer(layer_name);
    if (layer) {
        for (auto& transition : layer->transitions) {
            if (transition.from_state == state_name) {
                transitions.push_back(&transition);
            }
        }
    }
    return transitions;
}

std::vector<StateTransition*> StateMachineResource::GetTransitionsToState(const std::string& layer_name, const std::string& state_name) {
    std::vector<StateTransition*> transitions;
    auto* layer = GetLayer(layer_name);
    if (layer) {
        for (auto& transition : layer->transitions) {
            if (transition.to_state == state_name) {
                transitions.push_back(&transition);
            }
        }
    }
    return transitions;
}

bool StateMachineResource::SaveToFile(const std::string& filepath) const {
    try {
        std::string jsonData = ToJSON();
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filepath << std::endl;
            return false;
        }
        file << jsonData;
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving state machine resource: " << e.what() << std::endl;
        return false;
    }
}

bool StateMachineResource::LoadFromFile(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            std::cerr << "State machine file not found: " << filepath << std::endl;
            return false;
        }
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filepath << std::endl;
            return false;
        }
        
        std::string jsonData((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        file.close();
        
        return FromJSON(jsonData);
    } catch (const std::exception& e) {
        std::cerr << "Error loading state machine resource: " << e.what() << std::endl;
        return false;
    }
}

std::string StateMachineResource::ToJSON() const {
    json j;
    j["type"] = "StateMachine";
    j["version"] = "1.0";
    
    // Serialize parameters
    json params_json = json::array();
    for (const auto& pair : m_parameters) {
        const AnimationParameter& param = pair.second;
        json param_json;
        param_json["name"] = param.name;
        param_json["type"] = static_cast<int>(param.type);
        
        // Serialize default value
        switch (param.default_value.type) {
            case ConditionType::Bool:
                param_json["default_value"] = param.default_value.bool_value;
                break;
            case ConditionType::Int:
                param_json["default_value"] = param.default_value.int_value;
                break;
            case ConditionType::Float:
                param_json["default_value"] = param.default_value.float_value;
                break;
            case ConditionType::Trigger:
                param_json["default_value"] = false; // Triggers default to false
                break;
        }
        params_json.push_back(param_json);
    }
    j["parameters"] = params_json;
    
    // Serialize layers
    json layers_json = json::array();
    for (const auto& pair : m_layers) {
        const StateMachineLayer& layer = pair.second;
        json layer_json;
        layer_json["name"] = layer.name;
        layer_json["weight"] = layer.weight;
        layer_json["additive"] = layer.additive;
        layer_json["default_state"] = layer.default_state;
        
        // Serialize states
        json states_json = json::array();
        for (const auto& state : layer.states) {
            json state_json;
            state_json["id"] = state.id.ToString();
            state_json["name"] = state.name;
            state_json["animation_clip"] = state.animation_clip;
            state_json["speed"] = state.speed;
            state_json["looping"] = state.looping;
            state_json["position"] = {state.position.x, state.position.y};
            states_json.push_back(state_json);
        }
        layer_json["states"] = states_json;
        
        // Serialize transitions
        json transitions_json = json::array();
        for (const auto& transition : layer.transitions) {
            json trans_json;
            trans_json["id"] = transition.id.ToString();
            trans_json["from_state"] = transition.from_state;
            trans_json["to_state"] = transition.to_state;
            trans_json["transition_duration"] = transition.transition_duration;
            trans_json["exit_time"] = transition.exit_time;
            trans_json["has_exit_time"] = transition.has_exit_time;
            trans_json["can_transition_to_self"] = transition.can_transition_to_self;
            
            // Serialize conditions
            json conditions_json = json::array();
            for (const auto& condition : transition.conditions) {
                json cond_json;
                cond_json["parameter_name"] = condition.parameter_name;
                cond_json["operator"] = static_cast<int>(condition.operator_type);
                
                // Serialize condition value
                switch (condition.value.type) {
                    case ConditionType::Bool:
                        cond_json["value"] = condition.value.bool_value;
                        break;
                    case ConditionType::Int:
                        cond_json["value"] = condition.value.int_value;
                        break;
                    case ConditionType::Float:
                        cond_json["value"] = condition.value.float_value;
                        break;
                    case ConditionType::Trigger:
                        cond_json["value"] = true; // Triggers are always true when checked
                        break;
                }
                conditions_json.push_back(cond_json);
            }
            trans_json["conditions"] = conditions_json;
            transitions_json.push_back(trans_json);
        }
        layer_json["transitions"] = transitions_json;
        layers_json.push_back(layer_json);
    }
    j["layers"] = layers_json;
    
    return j.dump(4);
}

bool StateMachineResource::FromJSON(const std::string& jsonData) {
    try {
        json j = json::parse(jsonData);

        if (j["type"] != "StateMachine") {
            std::cerr << "Invalid state machine resource type" << std::endl;
            return false;
        }

        m_parameters.clear();
        m_layers.clear();

        // Deserialize parameters
        for (const auto& param_json : j["parameters"]) {
            AnimationParameter param;
            param.name = param_json["name"];
            param.type = static_cast<ConditionType>(param_json["type"]);

            // Deserialize default value
            param.default_value.type = param.type;
            switch (param.type) {
                case ConditionType::Bool:
                case ConditionType::Trigger:
                    param.default_value.bool_value = param_json["default_value"];
                    break;
                case ConditionType::Int:
                    param.default_value.int_value = param_json["default_value"];
                    break;
                case ConditionType::Float:
                    param.default_value.float_value = param_json["default_value"];
                    break;
            }
            m_parameters[param.name] = param;
        }

        // Deserialize layers
        for (const auto& layer_json : j["layers"]) {
            StateMachineLayer layer;
            layer.name = layer_json["name"];
            layer.weight = layer_json["weight"];
            layer.additive = layer_json["additive"];
            layer.default_state = layer_json["default_state"];

            // Deserialize states
            for (const auto& state_json : layer_json["states"]) {
                AnimationState state;
                state.id = UUID::FromString(state_json["id"]);
                state.name = state_json["name"];
                state.animation_clip = state_json["animation_clip"];
                state.speed = state_json["speed"];
                state.looping = state_json["looping"];
                state.position = glm::vec2(state_json["position"][0], state_json["position"][1]);
                layer.states.push_back(state);
            }

            // Deserialize transitions
            for (const auto& trans_json : layer_json["transitions"]) {
                StateTransition transition;
                transition.id = UUID::FromString(trans_json["id"]);
                transition.from_state = trans_json["from_state"];
                transition.to_state = trans_json["to_state"];
                transition.transition_duration = trans_json["transition_duration"];
                transition.exit_time = trans_json["exit_time"];
                transition.has_exit_time = trans_json["has_exit_time"];
                transition.can_transition_to_self = trans_json["can_transition_to_self"];

                // Deserialize conditions
                for (const auto& cond_json : trans_json["conditions"]) {
                    TransitionCondition condition;
                    condition.parameter_name = cond_json["parameter_name"];
                    condition.operator_type = static_cast<ComparisonOperator>(cond_json["operator"]);

                    // Find parameter type
                    auto param_it = m_parameters.find(condition.parameter_name);
                    if (param_it != m_parameters.end()) {
                        condition.value.type = param_it->second.type;
                        switch (param_it->second.type) {
                            case ConditionType::Bool:
                            case ConditionType::Trigger:
                                condition.value.bool_value = cond_json["value"];
                                break;
                            case ConditionType::Int:
                                condition.value.int_value = cond_json["value"];
                                break;
                            case ConditionType::Float:
                                condition.value.float_value = cond_json["value"];
                                break;
                        }
                    }
                    transition.conditions.push_back(condition);
                }
                layer.transitions.push_back(transition);
            }
            m_layers[layer.name] = layer;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing state machine JSON: " << e.what() << std::endl;
        return false;
    }
}

// StateMachineRuntime implementation

void StateMachineRuntime::SetResource(std::shared_ptr<StateMachineResource> resource) {
    m_resource = resource;
    m_parameter_values.clear();
    m_layer_runtimes.clear();
    m_animation_duration_provider = nullptr;

    if (m_resource) {
        // Initialize parameter values
        for (const auto& name : m_resource->GetParameterNames()) {
            const auto* param = m_resource->GetParameter(name);
            if (param) {
                m_parameter_values[name] = param->default_value;
            }
        }

        // Initialize layer runtimes
        for (const auto& layer_name : m_resource->GetLayerNames()) {
            LayerRuntime runtime;
            const auto* layer = m_resource->GetLayer(layer_name);
            if (layer && !layer->default_state.empty()) {
                runtime.current_state = layer->default_state;
            }
            runtime.state_time = 0.0f;
            runtime.transition_time = 0.0f;
            runtime.transition_duration = 0.0f;
            runtime.is_transitioning = false;
            runtime.is_playing = false;
            m_layer_runtimes[layer_name] = runtime;
        }
    }
}

void StateMachineRuntime::SetBool(const std::string& name, bool value) {
    auto it = m_parameter_values.find(name);
    if (it != m_parameter_values.end() && it->second.type == ConditionType::Bool) {
        it->second.bool_value = value;
    }
}

void StateMachineRuntime::SetInt(const std::string& name, int value) {
    auto it = m_parameter_values.find(name);
    if (it != m_parameter_values.end() && it->second.type == ConditionType::Int) {
        it->second.int_value = value;
    }
}

void StateMachineRuntime::SetFloat(const std::string& name, float value) {
    auto it = m_parameter_values.find(name);
    if (it != m_parameter_values.end() && it->second.type == ConditionType::Float) {
        it->second.float_value = value;
    }
}

void StateMachineRuntime::SetTrigger(const std::string& name) {
    auto it = m_parameter_values.find(name);
    if (it != m_parameter_values.end() && it->second.type == ConditionType::Trigger) {
        it->second.bool_value = true;
    }
}

bool StateMachineRuntime::GetBool(const std::string& name) const {
    auto it = m_parameter_values.find(name);
    if (it != m_parameter_values.end() && it->second.type == ConditionType::Bool) {
        return it->second.bool_value;
    }
    return false;
}

int StateMachineRuntime::GetInt(const std::string& name) const {
    auto it = m_parameter_values.find(name);
    if (it != m_parameter_values.end() && it->second.type == ConditionType::Int) {
        return it->second.int_value;
    }
    return 0;
}

float StateMachineRuntime::GetFloat(const std::string& name) const {
    auto it = m_parameter_values.find(name);
    if (it != m_parameter_values.end() && it->second.type == ConditionType::Float) {
        return it->second.float_value;
    }
    return 0.0f;
}

void StateMachineRuntime::Play(const std::string& layer_name) {
    if (layer_name.empty()) {
        // Play all layers
        for (auto& pair : m_layer_runtimes) {
            pair.second.is_playing = true;
        }
    } else {
        auto it = m_layer_runtimes.find(layer_name);
        if (it != m_layer_runtimes.end()) {
            it->second.is_playing = true;
        }
    }
}

void StateMachineRuntime::Stop() {
    for (auto& pair : m_layer_runtimes) {
        pair.second.is_playing = false;
        pair.second.state_time = 0.0f;
        pair.second.is_transitioning = false;
    }
}

void StateMachineRuntime::Pause() {
    for (auto& pair : m_layer_runtimes) {
        pair.second.is_playing = false;
    }
}

void StateMachineRuntime::Resume() {
    for (auto& pair : m_layer_runtimes) {
        pair.second.is_playing = true;
    }
}

std::string StateMachineRuntime::GetCurrentState(const std::string& layer_name) const {
    std::string target_layer = layer_name;
    if (target_layer.empty() && !m_layer_runtimes.empty()) {
        target_layer = m_layer_runtimes.begin()->first; // Use first layer
    }

    auto it = m_layer_runtimes.find(target_layer);
    if (it != m_layer_runtimes.end()) {
        return it->second.current_state;
    }
    return "";
}

float StateMachineRuntime::GetCurrentStateTime(const std::string& layer_name) const {
    std::string target_layer = layer_name;
    if (target_layer.empty() && !m_layer_runtimes.empty()) {
        target_layer = m_layer_runtimes.begin()->first; // Use first layer
    }

    auto it = m_layer_runtimes.find(target_layer);
    if (it != m_layer_runtimes.end()) {
        return it->second.state_time;
    }
    return 0.0f;
}

float StateMachineRuntime::GetCurrentStateNormalizedTime(const std::string& layer_name) const {
    // TODO: Implement normalized time calculation based on animation duration
    return GetCurrentStateTime(layer_name);
}

void StateMachineRuntime::Update(float delta_time) {
    if (!m_resource) return;

    for (auto& pair : m_layer_runtimes) {
        const std::string& layer_name = pair.first;
        LayerRuntime& runtime = pair.second;

        if (runtime.is_playing) {
            UpdateLayer(layer_name, runtime, delta_time);
        }
    }

    // Reset triggers after update
    for (auto& pair : m_parameter_values) {
        if (pair.second.type == ConditionType::Trigger) {
            pair.second.bool_value = false;
        }
    }
}

bool StateMachineRuntime::EvaluateConditions(const std::vector<TransitionCondition>& conditions) const {
    if (conditions.empty()) return true; // No conditions means always true

    for (const auto& condition : conditions) {
        if (!EvaluateCondition(condition)) {
            return false; // All conditions must be true
        }
    }
    return true;
}

bool StateMachineRuntime::EvaluateCondition(const TransitionCondition& condition) const {
    auto it = m_parameter_values.find(condition.parameter_name);
    if (it == m_parameter_values.end()) return false;

    const ParameterValue& param_value = it->second;
    const ParameterValue& condition_value = condition.value;

    if (param_value.type != condition_value.type) return false;

    switch (param_value.type) {
        case ConditionType::Bool:
        case ConditionType::Trigger:
            switch (condition.operator_type) {
                case ComparisonOperator::Equals:
                    return param_value.bool_value == condition_value.bool_value;
                case ComparisonOperator::NotEquals:
                    return param_value.bool_value != condition_value.bool_value;
                default:
                    return false;
            }
            break;

        case ConditionType::Int:
            switch (condition.operator_type) {
                case ComparisonOperator::Equals:
                    return param_value.int_value == condition_value.int_value;
                case ComparisonOperator::NotEquals:
                    return param_value.int_value != condition_value.int_value;
                case ComparisonOperator::Greater:
                    return param_value.int_value > condition_value.int_value;
                case ComparisonOperator::GreaterEqual:
                    return param_value.int_value >= condition_value.int_value;
                case ComparisonOperator::Less:
                    return param_value.int_value < condition_value.int_value;
                case ComparisonOperator::LessEqual:
                    return param_value.int_value <= condition_value.int_value;
            }
            break;

        case ConditionType::Float:
            switch (condition.operator_type) {
                case ComparisonOperator::Equals:
                    return std::abs(param_value.float_value - condition_value.float_value) < 0.001f;
                case ComparisonOperator::NotEquals:
                    return std::abs(param_value.float_value - condition_value.float_value) >= 0.001f;
                case ComparisonOperator::Greater:
                    return param_value.float_value > condition_value.float_value;
                case ComparisonOperator::GreaterEqual:
                    return param_value.float_value >= condition_value.float_value;
                case ComparisonOperator::Less:
                    return param_value.float_value < condition_value.float_value;
                case ComparisonOperator::LessEqual:
                    return param_value.float_value <= condition_value.float_value;
            }
            break;
    }

    return false;
}

void StateMachineRuntime::CheckTransitions(const std::string& layer_name, LayerRuntime& runtime) {
    if (!m_resource || runtime.is_transitioning) return;

    auto transitions = m_resource->GetTransitionsFromState(layer_name, runtime.current_state);

    for (auto* transition : transitions) {
        if (!transition->can_transition_to_self && transition->to_state == runtime.current_state) {
            continue; // Skip self-transitions if not allowed
        }

        bool can_transition = true;

        // Check exit time condition
        if (transition->has_exit_time) {
            float normalized_time = 0.0f;

            // Calculate normalized time based on animation duration
            if (m_animation_duration_provider && m_resource) {
                const auto* layer = m_resource->GetLayer(layer_name);
                if (layer) {
                    // Find current state to get its animation clip
                    for (const auto& state : layer->states) {
                        if (state.name == runtime.current_state) {
                            float animation_duration = m_animation_duration_provider->GetAnimationDuration(state.animation_clip);
                            if (animation_duration > 0.0f) {
                                normalized_time = runtime.state_time / animation_duration;
                            }
                            break;
                        }
                    }
                }
            } else {
                // Fallback: assume 1 second duration
                normalized_time = runtime.state_time;
            }

            if (normalized_time < transition->exit_time) {
                can_transition = false;
            }
        }

        // Check parameter conditions
        if (can_transition && !EvaluateConditions(transition->conditions)) {
            can_transition = false;
        }

        if (can_transition) {
            // Start transition
            runtime.next_state = transition->to_state;
            runtime.transition_duration = transition->transition_duration;
            runtime.transition_time = 0.0f;
            runtime.is_transitioning = true;
            break; // Take first valid transition
        }
    }
}

void StateMachineRuntime::UpdateLayer(const std::string& layer_name, LayerRuntime& runtime, float delta_time) {
    if (runtime.is_transitioning) {
        runtime.transition_time += delta_time;

        if (runtime.transition_time >= runtime.transition_duration) {
            // Transition complete
            runtime.current_state = runtime.next_state;
            runtime.next_state.clear();
            runtime.state_time = 0.0f;
            runtime.is_transitioning = false;
        }
    } else {
        runtime.state_time += delta_time;
        CheckTransitions(layer_name, runtime);
    }
}

} // namespace Lupine
