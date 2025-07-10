#include "lupine/components/StateAnimator.h"
#include "lupine/core/Node.h"
#include "lupine/core/Scene.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include "lupine/components/Sprite2D.h"
#include "lupine/components/Label.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Lupine {

StateAnimator::StateAnimator()
    : Component("StateAnimator")
    , m_playback_state(StateAnimatorPlaybackState::Stopped)
    , m_auto_play(false)
{
    InitializeExportVariables();
}

void StateAnimator::InitializeExportVariables() {
    AddExportVariable("state_machine_resource", ExportValue(m_state_machine_resource_path), 
                     "State machine resource file (.statemachine)", ExportVariableType::FilePath);
    AddExportVariable("animation_resource", ExportValue(m_animation_resource_path), 
                     "Animation resource file (.anim)", ExportVariableType::FilePath);
    AddExportVariable("auto_play", ExportValue(m_auto_play), 
                     "Automatically start playing when ready", ExportVariableType::Bool);
}

void StateAnimator::SetStateMachineResource(const std::string& filepath) {
    if (m_state_machine_resource_path != filepath) {
        m_state_machine_resource_path = filepath;
        LoadStateMachineResource();
    }
}

void StateAnimator::SetAnimationResource(const std::string& filepath) {
    if (m_animation_resource_path != filepath) {
        m_animation_resource_path = filepath;
        LoadAnimationResource();
    }
}

void StateAnimator::Play() {
    if (!m_state_machine_runtime) {
        std::cerr << "StateAnimator: No state machine resource loaded" << std::endl;
        return;
    }
    
    m_playback_state = StateAnimatorPlaybackState::Playing;
    m_state_machine_runtime->Play();
    
    std::cout << "StateAnimator: Started playing state machine" << std::endl;
}

void StateAnimator::Stop() {
    if (m_state_machine_runtime) {
        m_state_machine_runtime->Stop();
    }
    
    m_playback_state = StateAnimatorPlaybackState::Stopped;
    m_layer_animation_states.clear();
    
    std::cout << "StateAnimator: Stopped state machine" << std::endl;
}

void StateAnimator::Pause() {
    if (m_playback_state == StateAnimatorPlaybackState::Playing) {
        m_playback_state = StateAnimatorPlaybackState::Paused;
        if (m_state_machine_runtime) {
            m_state_machine_runtime->Pause();
        }
    }
}

void StateAnimator::Resume() {
    if (m_playback_state == StateAnimatorPlaybackState::Paused) {
        m_playback_state = StateAnimatorPlaybackState::Playing;
        if (m_state_machine_runtime) {
            m_state_machine_runtime->Resume();
        }
    }
}

void StateAnimator::PlayState(const std::string& state_name, const std::string& layer_name) {
    if (!m_state_machine_resource) {
        std::cerr << "StateAnimator: No state machine resource loaded" << std::endl;
        return;
    }
    
    std::string target_layer = layer_name.empty() ? "Base Layer" : layer_name;
    const auto* layer = m_state_machine_resource->GetLayer(target_layer);
    if (!layer) {
        std::cerr << "StateAnimator: Layer '" << target_layer << "' not found" << std::endl;
        return;
    }
    
    // Find the state
    bool state_found = false;
    for (const auto& state : layer->states) {
        if (state.name == state_name) {
            state_found = true;
            break;
        }
    }
    
    if (!state_found) {
        std::cerr << "StateAnimator: State '" << state_name << "' not found in layer '" << target_layer << "'" << std::endl;
        return;
    }
    
    // Force transition to the state (this would require extending StateMachineRuntime)
    // For now, we'll just start the animation directly
    StartStateAnimation(target_layer, state_name);
}

std::string StateAnimator::GetCurrentState(const std::string& layer_name) const {
    if (!m_state_machine_runtime) return "";
    return m_state_machine_runtime->GetCurrentState(layer_name);
}

float StateAnimator::GetCurrentStateTime(const std::string& layer_name) const {
    if (!m_state_machine_runtime) return 0.0f;
    return m_state_machine_runtime->GetCurrentStateTime(layer_name);
}

float StateAnimator::GetCurrentStateNormalizedTime(const std::string& layer_name) const {
    if (!m_state_machine_runtime) return 0.0f;
    return m_state_machine_runtime->GetCurrentStateNormalizedTime(layer_name);
}

