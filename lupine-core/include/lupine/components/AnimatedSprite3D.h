#pragma once

#include "lupine/core/Component.h"
#include "lupine/resources/AnimationResource.h"
#include <memory>
#include <string>
#include <glm/glm.hpp>

namespace Lupine {

class Node3D;

/**
 * @brief Animated 3D sprite component
 *
 * AnimatedSprite3D component displays animated sprites in 3D space with billboard behavior,
 * proper depth handling, and various alignment options. Uses the same animation system as
 * AnimatedSprite2D but renders in 3D space with proper depth testing and lighting.
 * Should be attached to Node3D nodes.
 */
class AnimatedSprite3D : public Component {
public:
    /**
     * @brief Billboard mode options (matching Sprite3D)
     */
    enum class BillboardMode {
        Disabled,       // No billboard behavior
        Enabled,        // Always face camera
        YBillboard,     // Only rotate around Y axis
        ParticlesBillboard  // Special mode for particles
    };

    /**
     * @brief Alpha cut mode options
     */
    enum class AlphaCutMode {
        Disabled,       // No alpha cutting
        Discard,        // Discard pixels below threshold
        Opaque,         // Treat pixels below threshold as opaque
        Transparent     // Treat pixels below threshold as transparent
    };

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
    AnimatedSprite3D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~AnimatedSprite3D();
    
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
    
    // Visual properties (matching Sprite3D interface)
    void SetModulate(const glm::vec4& modulate);
    const glm::vec4& GetModulate() const { return m_modulate; }

    void SetSize(const glm::vec2& size);
    const glm::vec2& GetSize() const { return m_size; }

    void SetOffset(const glm::vec2& offset);
    const glm::vec2& GetOffset() const { return m_offset; }

    void SetCentered(bool centered);
    bool GetCentered() const { return m_centered; }

    void SetFlipH(bool flip);
    bool GetFlipH() const { return m_flip_h; }

    void SetFlipV(bool flip);
    bool GetFlipV() const { return m_flip_v; }

    // 3D-specific properties (matching Sprite3D interface)
    void SetBillboardMode(BillboardMode mode);
    BillboardMode GetBillboardMode() const { return m_billboard_mode; }

    void SetAlphaCutMode(AlphaCutMode mode);
    AlphaCutMode GetAlphaCutMode() const { return m_alpha_cut_mode; }

    void SetAlphaCutThreshold(float threshold);
    float GetAlphaCutThreshold() const { return m_alpha_cut_threshold; }

    void SetTransparent(bool transparent);
    bool GetTransparent() const { return m_transparent; }

    void SetDoubleSided(bool double_sided);
    bool GetDoubleSided() const { return m_double_sided; }

    void SetReceivesLighting(bool receives_lighting);
    bool GetReceivesLighting() const { return m_receives_lighting; }
    
    // Available animations
    std::vector<std::string> GetAvailableAnimations() const;

    // Rendering (matching Sprite3D interface)
    unsigned int GetTextureID() const { return m_texture_id; }
    glm::vec4 GetCurrentTextureRegion() const;
    glm::vec4 GetLocalBounds() const;

    // Component interface
    std::string GetTypeName() const override { return "AnimatedSprite3D"; }
    std::string GetCategory() const override { return "3D"; }

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

    // Visual properties (matching Sprite3D)
    glm::vec4 m_modulate;
    glm::vec2 m_size;
    glm::vec2 m_offset;
    bool m_centered;
    bool m_flip_h;
    bool m_flip_v;

    // 3D-specific properties (matching Sprite3D)
    BillboardMode m_billboard_mode;
    AlphaCutMode m_alpha_cut_mode;
    float m_alpha_cut_threshold;
    bool m_transparent;
    bool m_double_sided;
    bool m_receives_lighting;

    // Internal rendering data (matching Sprite3D)
    unsigned int m_texture_id;
    bool m_texture_loaded;
    unsigned int m_vertex_array_object;
    unsigned int m_vertex_buffer_object;
    unsigned int m_element_buffer_object;
    bool m_mesh_initialized;
    
    // Internal methods (matching Sprite3D pattern)
    void LoadSpriteAnimationResource();
    void LoadTexture();
    void LoadTextureFromPath(const std::string& path);
    void UpdateFromExportVariables();
    void UpdateAnimation(float delta_time);
    void AdvanceFrame();
    glm::vec4 CalculateTextureRegion() const;
    glm::vec4 GetFrameRegionFromSheet(int frame_index) const;
    glm::vec4 GetFrameRegionFromResource(int frame_index) const;

    // Rendering methods (matching Sprite3D pattern)
    void InitializeMesh();
    void UpdateMeshVertices();
    void RenderSprite(Node3D* node3d);
    glm::mat4 CalculateBillboardTransform(const glm::mat4& node_transform) const;

    // Animation mode detection
    bool IsUsingResource() const { return !m_sprite_animation_path.empty(); }
    bool IsUsingDirectSheet() const { return !m_texture_path.empty() && m_sprite_animation_path.empty(); }
};

} // namespace Lupine
