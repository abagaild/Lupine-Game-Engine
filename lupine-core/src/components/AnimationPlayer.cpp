#include "lupine/components/AnimationPlayer.h"
#include "lupine/components/SkinnedMesh.h"
#include "lupine/core/Node.h"
#include <iostream>
#include <algorithm>

namespace Lupine {

AnimationPlayer::AnimationPlayer() 
    : m_playback_state(PlaybackState::Stopped), m_loop_mode(LoopMode::Loop),
      m_current_animation(""), m_current_time(0.0f), m_playback_speed(1.0f),
      m_auto_play(false), m_auto_play_animation(""), m_ping_pong_reverse(false),
      m_skinned_mesh(nullptr) {
}

void AnimationPlayer::Play(const std::string& animation_name, LoopMode loop_mode) {
    if (!m_skinned_mesh || !m_skinned_mesh->GetModel()) {
        std::cerr << "AnimationPlayer: No SkinnedMesh component found or model not loaded" << std::endl;
        return;
    }
    
    const Model* model = m_skinned_mesh->GetModel();
    const SkeletalAnimationClip* animation = model->GetAnimation(animation_name);
    if (!animation) {
        std::cerr << "AnimationPlayer: Animation '" << animation_name << "' not found" << std::endl;
        return;
    }
    
    m_current_animation = animation_name;
    m_loop_mode = loop_mode;
    m_playback_state = PlaybackState::Playing;
    m_current_time = 0.0f;
    m_ping_pong_reverse = false;
    
    std::cout << "AnimationPlayer: Playing animation '" << animation_name << "'" << std::endl;
}

void AnimationPlayer::Stop() {
    m_playback_state = PlaybackState::Stopped;
    m_current_time = 0.0f;
    m_ping_pong_reverse = false;
}

void AnimationPlayer::Pause() {
    if (m_playback_state == PlaybackState::Playing) {
        m_playback_state = PlaybackState::Paused;
    }
}

void AnimationPlayer::Resume() {
    if (m_playback_state == PlaybackState::Paused) {
        m_playback_state = PlaybackState::Playing;
    }
}

void AnimationPlayer::Seek(float time) {
    m_current_time = std::max(0.0f, time);
    
    // Clamp to animation duration if available
    float duration = GetAnimationDuration();
    if (duration > 0.0f) {
        m_current_time = std::min(m_current_time, duration);
    }
}

float AnimationPlayer::GetAnimationDuration(const std::string& animation_name) const {
    if (!m_skinned_mesh || !m_skinned_mesh->GetModel()) {
        return 0.0f;
    }
    
    const Model* model = m_skinned_mesh->GetModel();
    std::string anim_name = animation_name.empty() ? m_current_animation : animation_name;

    const SkeletalAnimationClip* animation = model->GetAnimation(anim_name);
    if (animation && animation->ticks_per_second > 0.0f) {
        return animation->duration / animation->ticks_per_second;
    }
    
    return 0.0f;
}

void AnimationPlayer::SetAutoPlay(bool auto_play, const std::string& animation_name) {
    m_auto_play = auto_play;
    m_auto_play_animation = animation_name;
}

std::vector<std::string> AnimationPlayer::GetAvailableAnimations() const {
    std::vector<std::string> animation_names;
    
    if (m_skinned_mesh && m_skinned_mesh->GetModel()) {
        const Model* model = m_skinned_mesh->GetModel();
        const auto& animations = model->GetAnimations();
        
        for (const auto& animation : animations) {
            animation_names.push_back(animation.name);
        }
    }
    
    return animation_names;
}

void AnimationPlayer::OnReady() {
    UpdateFromExportVariables();
    FindSkinnedMeshComponent();
    
    // Auto-play if enabled
    if (m_auto_play) {
        std::string anim_to_play = m_auto_play_animation;
        
        // Use first available animation if no specific animation set
        if (anim_to_play.empty()) {
            auto available_anims = GetAvailableAnimations();
            if (!available_anims.empty()) {
                anim_to_play = available_anims[0];
            }
        }
        
        if (!anim_to_play.empty()) {
            Play(anim_to_play, m_loop_mode);
        }
    }
}

void AnimationPlayer::OnUpdate(float delta_time) {
    UpdateFromExportVariables();
    
    if (m_playback_state != PlaybackState::Playing || !m_skinned_mesh) {
        return;
    }
    
    float duration = GetAnimationDuration();
    if (duration <= 0.0f) {
        return;
    }
    
    // Update animation time
    UpdateAnimationTime(delta_time * m_playback_speed, duration);
    
    // Update bone transforms in SkinnedMesh
    m_skinned_mesh->UpdateBoneTransforms(m_current_time, m_current_animation);
}

void AnimationPlayer::InitializeExportVariables() {
    // Create enum options for loop mode
    std::vector<std::string> loopModeOptions = {
        "None", "Loop", "PingPong"
    };

    AddEnumExportVariable("loop_mode", static_cast<int>(m_loop_mode), "Animation loop mode", loopModeOptions);
    AddExportVariable("playback_speed", ExportValue(m_playback_speed), "Playback speed multiplier", ExportVariableType::Float);
    AddExportVariable("auto_play", ExportValue(m_auto_play), "Auto-play animation on ready", ExportVariableType::Bool);
    AddExportVariable("auto_play_animation", ExportValue(m_auto_play_animation), "Animation to auto-play", ExportVariableType::String);
}

void AnimationPlayer::UpdateFromExportVariables() {
    // Update loop mode
    int loop_mode_index = GetExportVariableValue<int>("loop_mode", static_cast<int>(m_loop_mode));
    m_loop_mode = static_cast<LoopMode>(loop_mode_index);

    // Update playback speed
    m_playback_speed = GetExportVariableValue<float>("playback_speed", m_playback_speed);

    // Update auto-play settings
    m_auto_play = GetExportVariableValue<bool>("auto_play", m_auto_play);
    m_auto_play_animation = GetExportVariableValue<std::string>("auto_play_animation", m_auto_play_animation);
}

void AnimationPlayer::FindSkinnedMeshComponent() {
    if (GetOwner()) {
        m_skinned_mesh = GetOwner()->GetComponent<SkinnedMesh>();
        if (!m_skinned_mesh) {
            std::cerr << "AnimationPlayer: No SkinnedMesh component found on node" << std::endl;
        }
    }
}

void AnimationPlayer::UpdateAnimationTime(float delta_time, float duration) {
    switch (m_loop_mode) {
        case LoopMode::None:
            m_current_time += delta_time;
            if (m_current_time >= duration) {
                m_current_time = duration;
                m_playback_state = PlaybackState::Stopped;
            }
            break;
            
        case LoopMode::Loop:
            m_current_time += delta_time;
            while (m_current_time >= duration) {
                m_current_time -= duration;
            }
            break;
            
        case LoopMode::PingPong:
            if (!m_ping_pong_reverse) {
                m_current_time += delta_time;
                if (m_current_time >= duration) {
                    m_current_time = duration;
                    m_ping_pong_reverse = true;
                }
            } else {
                m_current_time -= delta_time;
                if (m_current_time <= 0.0f) {
                    m_current_time = 0.0f;
                    m_ping_pong_reverse = false;
                }
            }
            break;
    }
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(AnimationPlayer, "Animation", "Controls playback of skeletal animations")
