#include "lupine/components/AnimatedSprite2D.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/rendering/Renderer.h"
#include <iostream>
#include <algorithm>

namespace Lupine {

AnimatedSprite2D::AnimatedSprite2D()
    : Component("AnimatedSprite2D")
    , m_sprite_size(32, 32)
    , m_frame_count(1)
    , m_frames_per_row(1)
    , m_playback_state(PlaybackState::Stopped)
    , m_current_frame(0)
    , m_frame_time(0.0f)
    , m_speed_scale(1.0f)
    , m_auto_play(false)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_modulate(1.0f, 1.0f, 1.0f, 1.0f)
    , m_size(32.0f, 32.0f)  // Default size matching sprite_size
    , m_flip_h(false)
    , m_flip_v(false)
    , m_centered(true)
    , m_offset(0.0f, 0.0f)
    , m_texture_id(0)
    , m_texture_loaded(false)
{
    InitializeExportVariables();
}

void AnimatedSprite2D::InitializeExportVariables() {
    AddExportVariable("sprite_animation_resource", m_sprite_animation_path, "Path to .spriteanim resource file", ExportVariableType::FilePath);
    AddExportVariable("texture_path", m_texture_path, "Direct sprite sheet texture path", ExportVariableType::FilePath);
    AddExportVariable("sprite_size", m_sprite_size, "Size of each sprite frame in pixels", ExportVariableType::Vec2);
    AddExportVariable("frame_count", m_frame_count, "Total number of frames in sprite sheet", ExportVariableType::Int);
    AddExportVariable("frames_per_row", m_frames_per_row, "Number of frames per row in sprite sheet", ExportVariableType::Int);
    AddExportVariable("auto_play", m_auto_play, "Automatically play animation on ready", ExportVariableType::Bool);
    AddExportVariable("default_animation", m_default_animation, "Default animation to play", ExportVariableType::String);
    AddExportVariable("speed_scale", m_speed_scale, "Animation speed multiplier", ExportVariableType::Float);
    AddExportVariable("color", m_color, "Sprite color tint", ExportVariableType::Color);
    AddExportVariable("modulate", m_modulate, "Sprite color modulation", ExportVariableType::Color);
    AddExportVariable("size", m_size, "Sprite size for rendering", ExportVariableType::Vec2);
    AddExportVariable("flip_h", m_flip_h, "Flip sprite horizontally", ExportVariableType::Bool);
    AddExportVariable("flip_v", m_flip_v, "Flip sprite vertically", ExportVariableType::Bool);
    AddExportVariable("centered", m_centered, "Center sprite on node position", ExportVariableType::Bool);
    AddExportVariable("offset", m_offset, "Sprite offset from node position", ExportVariableType::Vec2);
}

void AnimatedSprite2D::SetSpriteAnimationResource(const std::string& filepath) {
    m_sprite_animation_path = filepath;
    LoadSpriteAnimationResource();
    
    // Update export variable
    SetExportVariable("sprite_animation_resource", m_sprite_animation_path);
}

void AnimatedSprite2D::SetTexturePath(const std::string& path) {
    m_texture_path = path;
    LoadTexture();
    
    // Update export variable
    SetExportVariable("texture_path", m_texture_path);
}

void AnimatedSprite2D::Play(const std::string& animation_name) {
    std::string target_animation = animation_name;
    
    if (IsUsingResource()) {
        if (!m_sprite_animation_resource) {
            std::cerr << "AnimatedSprite2D: No sprite animation resource loaded" << std::endl;
            return;
        }
        
        if (target_animation.empty()) {
            // Use default animation or first available
            if (!m_default_animation.empty()) {
                target_animation = m_default_animation;
            } else {
                auto animations = GetAvailableAnimations();
                if (!animations.empty()) {
                    target_animation = animations[0];
                }
            }
        }
        
        if (target_animation.empty()) {
            std::cerr << "AnimatedSprite2D: No animation specified or available" << std::endl;
            return;
        }
        
        const SpriteAnimation* animation = m_sprite_animation_resource->GetAnimation(target_animation);
        if (!animation) {
            std::cerr << "AnimatedSprite2D: Animation '" << target_animation << "' not found" << std::endl;
            return;
        }
        
        m_current_animation = target_animation;
    } else if (IsUsingDirectSheet()) {
        // For direct sprite sheet, use a default animation name
        m_current_animation = "default";
    } else {
        std::cerr << "AnimatedSprite2D: No sprite animation resource or texture configured" << std::endl;
        return;
    }
    
    m_playback_state = PlaybackState::Playing;
    m_current_frame = 0;
    m_frame_time = 0.0f;
    
    std::cout << "AnimatedSprite2D: Playing animation '" << m_current_animation << "'" << std::endl;
}

void AnimatedSprite2D::Stop() {
    m_playback_state = PlaybackState::Stopped;
    m_current_frame = 0;
    m_frame_time = 0.0f;
}

void AnimatedSprite2D::Pause() {
    if (m_playback_state == PlaybackState::Playing) {
        m_playback_state = PlaybackState::Paused;
    }
}

void AnimatedSprite2D::Resume() {
    if (m_playback_state == PlaybackState::Paused) {
        m_playback_state = PlaybackState::Playing;
    }
}

void AnimatedSprite2D::SetFrame(int frame) {
    if (IsUsingResource() && m_sprite_animation_resource && !m_current_animation.empty()) {
        const SpriteAnimation* animation = m_sprite_animation_resource->GetAnimation(m_current_animation);
        if (animation) {
            m_current_frame = std::clamp(frame, 0, static_cast<int>(animation->frames.size()) - 1);
        }
    } else if (IsUsingDirectSheet()) {
        m_current_frame = std::clamp(frame, 0, m_frame_count - 1);
    }
    
    m_frame_time = 0.0f;
}

std::vector<std::string> AnimatedSprite2D::GetAvailableAnimations() const {
    if (IsUsingResource() && m_sprite_animation_resource) {
        return m_sprite_animation_resource->GetAnimationNames();
    }
    
    return {"default"}; // For direct sprite sheet mode
}

glm::vec4 AnimatedSprite2D::GetCurrentTextureRegion() const {
    return CalculateTextureRegion();
}

glm::mat4 AnimatedSprite2D::GetTransformMatrix() const {
    glm::mat4 transform = glm::mat4(1.0f);

    if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
        // Use Node2D transform properties (matching Sprite2D approach)
        glm::vec2 pos = node2d->GetGlobalPosition();

        // Apply offset
        pos += m_offset;

        transform = glm::translate(transform, glm::vec3(pos.x, pos.y, 0.0f));

        float rotation = node2d->GetGlobalRotation();
        transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));

        glm::vec2 scale = node2d->GetGlobalScale();
        // Use actual sprite size (like Sprite2D) - don't apply flipping here since it's handled by renderer
        transform = glm::scale(transform, glm::vec3(scale.x * m_size.x, scale.y * m_size.y, 1.0f));

        // Apply centering offset - when centered, no additional offset needed
        // When not centered, sprite origin is at top-left, so offset by half size
        if (!m_centered) {
            transform = glm::translate(transform, glm::vec3(0.5f, 0.5f, 0.0f));
        }
    } else if (GetOwner()) {
        // Fallback for regular Node - just use sprite size at origin
        glm::vec3 pos = glm::vec3(m_offset.x, m_offset.y, 0.0f);
        transform = glm::translate(transform, pos);
        transform = glm::scale(transform, glm::vec3(m_size.x, m_size.y, 1.0f));

        // Apply centering offset if not centered
        if (!m_centered) {
            transform = glm::translate(transform, glm::vec3(0.5f, 0.5f, 0.0f));
        }
    }

    return transform;
}

