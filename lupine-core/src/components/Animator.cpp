#include "lupine/components/Animator.h"
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

Animator::Animator()
    : Component("Animator")
    , m_playback_state(AnimationPlaybackState::Stopped)
    , m_current_time(0.0f)
    , m_playback_speed(1.0f)
    , m_loop_mode(AnimationLoopMode::Loop)
    , m_ping_pong_reverse(false)
    , m_auto_play(false)
{
    InitializeExportVariables();
}

void Animator::InitializeExportVariables() {
    AddExportVariable("animation_resource", m_animation_resource_path, "Path to .anim resource file", ExportVariableType::String);
    AddExportVariable("auto_play", m_auto_play, "Automatically play animation on ready", ExportVariableType::Bool);
    AddExportVariable("auto_play_clip", m_auto_play_clip, "Animation clip to auto-play", ExportVariableType::String);
    AddExportVariable("playback_speed", m_playback_speed, "Animation playback speed multiplier", ExportVariableType::Float);
}

void Animator::SetAnimationResource(const std::string& filepath) {
    m_animation_resource_path = filepath;
    LoadAnimationResource();
    
    // Update export variable
    SetExportVariable("animation_resource", m_animation_resource_path);
}

void Animator::Play(const std::string& clip_name, AnimationLoopMode loop_mode) {
    if (!m_animation_resource) {
        std::cerr << "Animator: No animation resource loaded" << std::endl;
        return;
    }
    
    std::string target_clip = clip_name;
    if (target_clip.empty()) {
        // Use auto-play clip or first available clip
        if (!m_auto_play_clip.empty()) {
            target_clip = m_auto_play_clip;
        } else {
            auto clips = GetAvailableClips();
            if (!clips.empty()) {
                target_clip = clips[0];
            }
        }
    }
    
    if (target_clip.empty()) {
        std::cerr << "Animator: No animation clip specified or available" << std::endl;
        return;
    }
    
    const AnimationClip* clip = m_animation_resource->GetClip(target_clip);
    if (!clip) {
        std::cerr << "Animator: Animation clip '" << target_clip << "' not found" << std::endl;
        return;
    }
    
    m_current_clip = target_clip;
    m_loop_mode = loop_mode;
    m_playback_state = AnimationPlaybackState::Playing;
    m_current_time = 0.0f;
    m_ping_pong_reverse = false;
    
    std::cout << "Animator: Playing animation '" << target_clip << "'" << std::endl;
}

void Animator::Stop() {
    m_playback_state = AnimationPlaybackState::Stopped;
    m_current_time = 0.0f;
    m_ping_pong_reverse = false;
    
    // Apply animation at time 0 to reset to initial state
    if (!m_current_clip.empty()) {
        ApplyAnimationAtTime(0.0f);
    }
}

void Animator::Pause() {
    if (m_playback_state == AnimationPlaybackState::Playing) {
        m_playback_state = AnimationPlaybackState::Paused;
    }
}

void Animator::Resume() {
    if (m_playback_state == AnimationPlaybackState::Paused) {
        m_playback_state = AnimationPlaybackState::Playing;
    }
}

float Animator::GetCurrentNormalizedTime() const {
    float duration = GetClipDuration();
    return (duration > 0.0f) ? (m_current_time / duration) : 0.0f;
}

float Animator::GetClipDuration(const std::string& clip_name) const {
    if (!m_animation_resource) return 0.0f;
    
    std::string target_clip = clip_name.empty() ? m_current_clip : clip_name;
    const AnimationClip* clip = m_animation_resource->GetClip(target_clip);
    return clip ? clip->duration : 0.0f;
}

void Animator::SetTime(float time) {
    m_current_time = std::max(0.0f, time);
    ApplyAnimationAtTime(m_current_time);
}

void Animator::SetNormalizedTime(float normalized_time) {
    float duration = GetClipDuration();
    SetTime(normalized_time * duration);
}

std::vector<std::string> Animator::GetAvailableClips() const {
    if (!m_animation_resource) return {};
    return m_animation_resource->GetClipNames();
}

void Animator::OnReady() {
    UpdateFromExportVariables();
    LoadAnimationResource();
    
    // Auto-play if enabled
    if (m_auto_play) {
        Play(m_auto_play_clip, m_loop_mode);
    }
}