bool StateAnimator::IsInTransition(const std::string& layer_name) const {
    auto it = m_layer_animation_states.find(layer_name.empty() ? "Base Layer" : layer_name);
    if (it != m_layer_animation_states.end()) {
        return it->second.is_transitioning;
    }
    return false;
}

void StateAnimator::SetBool(const std::string& name, bool value) {
    if (m_state_machine_runtime) {
        m_state_machine_runtime->SetBool(name, value);
    }
}

void StateAnimator::SetInt(const std::string& name, int value) {
    if (m_state_machine_runtime) {
        m_state_machine_runtime->SetInt(name, value);
    }
}

void StateAnimator::SetFloat(const std::string& name, float value) {
    if (m_state_machine_runtime) {
        m_state_machine_runtime->SetFloat(name, value);
    }
}

void StateAnimator::SetTrigger(const std::string& name) {
    if (m_state_machine_runtime) {
        m_state_machine_runtime->SetTrigger(name);
    }
}

bool StateAnimator::GetBool(const std::string& name) const {
    if (m_state_machine_runtime) {
        return m_state_machine_runtime->GetBool(name);
    }
    return false;
}

int StateAnimator::GetInt(const std::string& name) const {
    if (m_state_machine_runtime) {
        return m_state_machine_runtime->GetInt(name);
    }
    return 0;
}

float StateAnimator::GetFloat(const std::string& name) const {
    if (m_state_machine_runtime) {
        return m_state_machine_runtime->GetFloat(name);
    }
    return 0.0f;
}

std::vector<std::string> StateAnimator::GetAvailableStates(const std::string& layer_name) const {
    std::vector<std::string> states;
    if (!m_state_machine_resource) return states;
    
    std::string target_layer = layer_name.empty() ? "Base Layer" : layer_name;
    const auto* layer = m_state_machine_resource->GetLayer(target_layer);
    if (layer) {
        for (const auto& state : layer->states) {
            states.push_back(state.name);
        }
    }
    return states;
}

std::vector<std::string> StateAnimator::GetAvailableParameters() const {
    if (!m_state_machine_resource) return {};
    return m_state_machine_resource->GetParameterNames();
}

std::vector<std::string> StateAnimator::GetAvailableLayers() const {
    if (!m_state_machine_resource) return {};
    return m_state_machine_resource->GetLayerNames();
}

void StateAnimator::OnReady() {
    UpdateFromExportVariables();
    
    if (m_auto_play && m_state_machine_runtime) {
        Play();
    }
}

void StateAnimator::OnUpdate(float delta_time) {
    UpdateFromExportVariables();
    
    if (m_playback_state == StateAnimatorPlaybackState::Playing) {
        UpdateStateMachine(delta_time);
        UpdateAnimations(delta_time);
        ApplyCurrentAnimations();
    }
}

void StateAnimator::LoadStateMachineResource() {
    m_state_machine_resource.reset();
    m_state_machine_runtime.reset();
    
    if (m_state_machine_resource_path.empty()) {
        return;
    }
    
    m_state_machine_resource = std::make_shared<StateMachineResource>();
    if (!m_state_machine_resource->LoadFromFile(m_state_machine_resource_path)) {
        std::cerr << "StateAnimator: Failed to load state machine resource: " << m_state_machine_resource_path << std::endl;
        m_state_machine_resource.reset();
        return;
    }
    
    m_state_machine_runtime = std::make_unique<StateMachineRuntime>();
    m_state_machine_runtime->SetResource(m_state_machine_resource);
    m_state_machine_runtime->SetAnimationDurationProvider(this);
    
    // Initialize layer animation states
    m_layer_animation_states.clear();
    for (const auto& layer_name : m_state_machine_resource->GetLayerNames()) {
        LayerAnimationState layer_state;
        layer_state.animation_time = 0.0f;
        layer_state.animation_speed = 1.0f;
        layer_state.is_transitioning = false;
        layer_state.transition_progress = 0.0f;
        layer_state.transition_duration = 0.0f;
        m_layer_animation_states[layer_name] = layer_state;
    }
    
    std::cout << "StateAnimator: Loaded state machine resource: " << m_state_machine_resource_path << std::endl;
}

