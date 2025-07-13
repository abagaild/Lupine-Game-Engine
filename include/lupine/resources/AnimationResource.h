#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "lupine/core/UUID.h"

namespace Lupine {

/**
 * @brief Animation interpolation types
 */
enum class InterpolationType {
    Linear,
    Ease,
    EaseIn,
    EaseOut,
    EaseInOut,
    Bounce,
    Elastic,
    Back,
    Cubic,
    Sine,
    Expo,
    Circ,
    Quad,
    Quart,
    Quint
};

/**
 * @brief Animation property types that can be animated
 */
enum class AnimationPropertyType {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Quaternion,
    Color,
    Bool,
    Int
};

/**
 * @brief Animation property value variant
 */
struct AnimationValue {
    AnimationPropertyType type;
    union {
        float float_value;
        glm::vec2 vec2_value;
        glm::vec3 vec3_value;
        glm::vec4 vec4_value;
        glm::quat quat_value;
        bool bool_value;
        int int_value;
    };
    
    AnimationValue() : type(AnimationPropertyType::Float), float_value(0.0f) {}
    AnimationValue(float value) : type(AnimationPropertyType::Float), float_value(value) {}
    AnimationValue(const glm::vec2& value) : type(AnimationPropertyType::Vec2), vec2_value(value) {}
    AnimationValue(const glm::vec3& value) : type(AnimationPropertyType::Vec3), vec3_value(value) {}
    AnimationValue(const glm::vec4& value) : type(AnimationPropertyType::Vec4), vec4_value(value) {}
    AnimationValue(const glm::quat& value) : type(AnimationPropertyType::Quaternion), quat_value(value) {}
    AnimationValue(bool value) : type(AnimationPropertyType::Bool), bool_value(value) {}
    AnimationValue(int value) : type(AnimationPropertyType::Int), int_value(value) {}
};

/**
 * @brief Animation keyframe
 */
struct AnimationKeyframe {
    float time;                     // Time in seconds
    AnimationValue value;           // Value at this keyframe
    InterpolationType interpolation; // Interpolation to next keyframe
    
    AnimationKeyframe() : time(0.0f), interpolation(InterpolationType::Linear) {}
    AnimationKeyframe(float t, const AnimationValue& v, InterpolationType interp = InterpolationType::Linear)
        : time(t), value(v), interpolation(interp) {}
};

/**
 * @brief Animation track for a specific property
 */
struct AnimationTrack {
    std::string node_path;          // Path to the node (e.g., "Player/Sprite")
    std::string property_name;      // Property name (e.g., "position", "rotation", "scale")
    AnimationPropertyType property_type;
    std::vector<AnimationKeyframe> keyframes;
    
    AnimationTrack() : property_type(AnimationPropertyType::Float) {}
    AnimationTrack(const std::string& path, const std::string& prop, AnimationPropertyType type)
        : node_path(path), property_name(prop), property_type(type) {}
};

/**
 * @brief Animation clip containing multiple tracks
 */
struct AnimationClip {
    std::string name;
    float duration;                 // Duration in seconds
    bool looping;
    std::vector<AnimationTrack> tracks;
    
    AnimationClip() : duration(1.0f), looping(false) {}
    AnimationClip(const std::string& n, float d = 1.0f, bool loop = false)
        : name(n), duration(d), looping(loop) {}
};

/**
 * @brief Tween animation resource (.anim files)
 */
class TweenAnimationResource {
public:
    TweenAnimationResource() = default;
    ~TweenAnimationResource() = default;
    
    // Animation management
    void AddClip(const AnimationClip& clip);
    void RemoveClip(const std::string& name);
    AnimationClip* GetClip(const std::string& name);
    const AnimationClip* GetClip(const std::string& name) const;
    std::vector<std::string> GetClipNames() const;
    
    // Serialization
    bool SaveToFile(const std::string& filepath) const;
    bool LoadFromFile(const std::string& filepath);
    
    // JSON serialization
    std::string ToJSON() const;
    bool FromJSON(const std::string& json);
    
private:
    std::map<std::string, AnimationClip> m_clips;
};

/**
 * @brief Sprite frame data
 */
struct SpriteFrame {
    glm::vec4 texture_region;       // x, y, width, height in texture coordinates (0-1)
    float duration;                 // Duration to display this frame in seconds
    
    SpriteFrame() : texture_region(0.0f, 0.0f, 1.0f, 1.0f), duration(0.1f) {}
    SpriteFrame(const glm::vec4& region, float dur = 0.1f)
        : texture_region(region), duration(dur) {}
};

/**
 * @brief Sprite animation data
 */
struct SpriteAnimation {
    std::string name;
    std::vector<SpriteFrame> frames;
    bool looping;
    float speed_scale;              // Speed multiplier for the animation
    
    SpriteAnimation() : looping(true), speed_scale(1.0f) {}
    SpriteAnimation(const std::string& n, bool loop = true, float speed = 1.0f)
        : name(n), looping(loop), speed_scale(speed) {}
        
    float GetTotalDuration() const {
        float total = 0.0f;
        for (const auto& frame : frames) {
            total += frame.duration;
        }
        return total * speed_scale;
    }
};

/**
 * @brief Sprite animation resource (.spriteanim files)
 */
class SpriteAnimationResource {
public:
    SpriteAnimationResource() = default;
    ~SpriteAnimationResource() = default;
    
    // Texture management
    void SetTexturePath(const std::string& path) { m_texture_path = path; }
    const std::string& GetTexturePath() const { return m_texture_path; }
    
    // Sprite sheet properties
    void SetSpriteSize(const glm::ivec2& size) { m_sprite_size = size; }
    const glm::ivec2& GetSpriteSize() const { return m_sprite_size; }
    
    void SetSheetSize(const glm::ivec2& size) { m_sheet_size = size; }
    const glm::ivec2& GetSheetSize() const { return m_sheet_size; }
    
    // Animation management
    void AddAnimation(const SpriteAnimation& animation);
    void RemoveAnimation(const std::string& name);
    SpriteAnimation* GetAnimation(const std::string& name);
    const SpriteAnimation* GetAnimation(const std::string& name) const;
    std::vector<std::string> GetAnimationNames() const;
    
    // Default animation
    void SetDefaultAnimation(const std::string& name) { m_default_animation = name; }
    const std::string& GetDefaultAnimation() const { return m_default_animation; }
    
    // Serialization
    bool SaveToFile(const std::string& filepath) const;
    bool LoadFromFile(const std::string& filepath);
    
    // JSON serialization
    std::string ToJSON() const;
    bool FromJSON(const std::string& json);
    
    // Utility functions
    glm::vec4 GetFrameTextureRegion(int frame_index) const;
    int GetFrameCount() const;
    
private:
    std::string m_texture_path;
    glm::ivec2 m_sprite_size;       // Size of each sprite in pixels
    glm::ivec2 m_sheet_size;        // Size of the sprite sheet in pixels
    std::string m_default_animation;
    std::map<std::string, SpriteAnimation> m_animations;
};

} // namespace Lupine
