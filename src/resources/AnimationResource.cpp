#include "lupine/resources/AnimationResource.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Lupine {

// TweenAnimationResource implementation

void TweenAnimationResource::AddClip(const AnimationClip& clip) {
    m_clips[clip.name] = clip;
}

void TweenAnimationResource::RemoveClip(const std::string& name) {
    m_clips.erase(name);
}

AnimationClip* TweenAnimationResource::GetClip(const std::string& name) {
    auto it = m_clips.find(name);
    return (it != m_clips.end()) ? &it->second : nullptr;
}

const AnimationClip* TweenAnimationResource::GetClip(const std::string& name) const {
    auto it = m_clips.find(name);
    return (it != m_clips.end()) ? &it->second : nullptr;
}

std::vector<std::string> TweenAnimationResource::GetClipNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_clips) {
        names.push_back(pair.first);
    }
    return names;
}

bool TweenAnimationResource::SaveToFile(const std::string& filepath) const {
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
        std::cerr << "Error saving tween animation resource: " << e.what() << std::endl;
        return false;
    }
}

bool TweenAnimationResource::LoadFromFile(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            std::cerr << "Animation file not found: " << filepath << std::endl;
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
        std::cerr << "Error loading tween animation resource: " << e.what() << std::endl;
        return false;
    }
}

std::string TweenAnimationResource::ToJSON() const {
    json j;
    j["type"] = "TweenAnimation";
    j["version"] = "1.0";
    
    json clips_json = json::array();
    for (const auto& pair : m_clips) {
        const AnimationClip& clip = pair.second;
        json clip_json;
        clip_json["name"] = clip.name;
        clip_json["duration"] = clip.duration;
        clip_json["looping"] = clip.looping;
        
        json tracks_json = json::array();
        for (const auto& track : clip.tracks) {
            json track_json;
            track_json["node_path"] = track.node_path;
            track_json["property_name"] = track.property_name;
            track_json["property_type"] = static_cast<int>(track.property_type);
            
            json keyframes_json = json::array();
            for (const auto& keyframe : track.keyframes) {
                json kf_json;
                kf_json["time"] = keyframe.time;
                kf_json["interpolation"] = static_cast<int>(keyframe.interpolation);
                
                // Serialize value based on type
                switch (keyframe.value.type) {
                    case AnimationPropertyType::Float:
                        kf_json["value"] = keyframe.value.float_value;
                        break;
                    case AnimationPropertyType::Vec2:
                        kf_json["value"] = {keyframe.value.vec2_value.x, keyframe.value.vec2_value.y};
                        break;
                    case AnimationPropertyType::Vec3:
                        kf_json["value"] = {keyframe.value.vec3_value.x, keyframe.value.vec3_value.y, keyframe.value.vec3_value.z};
                        break;
                    case AnimationPropertyType::Vec4:
                        kf_json["value"] = {keyframe.value.vec4_value.x, keyframe.value.vec4_value.y, keyframe.value.vec4_value.z, keyframe.value.vec4_value.w};
                        break;
                    case AnimationPropertyType::Quaternion:
                        kf_json["value"] = {keyframe.value.quat_value.x, keyframe.value.quat_value.y, keyframe.value.quat_value.z, keyframe.value.quat_value.w};
                        break;
                    case AnimationPropertyType::Bool:
                        kf_json["value"] = keyframe.value.bool_value;
                        break;
                    case AnimationPropertyType::Int:
                        kf_json["value"] = keyframe.value.int_value;
                        break;
                    case AnimationPropertyType::Color:
                        kf_json["value"] = {keyframe.value.vec4_value.x, keyframe.value.vec4_value.y, keyframe.value.vec4_value.z, keyframe.value.vec4_value.w};
                        break;
                }
                keyframes_json.push_back(kf_json);
            }
            track_json["keyframes"] = keyframes_json;
            tracks_json.push_back(track_json);
        }
        clip_json["tracks"] = tracks_json;
        clips_json.push_back(clip_json);
    }
    j["clips"] = clips_json;
    
    return j.dump(4);
}