void Animator::OnUpdate(float delta_time) {
    UpdateFromExportVariables();
    
    if (m_playback_state != AnimationPlaybackState::Playing || m_current_clip.empty()) {
        return;
    }
    
    float duration = GetClipDuration();
    if (duration <= 0.0f) {
        return;
    }
    
    // Update animation time
    UpdateAnimationTime(delta_time * m_playback_speed, duration);
    
    // Apply animation at current time
    ApplyAnimationAtTime(m_current_time);
}

void Animator::LoadAnimationResource() {
    m_animation_resource.reset();
    
    if (m_animation_resource_path.empty()) {
        return;
    }
    
    m_animation_resource = std::make_unique<TweenAnimationResource>();
    if (!m_animation_resource->LoadFromFile(m_animation_resource_path)) {
        std::cerr << "Animator: Failed to load animation resource: " << m_animation_resource_path << std::endl;
        m_animation_resource.reset();
    } else {
        std::cout << "Animator: Loaded animation resource: " << m_animation_resource_path << std::endl;
    }
}

void Animator::UpdateFromExportVariables() {
    // Update internal state from export variables
    std::string new_path = GetExportVariableValue<std::string>("animation_resource", m_animation_resource_path);
    if (new_path != m_animation_resource_path) {
        SetAnimationResource(new_path);
    }

    m_auto_play = GetExportVariableValue<bool>("auto_play", m_auto_play);
    m_auto_play_clip = GetExportVariableValue<std::string>("auto_play_clip", m_auto_play_clip);
    m_playback_speed = GetExportVariableValue<float>("playback_speed", m_playback_speed);
}

void Animator::UpdateAnimationTime(float delta_time, float duration) {
    if (m_ping_pong_reverse) {
        m_current_time -= delta_time;
    } else {
        m_current_time += delta_time;
    }
    
    // Handle looping
    if (m_current_time >= duration) {
        switch (m_loop_mode) {
            case AnimationLoopMode::None:
                m_current_time = duration;
                m_playback_state = AnimationPlaybackState::Stopped;
                break;
                
            case AnimationLoopMode::Loop:
                m_current_time = std::fmod(m_current_time, duration);
                break;
                
            case AnimationLoopMode::PingPong:
                m_current_time = duration;
                m_ping_pong_reverse = true;
                break;
        }
    } else if (m_current_time < 0.0f && m_ping_pong_reverse) {
        m_current_time = 0.0f;
        m_ping_pong_reverse = false;
    }
}

void Animator::ApplyAnimationAtTime(float time) {
    if (!m_animation_resource || m_current_clip.empty()) return;
    
    const AnimationClip* clip = m_animation_resource->GetClip(m_current_clip);
    if (!clip) return;
    
    // Apply all tracks
    for (const auto& track : clip->tracks) {
        ApplyTrackAtTime(track, time);
    }
}

void Animator::ApplyTrackAtTime(const AnimationTrack& track, float time) {
    if (track.keyframes.empty()) return;
    
    // Find the node
    Node* target_node = FindNodeByPath(track.node_path);
    if (!target_node) {
        return; // Node not found, skip this track
    }
    
    // Interpolate value at current time
    AnimationValue value = InterpolateKeyframes(track, time);
    
    // Apply the value to the node property
    SetNodeProperty(target_node, track.property_name, value);
}

AnimationValue Animator::InterpolateKeyframes(const AnimationTrack& track, float time) {
    if (track.keyframes.empty()) {
        return AnimationValue(0.0f);
    }
    
    if (track.keyframes.size() == 1) {
        return track.keyframes[0].value;
    }
    
    // Find surrounding keyframes
    auto it = std::lower_bound(track.keyframes.begin(), track.keyframes.end(), time,
        [](const AnimationKeyframe& kf, float t) {
            return kf.time < t;
        });
    
    if (it == track.keyframes.begin()) {
        return track.keyframes[0].value;
    }
    
    if (it == track.keyframes.end()) {
        return track.keyframes.back().value;
    }
    
    // Interpolate between keyframes
    const AnimationKeyframe& kf1 = *(it - 1);
    const AnimationKeyframe& kf2 = *it;
    
    float t = (time - kf1.time) / (kf2.time - kf1.time);
    t = std::clamp(t, 0.0f, 1.0f);
    
    return InterpolateValues(kf1.value, kf2.value, t, kf1.interpolation);
}