void StateAnimator::LoadAnimationResource() {
    m_animation_resource.reset();
    
    if (m_animation_resource_path.empty()) {
        return;
    }
    
    m_animation_resource = std::make_unique<TweenAnimationResource>();
    if (!m_animation_resource->LoadFromFile(m_animation_resource_path)) {
        std::cerr << "StateAnimator: Failed to load animation resource: " << m_animation_resource_path << std::endl;
        m_animation_resource.reset();
    } else {
        std::cout << "StateAnimator: Loaded animation resource: " << m_animation_resource_path << std::endl;
    }
}

void StateAnimator::UpdateFromExportVariables() {
    std::string new_state_machine_path = GetExportVariableValue<std::string>("state_machine_resource", m_state_machine_resource_path);
    if (new_state_machine_path != m_state_machine_resource_path) {
        SetStateMachineResource(new_state_machine_path);
    }

    std::string new_animation_path = GetExportVariableValue<std::string>("animation_resource", m_animation_resource_path);
    if (new_animation_path != m_animation_resource_path) {
        SetAnimationResource(new_animation_path);
    }

    m_auto_play = GetExportVariableValue<bool>("auto_play", m_auto_play);
}

void StateAnimator::UpdateStateMachine(float delta_time) {
    if (!m_state_machine_runtime) return;

    // Store previous states to detect changes
    std::map<std::string, std::string> previous_states;
    for (const auto& layer_name : m_state_machine_resource->GetLayerNames()) {
        previous_states[layer_name] = m_state_machine_runtime->GetCurrentState(layer_name);
    }

    // Update the state machine
    m_state_machine_runtime->Update(delta_time);

    // Check for state changes and handle them
    for (const auto& layer_name : m_state_machine_resource->GetLayerNames()) {
        std::string current_state = m_state_machine_runtime->GetCurrentState(layer_name);
        std::string previous_state = previous_states[layer_name];

        if (current_state != previous_state && !current_state.empty()) {
            OnStateChanged(layer_name, previous_state, current_state);
        }
    }
}

void StateAnimator::UpdateAnimations(float delta_time) {
    for (auto& pair : m_layer_animation_states) {
        LayerAnimationState& layer_state = pair.second;

        if (layer_state.is_transitioning) {
            // Update transition progress
            float transition_duration = layer_state.transition_duration > 0.0f ? layer_state.transition_duration : 0.25f;
            layer_state.transition_progress += delta_time / transition_duration;

            if (layer_state.transition_progress >= 1.0f) {
                // Transition complete
                layer_state.current_animation_clip = layer_state.next_animation_clip;
                layer_state.next_animation_clip.clear();
                layer_state.is_transitioning = false;
                layer_state.transition_progress = 0.0f;
                layer_state.transition_duration = 0.0f;
                layer_state.animation_time = 0.0f;
            }
        }

        // Update animation time
        if (!layer_state.current_animation_clip.empty()) {
            layer_state.animation_time += delta_time * layer_state.animation_speed;

            // Handle looping
            float duration = GetAnimationDuration(layer_state.current_animation_clip);
            if (duration > 0.0f && layer_state.animation_time >= duration) {
                layer_state.animation_time = std::fmod(layer_state.animation_time, duration);
            }
        }
    }
}

void StateAnimator::ApplyCurrentAnimations() {
    for (const auto& pair : m_layer_animation_states) {
        const std::string& layer_name = pair.first;
        const LayerAnimationState& layer_state = pair.second;

        if (layer_state.is_transitioning) {
            ApplyAnimationBlending(layer_name, layer_state);
        } else if (!layer_state.current_animation_clip.empty()) {
            ApplyAnimationAtTime(layer_state.current_animation_clip, layer_state.animation_time);
        }
    }
}

void StateAnimator::ApplyAnimationBlending(const std::string& /* layer_name */, const LayerAnimationState& layer_state) {
    if (layer_state.current_animation_clip.empty() || layer_state.next_animation_clip.empty()) {
        return;
    }

    float blend_factor = layer_state.transition_progress;
    BlendAnimations(layer_state.current_animation_clip, layer_state.animation_time,
                   layer_state.next_animation_clip, 0.0f, blend_factor);
}