bool TweenAnimationResource::FromJSON(const std::string& jsonData) {
    try {
        json j = json::parse(jsonData);
        
        if (j["type"] != "TweenAnimation") {
            std::cerr << "Invalid animation resource type" << std::endl;
            return false;
        }
        
        m_clips.clear();
        
        for (const auto& clip_json : j["clips"]) {
            AnimationClip clip;
            clip.name = clip_json["name"];
            clip.duration = clip_json["duration"];
            clip.looping = clip_json["looping"];
            
            for (const auto& track_json : clip_json["tracks"]) {
                AnimationTrack track;
                track.node_path = track_json["node_path"];
                track.property_name = track_json["property_name"];
                track.property_type = static_cast<AnimationPropertyType>(track_json["property_type"]);
                
                for (const auto& kf_json : track_json["keyframes"]) {
                    AnimationKeyframe keyframe;
                    keyframe.time = kf_json["time"];
                    keyframe.interpolation = static_cast<InterpolationType>(kf_json["interpolation"]);
                    
                    // Deserialize value based on type
                    keyframe.value.type = track.property_type;
                    switch (track.property_type) {
                        case AnimationPropertyType::Float:
                            keyframe.value.float_value = kf_json["value"];
                            break;
                        case AnimationPropertyType::Vec2:
                            keyframe.value.vec2_value = glm::vec2(kf_json["value"][0], kf_json["value"][1]);
                            break;
                        case AnimationPropertyType::Vec3:
                            keyframe.value.vec3_value = glm::vec3(kf_json["value"][0], kf_json["value"][1], kf_json["value"][2]);
                            break;
                        case AnimationPropertyType::Vec4:
                        case AnimationPropertyType::Color:
                            keyframe.value.vec4_value = glm::vec4(kf_json["value"][0], kf_json["value"][1], kf_json["value"][2], kf_json["value"][3]);
                            break;
                        case AnimationPropertyType::Quaternion:
                            keyframe.value.quat_value = glm::quat(kf_json["value"][3], kf_json["value"][0], kf_json["value"][1], kf_json["value"][2]);
                            break;
                        case AnimationPropertyType::Bool:
                            keyframe.value.bool_value = kf_json["value"];
                            break;
                        case AnimationPropertyType::Int:
                            keyframe.value.int_value = kf_json["value"];
                            break;
                    }
                    track.keyframes.push_back(keyframe);
                }
                clip.tracks.push_back(track);
            }
            m_clips[clip.name] = clip;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing tween animation JSON: " << e.what() << std::endl;
        return false;
    }
}

// SpriteAnimationResource implementation

void SpriteAnimationResource::AddAnimation(const SpriteAnimation& animation) {
    m_animations[animation.name] = animation;
}

void SpriteAnimationResource::RemoveAnimation(const std::string& name) {
    m_animations.erase(name);
}

SpriteAnimation* SpriteAnimationResource::GetAnimation(const std::string& name) {
    auto it = m_animations.find(name);
    return (it != m_animations.end()) ? &it->second : nullptr;
}

const SpriteAnimation* SpriteAnimationResource::GetAnimation(const std::string& name) const {
    auto it = m_animations.find(name);
    return (it != m_animations.end()) ? &it->second : nullptr;
}

std::vector<std::string> SpriteAnimationResource::GetAnimationNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_animations) {
        names.push_back(pair.first);
    }
    return names;
}

bool SpriteAnimationResource::SaveToFile(const std::string& filepath) const {
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
        std::cerr << "Error saving sprite animation resource: " << e.what() << std::endl;
        return false;
    }
}

bool SpriteAnimationResource::LoadFromFile(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            std::cerr << "Sprite animation file not found: " << filepath << std::endl;
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
        std::cerr << "Error loading sprite animation resource: " << e.what() << std::endl;
        return false;
    }
}