AnimationValue Animator::InterpolateValues(const AnimationValue& a, const AnimationValue& b, float t, InterpolationType interpolation) {
    if (a.type != b.type) return a;
    
    // Apply interpolation curve
    float curved_t = ApplyInterpolationCurve(t, interpolation);
    
    switch (a.type) {
        case AnimationPropertyType::Float:
            return AnimationValue(a.float_value + (b.float_value - a.float_value) * curved_t);
        case AnimationPropertyType::Vec2:
            return AnimationValue(glm::mix(a.vec2_value, b.vec2_value, curved_t));
        case AnimationPropertyType::Vec3:
            return AnimationValue(glm::mix(a.vec3_value, b.vec3_value, curved_t));
        case AnimationPropertyType::Vec4:
        case AnimationPropertyType::Color:
            return AnimationValue(glm::mix(a.vec4_value, b.vec4_value, curved_t));
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

float Animator::ApplyInterpolationCurve(float t, InterpolationType interpolation) {
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

Node* Animator::FindNodeByPath(const std::string& path) {
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

void Animator::SetNodeProperty(Node* node, const std::string& property_name, const AnimationValue& value) {
    if (!node) return;

    // Handle node transform properties
    if (property_name == "position") {
        if (auto* node2d = dynamic_cast<Node2D*>(node)) {
            if (value.type == AnimationPropertyType::Vec2) {
                node2d->SetPosition(value.vec2_value);
            }
        } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
            if (value.type == AnimationPropertyType::Vec3) {
                node3d->SetPosition(value.vec3_value);
            }
        } else if (auto* control = dynamic_cast<Control*>(node)) {
            if (value.type == AnimationPropertyType::Vec2) {
                control->SetPosition(value.vec2_value);
            }
        }
    } else if (property_name == "rotation") {
        if (auto* node2d = dynamic_cast<Node2D*>(node)) {
            if (value.type == AnimationPropertyType::Float) {
                node2d->SetRotation(value.float_value);
            }
        } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
            if (value.type == AnimationPropertyType::Quaternion) {
                node3d->SetRotation(value.quat_value);
            } else if (value.type == AnimationPropertyType::Vec3) {
                // Convert Euler angles to quaternion
                glm::quat quat = glm::quat(glm::radians(value.vec3_value));
                node3d->SetRotation(quat);
            }
        }
    } else if (property_name == "scale") {
        if (auto* node2d = dynamic_cast<Node2D*>(node)) {
            if (value.type == AnimationPropertyType::Vec2) {
                node2d->SetScale(value.vec2_value);
            }
        } else if (auto* node3d = dynamic_cast<Node3D*>(node)) {
            if (value.type == AnimationPropertyType::Vec3) {
                node3d->SetScale(value.vec3_value);
            }
        }
    } else if (property_name == "size") {
        if (auto* control = dynamic_cast<Control*>(node)) {
            if (value.type == AnimationPropertyType::Vec2) {
                control->SetSize(value.vec2_value);
            }
        }
    }

    // Handle component properties
    auto components = node->GetComponents<Component>();
    for (const auto* component : components) {
        if (component->GetTypeName() == "Sprite2D") {
            auto* sprite = static_cast<const Sprite2D*>(component);
            if (property_name == "color" || property_name == "modulate") {
                if (value.type == AnimationPropertyType::Color || value.type == AnimationPropertyType::Vec4) {
                    // Note: This would require adding SetColor/SetModulate methods to Sprite2D
                    // For now, we'll skip component property animation
                }
            }
        } else if (component->GetTypeName() == "Label") {
            auto* label = static_cast<const Label*>(component);
            if (property_name == "color") {
                if (value.type == AnimationPropertyType::Color || value.type == AnimationPropertyType::Vec4) {
                    // Note: This would require adding SetColor method to Label
                    // For now, we'll skip component property animation
                }
            }
        }
    }
}

AnimationValue Animator::GetNodeProperty(Node* node, const std::string& property_name) {
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

} // namespace Lupine