void AnimatedSprite2D::OnReady() {
    UpdateFromExportVariables();
    
    if (IsUsingResource()) {
        LoadSpriteAnimationResource();
    } else if (IsUsingDirectSheet()) {
        LoadTexture();
    }
    
    // Auto-play if enabled
    if (m_auto_play) {
        Play(m_default_animation);
    } else if (IsUsingDirectSheet() && m_playback_state == PlaybackState::Stopped) {
        // For direct sprite sheet mode, ensure we show the first frame even when not playing
        m_current_frame = 0;
    }
}

void AnimatedSprite2D::OnUpdate(float delta_time) {
    UpdateFromExportVariables();

    // Load texture if needed
    if (IsUsingResource() && !m_sprite_animation_path.empty() && !m_sprite_animation_resource) {
        LoadSpriteAnimationResource();
    } else if (IsUsingDirectSheet() && !m_texture_path.empty() && !m_texture_loaded) {
        LoadTexture();
    }

    if (m_playback_state == PlaybackState::Playing) {
        UpdateAnimation(delta_time);
    }

    // Check rendering context - AnimatedSprite2D should only render in 2D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor3D) {
        return; // Don't render 2D sprites in 3D editor view
    }

    // Use white texture if no texture is loaded
    std::shared_ptr<GraphicsTexture> texture_to_use = (m_texture_loaded && m_texture_id != 0) ? Renderer::GetWhiteTexture() : Renderer::GetWhiteTexture();

    // Calculate final texture region based on current frame
    glm::vec4 final_texture_region = CalculateTextureRegion();

    // Build transform matrix
    glm::mat4 transform = GetTransformMatrix();

    // Calculate final color (color * modulate)
    glm::vec4 final_color = m_color * m_modulate;

    // Skip rendering if size is zero or color is fully transparent
    if (m_size.x <= 0.0f || m_size.y <= 0.0f || final_color.a <= 0.0f) {
        return;
    }

    // Render sprite with texture
    Renderer::RenderQuad(transform, final_color, texture_to_use, final_texture_region, m_flip_h, m_flip_v);
}