std::string SpriteAnimationResource::ToJSON() const {
    json j;
    j["type"] = "SpriteAnimation";
    j["version"] = "1.0";
    j["texture_path"] = m_texture_path;
    j["sprite_size"] = {m_sprite_size.x, m_sprite_size.y};
    j["sheet_size"] = {m_sheet_size.x, m_sheet_size.y};
    j["default_animation"] = m_default_animation;

    json animations_json = json::array();
    for (const auto& pair : m_animations) {
        const SpriteAnimation& anim = pair.second;
        json anim_json;
        anim_json["name"] = anim.name;
        anim_json["looping"] = anim.looping;
        anim_json["speed_scale"] = anim.speed_scale;

        json frames_json = json::array();
        for (const auto& frame : anim.frames) {
            json frame_json;
            frame_json["texture_region"] = {frame.texture_region.x, frame.texture_region.y, frame.texture_region.z, frame.texture_region.w};
            frame_json["duration"] = frame.duration;
            frames_json.push_back(frame_json);
        }
        anim_json["frames"] = frames_json;
        animations_json.push_back(anim_json);
    }
    j["animations"] = animations_json;

    return j.dump(4);
}

bool SpriteAnimationResource::FromJSON(const std::string& jsonData) {
    try {
        json j = json::parse(jsonData);

        if (j["type"] != "SpriteAnimation") {
            std::cerr << "Invalid sprite animation resource type" << std::endl;
            return false;
        }

        m_texture_path = j["texture_path"];
        m_sprite_size = glm::ivec2(j["sprite_size"][0], j["sprite_size"][1]);
        m_sheet_size = glm::ivec2(j["sheet_size"][0], j["sheet_size"][1]);
        m_default_animation = j["default_animation"];

        m_animations.clear();

        for (const auto& anim_json : j["animations"]) {
            SpriteAnimation anim;
            anim.name = anim_json["name"];
            anim.looping = anim_json["looping"];
            anim.speed_scale = anim_json["speed_scale"];

            for (const auto& frame_json : anim_json["frames"]) {
                SpriteFrame frame;
                frame.texture_region = glm::vec4(
                    frame_json["texture_region"][0],
                    frame_json["texture_region"][1],
                    frame_json["texture_region"][2],
                    frame_json["texture_region"][3]
                );
                frame.duration = frame_json["duration"];
                anim.frames.push_back(frame);
            }
            m_animations[anim.name] = anim;
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing sprite animation JSON: " << e.what() << std::endl;
        return false;
    }
}

glm::vec4 SpriteAnimationResource::GetFrameTextureRegion(int frame_index) const {
    if (m_sprite_size.x <= 0 || m_sprite_size.y <= 0 || m_sheet_size.x <= 0 || m_sheet_size.y <= 0) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    int cols = m_sheet_size.x / m_sprite_size.x;
    int rows = m_sheet_size.y / m_sprite_size.y;

    if (cols <= 0 || rows <= 0) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    int total_frames = cols * rows;
    frame_index = frame_index % total_frames; // Wrap around

    int col = frame_index % cols;
    int row = frame_index / cols;

    float u = static_cast<float>(col * m_sprite_size.x) / static_cast<float>(m_sheet_size.x);
    float v = static_cast<float>(row * m_sprite_size.y) / static_cast<float>(m_sheet_size.y);
    float w = static_cast<float>(m_sprite_size.x) / static_cast<float>(m_sheet_size.x);
    float h = static_cast<float>(m_sprite_size.y) / static_cast<float>(m_sheet_size.y);

    return glm::vec4(u, v, w, h);
}

int SpriteAnimationResource::GetFrameCount() const {
    if (m_sprite_size.x <= 0 || m_sprite_size.y <= 0 || m_sheet_size.x <= 0 || m_sheet_size.y <= 0) {
        return 0;
    }

    int cols = m_sheet_size.x / m_sprite_size.x;
    int rows = m_sheet_size.y / m_sprite_size.y;

    return cols * rows;
}

} // namespace Lupine
