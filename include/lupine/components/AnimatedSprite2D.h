#pragma once

#include "lupine/core/Component.h"
#include "lupine/resources/AnimationResource.h"
#include "lupine/rendering/GraphicsTexture.h"
#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace Lupine {

/**
 * @brief Animated 2D sprite component
 * 
 * AnimatedSprite2D component displays animated sprites using either:
 * - Direct sprite sheet with frame configuration
 * - .spriteanim resource files
 * 
 * Provides Godot-like animated sprite functionality with frame-based animation,
 * multiple named animations, and playback control.
 */
class AnimatedSprite2D : public Component {
public:
    /**
     * @brief Animation playback state
     */
    enum class PlaybackState {
        Stopped,
        Playing,
        Paused
    };
    
    /**
     * @brief Constructor
     */
    AnimatedSprite2D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~AnimatedSprite2D() = default;
    
    // Resource management
    void SetSpriteAnimationResource(const std::string& filepath);
    const std::string& GetSpriteAnimationResourcePath() const { return m_sprite_animation_path; }
    
    // Direct sprite sheet mode
    void SetTexturePath(const std::string& path);
    const std::string& GetTexturePath() const { return m_texture_path; }
    
    void SetSpriteSize(const glm::ivec2& size) { m_sprite_size = size; }
    const glm::ivec2& GetSpriteSize() const { return m_sprite_size; }
    
    void SetFrameCount(int count) { m_frame_count = count; }
    int GetFrameCount() const { return m_frame_count; }
    
    void SetFramesPerRow(int frames) { m_frames_per_row = frames; }
    int GetFramesPerRow() const { return m_frames_per_row; }
    
    // Animation control
    void Play(const std::string& animation_name = "");
    void Stop();
    void Pause();
    void Resume();
    
    // Animation state
    bool IsPlaying() const { return m_playback_state == PlaybackState::Playing; }
    bool IsPaused() const { return m_playback_state == PlaybackState::Paused; }
    bool IsStopped() const { return m_playback_state == PlaybackState::Stopped; }
    
    // Current animation info
    const std::string& GetCurrentAnimation() const { return m_current_animation; }
    int GetCurrentFrame() const { return m_current_frame; }
    float GetFrameTime() const { return m_frame_time; }
    
    // Frame control
    void SetFrame(int frame);
    void SetFrameTime(float time) { m_frame_time = time; }
    
    // Playback settings
    void SetSpeedScale(float speed) { m_speed_scale = speed; }
    float GetSpeedScale() const { return m_speed_scale; }
    
    void SetAutoPlay(bool auto_play) { m_auto_play = auto_play; }
    bool GetAutoPlay() const { return m_auto_play; }
    
    void SetDefaultAnimation(const std::string& animation) { m_default_animation = animation; }
    const std::string& GetDefaultAnimation() const { return m_default_animation; }
    
    // Visual properties
    void SetColor(const glm::vec4& color) { m_color = color; }
    const glm::vec4& GetColor() const { return m_color; }

    void SetModulate(const glm::vec4& modulate) { m_modulate = modulate; }
    const glm::vec4& GetModulate() const { return m_modulate; }

    void SetFlipH(bool flip) { m_flip_h = flip; }
    bool GetFlipH() const { return m_flip_h; }
    
    void SetFlipV(bool flip) { m_flip_v = flip; }
    bool GetFlipV() const { return m_flip_v; }
    
    void SetCentered(bool centered) { m_centered = centered; }
    bool GetCentered() const { return m_centered; }
    
    void SetOffset(const glm::vec2& offset) { m_offset = offset; }
    const glm::vec2& GetOffset() const { return m_offset; }

    void SetSize(const glm::vec2& size) { m_size = size; }
    const glm::vec2& GetSize() const { return m_size; }
    
    // Available animations
    std::vector<std::string> GetAvailableAnimations() const;
    
    // Rendering
    uint32_t GetTextureHandle() const;
    glm::vec4 GetCurrentTextureRegion() const;
    glm::mat4 GetTransformMatrix() const;
    
    // Component interface
    std::string GetTypeName() const override { return "AnimatedSprite2D"; }
    std::string GetCategory() const override { return "2D"; }
    
    // Lifecycle methods
    void OnReady() override;
    void OnUpdate(float delta_time) override;

protected:
    void InitializeExportVariables() override;

private:
    // Resource paths
    std::string m_sprite_animation_path;
    std::string m_texture_path;
    
    // Sprite animation resource
    std::unique_ptr<SpriteAnimationResource> m_sprite_animation_resource;
    
    // Direct sprite sheet properties
    glm::ivec2 m_sprite_size;
    int m_frame_count;
    int m_frames_per_row;
    
    // Animation state
    PlaybackState m_playback_state;
    std::string m_current_animation;
    int m_current_frame;
    float m_frame_time;
    float m_speed_scale;
    
    // Auto-play settings
    bool m_auto_play;
    std::string m_default_animation;
    
    // Visual properties
    glm::vec4 m_color;
    glm::vec4 m_modulate;
    glm::vec2 m_size;  // Sprite size for rendering (like Sprite2D)
    bool m_flip_h;
    bool m_flip_v;
    bool m_centered;
    glm::vec2 m_offset;
    
    // Rendering data
    std::shared_ptr<GraphicsTexture> m_texture;
    bool m_texture_loaded;
    
    // Internal methods
    void LoadSpriteAnimationResource();
    void LoadTexture();
    void LoadTextureFromPath(const std::string& path);
    void UpdateFromExportVariables();
    void UpdateAnimation(float delta_time);
    void AdvanceFrame();
    glm::vec4 CalculateTextureRegion() const;
    glm::vec4 GetFrameRegionFromSheet(int frame_index) const;
    glm::vec4 GetFrameRegionFromResource(int frame_index) const;
    
    // Animation mode detection
    bool IsUsingResource() const { return !m_sprite_animation_path.empty(); }
    bool IsUsingDirectSheet() const { return !m_texture_path.empty() && m_sprite_animation_path.empty(); }
};

} // namespace Lupine