void AnimatedSprite2D::LoadSpriteAnimationResource() {
    m_sprite_animation_resource.reset();
    
    if (m_sprite_animation_path.empty()) {
        return;
    }
    
    m_sprite_animation_resource = std::make_unique<SpriteAnimationResource>();
    if (!m_sprite_animation_resource->LoadFromFile(m_sprite_animation_path)) {
        std::cerr << "AnimatedSprite2D: Failed to load sprite animation resource: " << m_sprite_animation_path << std::endl;
        m_sprite_animation_resource.reset();
    } else {
        std::cout << "AnimatedSprite2D: Loaded sprite animation resource: " << m_sprite_animation_path << std::endl;
        
        // Load the texture from the resource
        std::string texturePath = m_sprite_animation_resource->GetTexturePath();
        if (!texturePath.empty()) {
            LoadTextureFromPath(texturePath);
        }
    }
}

void AnimatedSprite2D::LoadTexture() {
    if (m_texture_path.empty()) {
        m_texture_loaded = false;
        m_texture_id = 0;
        return;
    }
    
    LoadTextureFromPath(m_texture_path);
}

void AnimatedSprite2D::LoadTextureFromPath(const std::string& path) {
    // Initialize ResourceManager if not already done
    if (!ResourceManager::IsInitialized()) {
        ResourceManager::Initialize();
    }

    // Load texture using ResourceManager
    Texture texture = ResourceManager::LoadTexture(path);
    if (texture.IsValid()) {
        m_texture_id = texture.id;
        m_texture_loaded = true;
    } else {
        m_texture_id = 0;
        m_texture_loaded = false;
    }
}

void AnimatedSprite2D::UpdateFromExportVariables() {
    // Update internal state from export variables
    std::string new_resource_path = GetExportVariableValue<std::string>("sprite_animation_resource", m_sprite_animation_path);
    if (new_resource_path != m_sprite_animation_path) {
        SetSpriteAnimationResource(new_resource_path);
    }

    std::string new_texture_path = GetExportVariableValue<std::string>("texture_path", m_texture_path);
    if (new_texture_path != m_texture_path) {
        SetTexturePath(new_texture_path);
    }

    // Update other properties from export variables
    glm::vec2 sprite_size_vec2 = GetExportVariableValue<glm::vec2>("sprite_size", glm::vec2(m_sprite_size.x, m_sprite_size.y));
    m_sprite_size = glm::ivec2(sprite_size_vec2.x, sprite_size_vec2.y);

    m_frame_count = GetExportVariableValue<int>("frame_count", m_frame_count);
    m_frames_per_row = GetExportVariableValue<int>("frames_per_row", m_frames_per_row);
    m_auto_play = GetExportVariableValue<bool>("auto_play", m_auto_play);
    m_default_animation = GetExportVariableValue<std::string>("default_animation", m_default_animation);
    m_speed_scale = GetExportVariableValue<float>("speed_scale", m_speed_scale);
    m_color = GetExportVariableValue<glm::vec4>("color", m_color);
    m_modulate = GetExportVariableValue<glm::vec4>("modulate", m_modulate);
    m_size = GetExportVariableValue<glm::vec2>("size", m_size);
    m_flip_h = GetExportVariableValue<bool>("flip_h", m_flip_h);
    m_flip_v = GetExportVariableValue<bool>("flip_v", m_flip_v);
    m_centered = GetExportVariableValue<bool>("centered", m_centered);
    m_offset = GetExportVariableValue<glm::vec2>("offset", m_offset);
}

