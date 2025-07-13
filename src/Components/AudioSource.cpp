#include "lupine/components/AudioSource.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include <iostream>
#include <algorithm>

namespace Lupine {

AudioSource::AudioSource()
    : Component("AudioSource")
    , m_audio_clip_path("")
    , m_audio_clip(nullptr)
    , m_instance_id(-1)
    , m_is_playing(false)
    , m_is_paused(false)
    , m_volume(1.0f)
    , m_pitch(1.0f)
    , m_looping(false)
    , m_play_on_start(false)
    , m_is_3d(false)
    , m_max_distance(100.0f) {

    // Initialize export variables after member variables are set
    InitializeExportVariables();
}

AudioSource::~AudioSource() {
    Stop();
}

void AudioSource::OnReady() {
    UpdateFromExportVariables();
    if (m_play_on_start && !m_audio_clip_path.empty()) {
        Play();
    }
}

void AudioSource::OnUpdate(float delta_time) {
    (void)delta_time; // Suppress unused parameter warning

    // Check if export variables have changed
    UpdateFromExportVariables();

    if (m_is_3d && m_is_playing) {
        Update3DAudio();
    }
}

void AudioSource::OnDestroy() {
    Stop();
}

void AudioSource::SetAudioClip(const std::string& path) {
    if (m_audio_clip_path == path) {
        return;
    }
    
    // Stop current playback
    Stop();
    
    m_audio_clip_path = path;
    m_audio_clip = nullptr;
    
    if (!path.empty()) {
        m_audio_clip = AudioManager::LoadAudioClip(path);
        if (!m_audio_clip) {
            std::cerr << "Failed to load audio clip: " << path << std::endl;
        }
    }
}

void AudioSource::Play() {
    if (!m_audio_clip) {
        if (!m_audio_clip_path.empty()) {
            SetAudioClip(m_audio_clip_path); // Try to load again
        }
        if (!m_audio_clip) {
            std::cerr << "No audio clip loaded for AudioSource" << std::endl;
            return;
        }
    }
    
    // Stop current playback if any
    Stop();
    
    // Start new playback
    m_instance_id = AudioManager::PlayAudio(m_audio_clip, m_volume, m_pitch, m_looping, this);
    m_is_playing = (m_instance_id != -1);
    m_is_paused = false;
    
    if (m_is_playing) {
        std::cout << "Playing audio: " << m_audio_clip_path << std::endl;
    }
}

void AudioSource::Stop() {
    if (m_is_playing && m_instance_id != -1) {
        AudioManager::StopAudio(m_instance_id);
        m_instance_id = -1;
        m_is_playing = false;
        m_is_paused = false;
    }
}

void AudioSource::Pause() {
    if (m_is_playing && !m_is_paused && m_instance_id != -1) {
        AudioManager::PauseAudio(m_instance_id);
        m_is_paused = true;
    }
}

void AudioSource::Resume() {
    if (m_is_playing && m_is_paused && m_instance_id != -1) {
        AudioManager::ResumeAudio(m_instance_id);
        m_is_paused = false;
    }
}

void AudioSource::SetVolume(float volume) {
    m_volume = std::max(0.0f, std::min(volume, 1.0f));

    if (m_is_playing && m_instance_id != -1) {
        AudioManager::SetVolume(m_instance_id, m_volume);
    }
}

void AudioSource::SetPitch(float pitch) {
    m_pitch = std::max(pitch, 0.1f);
    
    if (m_is_playing && m_instance_id != -1) {
        AudioManager::SetPitch(m_instance_id, m_pitch);
    }
}

void AudioSource::SetLooping(bool looping) {
    m_looping = looping;
    
    // Note: For simplicity, looping is set when starting playback
    // In a more advanced system, you could change it during playback
}

void AudioSource::SetPlayOnStart(bool play_on_start) {
    m_play_on_start = play_on_start;
}

void AudioSource::SetIs3D(bool is_3d) {
    m_is_3d = is_3d;
}

void AudioSource::SetMaxDistance(float max_distance) {
    m_max_distance = std::max(max_distance, 1.0f);
}

void AudioSource::Update3DAudio() {
    if (!m_is_3d || !m_owner) {
        return;
    }
    
    // Get node position for 3D audio calculation
    glm::vec3 position(0.0f);
    
    // Try to get position from Node3D first
    if (auto node3d = dynamic_cast<Node3D*>(m_owner)) {
        position = node3d->GetGlobalPosition();
    }
    // Fallback to Node2D position (z = 0)
    else if (auto node2d = dynamic_cast<Node2D*>(m_owner)) {
        glm::vec2 pos2d = node2d->GetGlobalPosition();
        position = glm::vec3(pos2d.x, pos2d.y, 0.0f);
    }
    
    // Calculate 3D volume based on distance
    float distance_volume = AudioManager::Calculate3DVolume(position, m_max_distance);
    float final_volume = m_volume * distance_volume;
    
    if (m_is_playing && m_instance_id != -1) {
        AudioManager::SetVolume(m_instance_id, final_volume);
    }
}

void AudioSource::UpdateFromExportVariables() {
    // Update audio clip path
    std::string new_audio_clip = GetExportVariableValue<std::string>("audio_clip", "");
    if (new_audio_clip != m_audio_clip_path) {
        SetAudioClip(new_audio_clip);
    }

    // Update volume
    float new_volume = GetExportVariableValue<float>("volume", 1.0f);
    if (new_volume != m_volume) {
        SetVolume(new_volume);
    }

    // Update pitch
    float new_pitch = GetExportVariableValue<float>("pitch", 1.0f);
    if (new_pitch != m_pitch) {
        SetPitch(new_pitch);
    }

    // Update looping
    bool new_looping = GetExportVariableValue<bool>("looping", false);
    if (new_looping != m_looping) {
        m_looping = new_looping;
    }

    // Update play on start
    bool new_play_on_start = GetExportVariableValue<bool>("play_on_start", false);
    if (new_play_on_start != m_play_on_start) {
        m_play_on_start = new_play_on_start;
    }

    // Update 3D audio settings
    bool new_is_3d = GetExportVariableValue<bool>("is_3d", false);
    if (new_is_3d != m_is_3d) {
        m_is_3d = new_is_3d;
    }

    // Update max distance
    float new_max_distance = GetExportVariableValue<float>("max_distance", 100.0f);
    if (new_max_distance != m_max_distance) {
        m_max_distance = new_max_distance;
    }
}

void AudioSource::InitializeExportVariables() {
    // Audio clip
    AddExportVariable("audio_clip", m_audio_clip_path, "Path to audio file", ExportVariableType::FilePath);
    
    // Playback properties
    AddExportVariable("volume", m_volume, "Audio volume (0.0 to 1.0)", ExportVariableType::Float);
    AddExportVariable("pitch", m_pitch, "Audio pitch multiplier (1.0 = normal)", ExportVariableType::Float);
    AddExportVariable("looping", m_looping, "Whether audio should loop", ExportVariableType::Bool);
    AddExportVariable("play_on_start", m_play_on_start, "Play audio automatically when component starts", ExportVariableType::Bool);
    
    // 3D Audio properties
    AddExportVariable("is_3d", m_is_3d, "Enable 3D positional audio", ExportVariableType::Bool);
    AddExportVariable("max_distance", m_max_distance, "Maximum distance for 3D audio falloff", ExportVariableType::Float);
}

} // namespace Lupine