void StateAnimator::OnStateChanged(const std::string& layer_name, const std::string& old_state, const std::string& new_state) {
    std::cout << "StateAnimator: State changed in layer '" << layer_name << "' from '" << old_state << "' to '" << new_state << "'" << std::endl;

    // Find the new state and start its animation
    const auto* layer = m_state_machine_resource->GetLayer(layer_name);
    if (layer) {
        for (const auto& state : layer->states) {
            if (state.name == new_state) {
                if (!old_state.empty()) {
                    // Find transition duration from state machine
                    float transition_duration = 0.25f; // Default
                    auto transitions = m_state_machine_resource->GetTransitionsFromState(layer_name, old_state);
                    for (auto* transition : transitions) {
                        if (transition->to_state == new_state) {
                            transition_duration = transition->transition_duration;
                            break;
                        }
                    }

                    // Start transition
                    StartTransition(layer_name, old_state, new_state, transition_duration);
                } else {
                    // Direct state change
                    StartStateAnimation(layer_name, new_state);
                }
                break;
            }
        }
    }
}

void StateAnimator::StartStateAnimation(const std::string& layer_name, const std::string& state_name) {
    const auto* layer = m_state_machine_resource->GetLayer(layer_name);
    if (!layer) return;

    // Find the state
    for (const auto& state : layer->states) {
        if (state.name == state_name) {
            auto& layer_state = m_layer_animation_states[layer_name];
            layer_state.current_animation_clip = state.animation_clip;
            layer_state.animation_time = 0.0f;
            layer_state.animation_speed = state.speed;
            layer_state.is_transitioning = false;
            layer_state.transition_progress = 0.0f;
            break;
        }
    }
}

void StateAnimator::StartTransition(const std::string& layer_name, const std::string& /* from_state */, const std::string& to_state, float duration) {
    const auto* layer = m_state_machine_resource->GetLayer(layer_name);
    if (!layer) return;

    // Find the target state
    for (const auto& state : layer->states) {
        if (state.name == to_state) {
            auto& layer_state = m_layer_animation_states[layer_name];
            layer_state.next_animation_clip = state.animation_clip;
            layer_state.is_transitioning = true;
            layer_state.transition_progress = 0.0f;
            layer_state.transition_duration = duration;
            break;
        }
    }
}

float StateAnimator::GetAnimationDuration(const std::string& animation_clip) const {
    if (!m_animation_resource || animation_clip.empty()) return 1.0f; // Default 1 second

    const auto* clip = m_animation_resource->GetClip(animation_clip);
    return clip ? clip->duration : 1.0f; // Default 1 second if clip not found
}

float StateAnimator::CalculateNormalizedTime(const std::string& animation_clip, float time) const {
    float duration = GetAnimationDuration(animation_clip);
    return (duration > 0.0f) ? (time / duration) : 0.0f;
}

void StateAnimator::ApplyAnimationAtTime(const std::string& animation_clip, float time, float weight) {
    if (!m_animation_resource || animation_clip.empty()) return;

    const AnimationClip* clip = m_animation_resource->GetClip(animation_clip);
    if (!clip) return;

    // Apply all tracks
    for (const auto& track : clip->tracks) {
        ApplyTrackAtTime(track, time, weight);
    }
}

void StateAnimator::ApplyTrackAtTime(const AnimationTrack& track, float time, float weight) {
    if (track.keyframes.empty()) return;

    // Find the node
    Node* target_node = FindNodeByPath(track.node_path);
    if (!target_node) {
        return; // Node not found, skip this track
    }

    // Interpolate value at current time
    AnimationValue value = InterpolateKeyframes(track, time);

    // Apply the value to the node property
    SetNodeProperty(target_node, track.property_name, value, weight);
}