void AnimatedSprite2D::UpdateAnimation(float delta_time) {
    if (IsUsingResource() && m_sprite_animation_resource && !m_current_animation.empty()) {
        const SpriteAnimation* animation = m_sprite_animation_resource->GetAnimation(m_current_animation);
        if (!animation || animation->frames.empty()) return;

        // Ensure current frame is within bounds
        if (m_current_frame < 0 || m_current_frame >= static_cast<int>(animation->frames.size())) {
            m_current_frame = 0;
        }

        // Update frame time
        float frame_duration = animation->frames[m_current_frame].duration;
        m_frame_time += delta_time * m_speed_scale * animation->speed_scale;

        // Check if we need to advance to next frame
        if (m_frame_time >= frame_duration) {
            m_frame_time = 0.0f;
            AdvanceFrame();
        }
    } else if (IsUsingDirectSheet()) {
        // For direct sprite sheet, use a default frame duration
        const float DEFAULT_FRAME_DURATION = 0.1f;
        m_frame_time += delta_time * m_speed_scale;

        if (m_frame_time >= DEFAULT_FRAME_DURATION) {
            m_frame_time = 0.0f;
            AdvanceFrame();
        }
    }
}

void AnimatedSprite2D::AdvanceFrame() {
    if (IsUsingResource() && m_sprite_animation_resource && !m_current_animation.empty()) {
        const SpriteAnimation* animation = m_sprite_animation_resource->GetAnimation(m_current_animation);
        if (!animation || animation->frames.empty()) return;

        m_current_frame++;

        // Handle looping
        if (m_current_frame >= static_cast<int>(animation->frames.size())) {
            if (animation->looping) {
                m_current_frame = 0;
            } else {
                m_current_frame = static_cast<int>(animation->frames.size()) - 1;
                m_playback_state = PlaybackState::Stopped;
            }
        }
    } else if (IsUsingDirectSheet()) {
        m_current_frame++;

        // Handle looping for direct sprite sheet
        if (m_current_frame >= m_frame_count) {
            m_current_frame = 0; // Always loop for direct sprite sheet
        }
    }
}

glm::vec4 AnimatedSprite2D::CalculateTextureRegion() const {
    if (IsUsingResource()) {
        return GetFrameRegionFromResource(m_current_frame);
    } else if (IsUsingDirectSheet()) {
        return GetFrameRegionFromSheet(m_current_frame);
    }

    return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // Full texture
}

glm::vec4 AnimatedSprite2D::GetFrameRegionFromSheet(int frame_index) const {
    if (m_frames_per_row <= 0 || m_sprite_size.x <= 0 || m_sprite_size.y <= 0) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    // Calculate frame position in grid
    int col = frame_index % m_frames_per_row;
    int row = frame_index / m_frames_per_row;

    // Try to get actual texture dimensions from ResourceManager
    float texture_width = static_cast<float>(m_frames_per_row * m_sprite_size.x);
    float texture_height = static_cast<float>((m_frame_count / m_frames_per_row + 1) * m_sprite_size.y);

    if (m_texture_loaded && !m_texture_path.empty()) {
        Texture texture = ResourceManager::GetTexture(m_texture_path);
        if (texture.IsValid()) {
            texture_width = static_cast<float>(texture.width);
            texture_height = static_cast<float>(texture.height);
        }
    }

    float u = static_cast<float>(col * m_sprite_size.x) / texture_width;
    float v = static_cast<float>(row * m_sprite_size.y) / texture_height;
    float w = static_cast<float>(m_sprite_size.x) / texture_width;
    float h = static_cast<float>(m_sprite_size.y) / texture_height;

    return glm::vec4(u, v, w, h);
}

glm::vec4 AnimatedSprite2D::GetFrameRegionFromResource(int frame_index) const {
    if (!m_sprite_animation_resource || m_current_animation.empty()) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    const SpriteAnimation* animation = m_sprite_animation_resource->GetAnimation(m_current_animation);
    if (!animation || frame_index < 0 || frame_index >= static_cast<int>(animation->frames.size())) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    return animation->frames[frame_index].texture_region;
}

} // namespace Lupine