void StateAnimator::BlendAnimations(const std::string& clip_a, float time_a, const std::string& clip_b, float time_b, float blend_factor) {
    if (!m_animation_resource || clip_a.empty() || clip_b.empty()) return;

    const AnimationClip* clipA = m_animation_resource->GetClip(clip_a);
    const AnimationClip* clipB = m_animation_resource->GetClip(clip_b);
    if (!clipA || !clipB) return;

    // Create a map of all animated properties
    std::map<std::pair<std::string, std::string>, std::pair<const AnimationTrack*, const AnimationTrack*>> track_pairs;

    // Map tracks from clip A
    for (const auto& track : clipA->tracks) {
        auto key = std::make_pair(track.node_path, track.property_name);
        track_pairs[key].first = &track;
    }

    // Map tracks from clip B
    for (const auto& track : clipB->tracks) {
        auto key = std::make_pair(track.node_path, track.property_name);
        track_pairs[key].second = &track;
    }

    // Apply blended values
    for (const auto& pair : track_pairs) {
        const std::string& node_path = pair.first.first;
        const std::string& property_name = pair.first.second;
        const AnimationTrack* trackA = pair.second.first;
        const AnimationTrack* trackB = pair.second.second;

        Node* target_node = FindNodeByPath(node_path);
        if (!target_node) continue;

        AnimationValue valueA, valueB;

        if (trackA) {
            valueA = InterpolateKeyframes(*trackA, time_a);
        } else {
            // Get current value as fallback
            valueA = GetNodeProperty(target_node, property_name);
        }

        if (trackB) {
            valueB = InterpolateKeyframes(*trackB, time_b);
        } else {
            // Get current value as fallback
            valueB = GetNodeProperty(target_node, property_name);
        }

        // Blend the values
        AnimationValue blended_value = BlendAnimationValues(valueA, valueB, blend_factor);
        SetNodeProperty(target_node, property_name, blended_value);
    }
}

// Copy methods from Animator component for node property access
Node* StateAnimator::FindNodeByPath(const std::string& path) {
    if (!m_owner || !m_owner->GetScene()) return nullptr;

    Node* root = m_owner->GetScene()->GetRootNode();
    if (!root) return nullptr;

    // Split path by '/'
    std::vector<std::string> pathParts;
    std::string currentPart;
    for (char c : path) {
        if (c == '/') {
            if (!currentPart.empty()) {
                pathParts.push_back(currentPart);
                currentPart.clear();
            }
        } else {
            currentPart += c;
        }
    }
    if (!currentPart.empty()) {
        pathParts.push_back(currentPart);
    }

    if (pathParts.empty()) return nullptr;

    Node* current = root;

    // Skip root if path starts with root name
    size_t startIndex = 0;
    if (!pathParts.empty() && pathParts[0] == current->GetName()) {
        startIndex = 1;
    }

    for (size_t i = startIndex; i < pathParts.size(); ++i) {
        const std::string& partName = pathParts[i];
        bool found = false;

        for (const auto& child : current->GetChildren()) {
            if (child->GetName() == partName) {
                current = child.get();
                found = true;
                break;
            }
        }

        if (!found) return nullptr;
    }

    return current;
}

void StateAnimator::SetNodeProperty(Node* node, const std::string& property_name, const AnimationValue& value, float weight) {
    if (!node) return;

    // Handle node transform properties
    if (property_name == "position") {
        if (auto* node2d = dynamic_cast<Node2D*>(node)) {
            if (value.type == AnimationPropertyType::Vec2) {
                if (weight >= 1.0f) {
                    node2d->SetPosition(value.vec2_value);
                } else {
                    // Blend with current position
                    glm::vec2 current = node2d->GetPosition();
                    glm::vec2 blended = current * (1.0f - weight) + value.vec2_value * weight;
                    node2d->SetPosition(blended);
                }
            }
        } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
            if (value.type == AnimationPropertyType::Vec3) {
                if (weight >= 1.0f) {
                    node3d->SetPosition(value.vec3_value);
                } else {
                    // Blend with current position
                    glm::vec3 current = node3d->GetPosition();
                    glm::vec3 blended = current * (1.0f - weight) + value.vec3_value * weight;
                    node3d->SetPosition(blended);
                }
            }
        } else if (auto* control = dynamic_cast<Control*>(node)) {
            if (value.type == AnimationPropertyType::Vec2) {
                if (weight >= 1.0f) {
                    control->SetPosition(value.vec2_value);
                } else {
                    // Blend with current position
                    glm::vec2 current = control->GetPosition();
                    glm::vec2 blended = current * (1.0f - weight) + value.vec2_value * weight;
                    control->SetPosition(blended);
                }
            }
        }
    } else if (property_name == "rotation") {
        if (auto* node2d = dynamic_cast<Node2D*>(node)) {
            if (value.type == AnimationPropertyType::Float) {
                if (weight >= 1.0f) {
                    node2d->SetRotation(value.float_value);
                } else {
                    // Blend with current rotation
                    float current = node2d->GetRotation();
                    float blended = current * (1.0f - weight) + value.float_value * weight;
                    node2d->SetRotation(blended);
                }
            }
        } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
            if (value.type == AnimationPropertyType::Quaternion) {
                if (weight >= 1.0f) {
                    node3d->SetRotation(value.quat_value);
                } else {
                    // Slerp with current rotation
                    glm::quat current = node3d->GetRotation();
                    glm::quat blended = glm::slerp(current, value.quat_value, weight);
                    node3d->SetRotation(blended);
                }
            } else if (value.type == AnimationPropertyType::Vec3) {
                // Convert Euler angles to quaternion
                glm::quat quat = glm::quat(glm::radians(value.vec3_value));
                if (weight >= 1.0f) {
                    node3d->SetRotation(quat);
                } else {
                    // Slerp with current rotation
                    glm::quat current = node3d->GetRotation();
                    glm::quat blended = glm::slerp(current, quat, weight);
                    node3d->SetRotation(blended);
                }
            }
        }
    } else if (property_name == "scale") {
        if (auto* node2d = dynamic_cast<Node2D*>(node)) {
            if (value.type == AnimationPropertyType::Vec2) {
                if (weight >= 1.0f) {
                    node2d->SetScale(value.vec2_value);
                } else {
                    // Blend with current scale
                    glm::vec2 current = node2d->GetScale();
                    glm::vec2 blended = current * (1.0f - weight) + value.vec2_value * weight;
                    node2d->SetScale(blended);
                }
            }
        } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
            if (value.type == AnimationPropertyType::Vec3) {
                if (weight >= 1.0f) {
                    node3d->SetScale(value.vec3_value);
                } else {
                    // Blend with current scale
                    glm::vec3 current = node3d->GetScale();
                    glm::vec3 blended = current * (1.0f - weight) + value.vec3_value * weight;
                    node3d->SetScale(blended);
                }
            }
        }
    } else if (property_name == "size") {
        if (auto* control = dynamic_cast<Control*>(node)) {
            if (value.type == AnimationPropertyType::Vec2) {
                if (weight >= 1.0f) {
                    control->SetSize(value.vec2_value);
                } else {
                    // Blend with current size
                    glm::vec2 current = control->GetSize();
                    glm::vec2 blended = current * (1.0f - weight) + value.vec2_value * weight;
                    control->SetSize(blended);
                }
            }
        }
    }
}

AnimationValue StateAnimator::GetNodeProperty(Node* node, const std::string& property_name) {
    if (!node) return AnimationValue(0.0f);

    // Handle node transform properties
    if (property_name == "position") {
        if (auto* node2d = dynamic_cast<Node2D*>(node)) {
            return AnimationValue(node2d->GetPosition());
        } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
            return AnimationValue(node3d->GetPosition());
        } else if (auto* control = dynamic_cast<Control*>(node)) {
            return AnimationValue(control->GetPosition());
        }
    } else if (property_name == "rotation") {
        if (auto* node2d = dynamic_cast<Node2D*>(node)) {
            return AnimationValue(node2d->GetRotation());
        } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
            return AnimationValue(node3d->GetRotation());
        }
    } else if (property_name == "scale") {
        if (auto* node2d = dynamic_cast<Node2D*>(node)) {
            return AnimationValue(node2d->GetScale());
        } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
            return AnimationValue(node3d->GetScale());
        }
    } else if (property_name == "size") {
        if (auto* control = dynamic_cast<Control*>(node)) {
            return AnimationValue(control->GetSize());
        }
    }

    return AnimationValue(0.0f);
}

AnimationValue StateAnimator::BlendAnimationValues(const AnimationValue& a, const AnimationValue& b, float t) {
    if (a.type != b.type) return a; // Can't blend different types

    switch (a.type) {
        case AnimationPropertyType::Float:
            return AnimationValue(a.float_value * (1.0f - t) + b.float_value * t);
        case AnimationPropertyType::Vec2:
            return AnimationValue(a.vec2_value * (1.0f - t) + b.vec2_value * t);
        case AnimationPropertyType::Vec3:
            return AnimationValue(a.vec3_value * (1.0f - t) + b.vec3_value * t);
        case AnimationPropertyType::Vec4:
        case AnimationPropertyType::Color:
            return AnimationValue(a.vec4_value * (1.0f - t) + b.vec4_value * t);
        case AnimationPropertyType::Quaternion:
            return AnimationValue(glm::slerp(a.quat_value, b.quat_value, t));
        case AnimationPropertyType::Bool:
            return AnimationValue(t < 0.5f ? a.bool_value : b.bool_value);
        case AnimationPropertyType::Int:
            return AnimationValue(static_cast<int>(a.int_value * (1.0f - t) + b.int_value * t));
        default:
            return a;
    }
}

AnimationValue StateAnimator::InterpolateKeyframes(const AnimationTrack& track, float time) {
    if (track.keyframes.empty()) {
        return AnimationValue(0.0f);
    }

    if (track.keyframes.size() == 1) {
        return track.keyframes[0].value;
    }

    // Find the keyframes to interpolate between
    size_t keyframe_index = 0;
    for (size_t i = 0; i < track.keyframes.size() - 1; ++i) {
        if (time >= track.keyframes[i].time && time <= track.keyframes[i + 1].time) {
            keyframe_index = i;
            break;
        }
    }

    // Handle time outside keyframe range
    if (time <= track.keyframes[0].time) {
        return track.keyframes[0].value;
    }
    if (time >= track.keyframes.back().time) {
        return track.keyframes.back().value;
    }

    const AnimationKeyframe& keyframe_a = track.keyframes[keyframe_index];
    const AnimationKeyframe& keyframe_b = track.keyframes[keyframe_index + 1];

    // Calculate interpolation factor
    float duration = keyframe_b.time - keyframe_a.time;
    float t = (duration > 0.0f) ? ((time - keyframe_a.time) / duration) : 0.0f;

    // Apply interpolation curve
    t = ApplyInterpolationCurve(t, keyframe_a.interpolation);

    // Interpolate between values
    return InterpolateValues(keyframe_a.value, keyframe_b.value, t, keyframe_a.interpolation);
}

AnimationValue StateAnimator::InterpolateValues(const AnimationValue& a, const AnimationValue& b, float t, InterpolationType interpolation) {
    float curved_t = ApplyInterpolationCurve(t, interpolation);

    switch (a.type) {
        case AnimationPropertyType::Float:
            return AnimationValue(a.float_value + (b.float_value - a.float_value) * curved_t);
        case AnimationPropertyType::Vec2:
            return AnimationValue(a.vec2_value + (b.vec2_value - a.vec2_value) * curved_t);
        case AnimationPropertyType::Vec3:
            return AnimationValue(a.vec3_value + (b.vec3_value - a.vec3_value) * curved_t);
        case AnimationPropertyType::Vec4:
        case AnimationPropertyType::Color:
            return AnimationValue(a.vec4_value + (b.vec4_value - a.vec4_value) * curved_t);
        case AnimationPropertyType::Quaternion:
            return AnimationValue(glm::slerp(a.quat_value, b.quat_value, curved_t));
        case AnimationPropertyType::Bool:
            return AnimationValue(curved_t < 0.5f ? a.bool_value : b.bool_value);
        case AnimationPropertyType::Int:
            return AnimationValue(static_cast<int>(a.int_value + (b.int_value - a.int_value) * curved_t));
        default:
            return a;
    }
}

float StateAnimator::ApplyInterpolationCurve(float t, InterpolationType interpolation) {
    switch (interpolation) {
        case InterpolationType::Linear:
            return t;
        case InterpolationType::Ease:
            return t * t * (3.0f - 2.0f * t); // Smoothstep
        case InterpolationType::EaseIn:
            return t * t;
        case InterpolationType::EaseOut:
            return 1.0f - (1.0f - t) * (1.0f - t);
        case InterpolationType::EaseInOut:
            return t < 0.5f ? 2.0f * t * t : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
        case InterpolationType::Bounce:
            // Simplified bounce curve
            return 1.0f - std::abs(std::sin(t * 3.14159f * 4.0f)) * (1.0f - t);
        case InterpolationType::Elastic:
            // Simplified elastic curve
            return std::sin(t * 3.14159f * 8.0f) * (1.0f - t) + t;
        default:
            return t;
    }
}

} // namespace Lupine
