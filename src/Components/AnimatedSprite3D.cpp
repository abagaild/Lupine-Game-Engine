#include "lupine/components/AnimatedSprite3D.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/GraphicsDeviceFactory.h"
#include "lupine/rendering/GraphicsTexture.h"
#include "lupine/rendering/GraphicsVertexArray.h"
#include "lupine/rendering/GraphicsBuffer.h"
#include "lupine/rendering/VertexLayout.h"
#include "lupine/resources/MeshLoader.h"
#include <iostream>
#include <algorithm>

namespace Lupine {

AnimatedSprite3D::AnimatedSprite3D()
    : Component("AnimatedSprite3D")
    , m_sprite_size(32, 32)
    , m_frame_count(1)
    , m_frames_per_row(1)
    , m_playback_state(PlaybackState::Stopped)
    , m_current_frame(0)
    , m_frame_time(0.0f)
    , m_speed_scale(1.0f)
    , m_auto_play(false)
    , m_modulate(1.0f, 1.0f, 1.0f, 1.0f)
    , m_size(1.0f, 1.0f)
    , m_offset(0.0f, 0.0f)
    , m_centered(true)
    , m_flip_h(false)
    , m_flip_v(false)
    , m_billboard_mode(BillboardMode::Disabled)
    , m_alpha_cut_mode(AlphaCutMode::Disabled)
    , m_alpha_cut_threshold(0.5f)
    , m_transparent(false)
    , m_double_sided(false)
    , m_receives_lighting(true)
    , m_texture(nullptr)
    , m_texture_loaded(false)
    , m_vertex_array(nullptr)
    , m_vertex_buffer(nullptr)
    , m_index_buffer(nullptr)
    , m_mesh_initialized(false)
{
    InitializeExportVariables();
}

AnimatedSprite3D::~AnimatedSprite3D() {
    // Clean up graphics resources - shared_ptr will handle cleanup automatically
    if (m_mesh_initialized) {
        m_vertex_array.reset();
        m_vertex_buffer.reset();
        m_index_buffer.reset();
    }
}

void AnimatedSprite3D::InitializeExportVariables() {
    // Animation resource properties
    AddExportVariable("sprite_animation_resource", m_sprite_animation_path, "Path to .spriteanim resource file", ExportVariableType::FilePath);
    AddExportVariable("texture_path", m_texture_path, "Direct sprite sheet texture path", ExportVariableType::FilePath);
    AddExportVariable("sprite_size", m_sprite_size, "Size of each sprite frame in pixels", ExportVariableType::Vec2);
    AddExportVariable("frame_count", m_frame_count, "Total number of frames in sprite sheet", ExportVariableType::Int);
    AddExportVariable("frames_per_row", m_frames_per_row, "Number of frames per row in sprite sheet", ExportVariableType::Int);
    
    // Animation control properties
    AddExportVariable("auto_play", m_auto_play, "Automatically play animation on ready", ExportVariableType::Bool);
    AddExportVariable("default_animation", m_default_animation, "Default animation to play", ExportVariableType::String);
    AddExportVariable("speed_scale", m_speed_scale, "Animation speed multiplier", ExportVariableType::Float);
    
    // Visual properties (matching Sprite3D)
    AddExportVariable("modulate", m_modulate, "Color modulation", ExportVariableType::Color);
    AddExportVariable("size", m_size, "Sprite size in world units", ExportVariableType::Vec2);
    AddExportVariable("offset", m_offset, "Sprite offset", ExportVariableType::Vec2);
    AddExportVariable("centered", m_centered, "Center sprite on node position", ExportVariableType::Bool);
    AddExportVariable("flip_h", m_flip_h, "Flip sprite horizontally", ExportVariableType::Bool);
    AddExportVariable("flip_v", m_flip_v, "Flip sprite vertically", ExportVariableType::Bool);
    
    // 3D-specific properties (matching Sprite3D)
    std::vector<std::string> billboardModeOptions = {
        "Disabled", "Enabled", "Y-Billboard", "Particles"
    };
    AddEnumExportVariable("billboard_mode", static_cast<int>(m_billboard_mode), "Billboard mode", billboardModeOptions);

    std::vector<std::string> alphaCutModeOptions = {
        "Disabled", "Discard", "OpaquePrepass"
    };
    AddEnumExportVariable("alpha_cut_mode", static_cast<int>(m_alpha_cut_mode), "Alpha cut mode", alphaCutModeOptions);
    AddExportVariable("alpha_cut_threshold", m_alpha_cut_threshold, "Alpha cut threshold", ExportVariableType::Float);
    AddExportVariable("transparent", m_transparent, "Enable transparency", ExportVariableType::Bool);
    AddExportVariable("double_sided", m_double_sided, "Render both sides", ExportVariableType::Bool);
    AddExportVariable("receives_lighting", m_receives_lighting, "Receive lighting", ExportVariableType::Bool);
}

void AnimatedSprite3D::SetSpriteAnimationResource(const std::string& filepath) {
    m_sprite_animation_path = filepath;
    LoadSpriteAnimationResource();
    SetExportVariable("sprite_animation_resource", m_sprite_animation_path);
}

void AnimatedSprite3D::SetTexturePath(const std::string& path) {
    m_texture_path = path;
    LoadTexture();
    SetExportVariable("texture_path", m_texture_path);
}

void AnimatedSprite3D::SetModulate(const glm::vec4& modulate) {
    m_modulate = modulate;
    SetExportVariable("modulate", modulate);
}

void AnimatedSprite3D::SetSize(const glm::vec2& size) {
    m_size = glm::max(size, glm::vec2(0.001f)); // Prevent zero/negative size
    SetExportVariable("size", m_size);
    UpdateMeshVertices();
}

void AnimatedSprite3D::SetOffset(const glm::vec2& offset) {
    m_offset = offset;
    SetExportVariable("offset", offset);
    UpdateMeshVertices();
}

void AnimatedSprite3D::SetCentered(bool centered) {
    m_centered = centered;
    SetExportVariable("centered", centered);
    UpdateMeshVertices();
}

void AnimatedSprite3D::SetFlipH(bool flip) {
    m_flip_h = flip;
    SetExportVariable("flip_h", flip);
    UpdateMeshVertices();
}

void AnimatedSprite3D::SetFlipV(bool flip) {
    m_flip_v = flip;
    SetExportVariable("flip_v", flip);
    UpdateMeshVertices();
}

void AnimatedSprite3D::SetBillboardMode(BillboardMode mode) {
    m_billboard_mode = mode;
    SetExportVariable("billboard_mode", static_cast<int>(mode));
}

void AnimatedSprite3D::SetAlphaCutMode(AlphaCutMode mode) {
    m_alpha_cut_mode = mode;
    SetExportVariable("alpha_cut_mode", static_cast<int>(mode));
}

void AnimatedSprite3D::SetAlphaCutThreshold(float threshold) {
    m_alpha_cut_threshold = threshold;
    SetExportVariable("alpha_cut_threshold", threshold);
}

void AnimatedSprite3D::SetTransparent(bool transparent) {
    m_transparent = transparent;
    SetExportVariable("transparent", transparent);
}

void AnimatedSprite3D::SetDoubleSided(bool double_sided) {
    m_double_sided = double_sided;
    SetExportVariable("double_sided", double_sided);
}

void AnimatedSprite3D::SetReceivesLighting(bool receives_lighting) {
    m_receives_lighting = receives_lighting;
    SetExportVariable("receives_lighting", receives_lighting);
}

void AnimatedSprite3D::Play(const std::string& animation_name) {
    std::string target_animation = animation_name;
    
    if (IsUsingResource()) {
        if (!m_sprite_animation_resource) {
            std::cerr << "AnimatedSprite3D: No sprite animation resource loaded" << std::endl;
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
            std::cerr << "AnimatedSprite3D: No animation specified or available" << std::endl;
            return;
        }
        
        const SpriteAnimation* animation = m_sprite_animation_resource->GetAnimation(target_animation);
        if (!animation) {
            std::cerr << "AnimatedSprite3D: Animation '" << target_animation << "' not found" << std::endl;
            return;
        }
        
        m_current_animation = target_animation;
    } else if (IsUsingDirectSheet()) {
        // For direct sprite sheet, use a default animation name
        m_current_animation = "default";
    } else {
        std::cerr << "AnimatedSprite3D: No sprite animation resource or texture configured" << std::endl;
        return;
    }
    
    m_playback_state = PlaybackState::Playing;
    m_current_frame = 0;
    m_frame_time = 0.0f;
    
    std::cout << "AnimatedSprite3D: Playing animation '" << m_current_animation << "'" << std::endl;
}

void AnimatedSprite3D::Stop() {
    m_playback_state = PlaybackState::Stopped;
    m_current_frame = 0;
    m_frame_time = 0.0f;
}

void AnimatedSprite3D::Pause() {
    if (m_playback_state == PlaybackState::Playing) {
        m_playback_state = PlaybackState::Paused;
    }
}

void AnimatedSprite3D::Resume() {
    if (m_playback_state == PlaybackState::Paused) {
        m_playback_state = PlaybackState::Playing;
    }
}

void AnimatedSprite3D::SetFrame(int frame) {
    if (IsUsingResource() && m_sprite_animation_resource && !m_current_animation.empty()) {
        const SpriteAnimation* animation = m_sprite_animation_resource->GetAnimation(m_current_animation);
        if (animation) {
            m_current_frame = std::clamp(frame, 0, static_cast<int>(animation->frames.size()) - 1);
        }
    } else if (IsUsingDirectSheet()) {
        m_current_frame = std::clamp(frame, 0, m_frame_count - 1);
    }
    
    m_frame_time = 0.0f;
    UpdateMeshVertices(); // Update texture coordinates
}

std::vector<std::string> AnimatedSprite3D::GetAvailableAnimations() const {
    if (IsUsingResource() && m_sprite_animation_resource) {
        return m_sprite_animation_resource->GetAnimationNames();
    }
    
    return {"default"}; // For direct sprite sheet mode
}

glm::vec4 AnimatedSprite3D::GetCurrentTextureRegion() const {
    return CalculateTextureRegion();
}

glm::vec4 AnimatedSprite3D::GetLocalBounds() const {
    glm::vec2 half_size = m_size * 0.5f;
    glm::vec2 sprite_offset = m_offset;

    if (m_centered) {
        sprite_offset -= half_size;
    }

    return glm::vec4(sprite_offset.x, sprite_offset.y, m_size.x, m_size.y);
}

void AnimatedSprite3D::OnReady() {
    UpdateFromExportVariables();
    
    if (IsUsingResource()) {
        LoadSpriteAnimationResource();
    } else if (IsUsingDirectSheet()) {
        LoadTexture();
    }
    
    InitializeMesh();
    
    // Auto-play if enabled
    if (m_auto_play) {
        Play(m_default_animation);
    }
}

void AnimatedSprite3D::OnUpdate(float delta_time) {
    // Check if export variables have changed
    UpdateFromExportVariables();

    // Check rendering context - AnimatedSprite3D should only render in 3D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor2D) {
        return; // Don't render 3D sprites in 2D editor view
    }

    // Load texture if needed
    if (IsUsingResource() && !m_sprite_animation_path.empty() && !m_sprite_animation_resource) {
        LoadSpriteAnimationResource();
    } else if (IsUsingDirectSheet() && !m_texture_path.empty() && !m_texture_loaded) {
        LoadTexture();
    }

    // Initialize mesh if needed
    if (!m_mesh_initialized) {
        InitializeMesh();
    }

    // Update animation
    if (m_playback_state == PlaybackState::Playing) {
        UpdateAnimation(delta_time);
    }

    // Render the sprite
    if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
        RenderSprite(node3d);
    }
}

void AnimatedSprite3D::LoadSpriteAnimationResource() {
    m_sprite_animation_resource.reset();

    if (m_sprite_animation_path.empty()) {
        return;
    }

    m_sprite_animation_resource = std::make_unique<SpriteAnimationResource>();
    if (!m_sprite_animation_resource->LoadFromFile(m_sprite_animation_path)) {
        std::cerr << "AnimatedSprite3D: Failed to load sprite animation resource: " << m_sprite_animation_path << std::endl;
        m_sprite_animation_resource.reset();
    } else {
        std::cout << "AnimatedSprite3D: Loaded sprite animation resource: " << m_sprite_animation_path << std::endl;

        // Load the texture from the resource
        std::string texturePath = m_sprite_animation_resource->GetTexturePath();
        if (!texturePath.empty()) {
            LoadTextureFromPath(texturePath);
        }
    }
}

void AnimatedSprite3D::LoadTexture() {
    if (m_texture_path.empty()) {
        m_texture_loaded = false;
        m_texture = nullptr;
        return;
    }

    LoadTextureFromPath(m_texture_path);
}

void AnimatedSprite3D::LoadTextureFromPath(const std::string& path) {
    // Initialize ResourceManager if not already done
    if (!ResourceManager::IsInitialized()) {
        ResourceManager::Initialize();
    }

    // Load texture using ResourceManager with unified graphics API
    Texture texture = ResourceManager::LoadTexture(path);
    if (texture.IsValid()) {
        m_texture = texture.graphics_texture;
        m_texture_loaded = true;
        std::cout << "AnimatedSprite3D: Loaded texture " << path << std::endl;
    } else {
        m_texture = nullptr;
        m_texture_loaded = false;
        std::cerr << "AnimatedSprite3D: Failed to load texture " << path << std::endl;
    }
}

void AnimatedSprite3D::UpdateFromExportVariables() {
    // Update internal state from export variables
    std::string new_resource_path = GetExportVariableValue<std::string>("sprite_animation_resource", m_sprite_animation_path);
    if (new_resource_path != m_sprite_animation_path) {
        SetSpriteAnimationResource(new_resource_path);
    }

    std::string new_texture_path = GetExportVariableValue<std::string>("texture_path", m_texture_path);
    if (new_texture_path != m_texture_path) {
        SetTexturePath(new_texture_path);
    }

    // Update sprite sheet properties
    glm::vec2 sprite_size_vec2 = GetExportVariableValue<glm::vec2>("sprite_size", glm::vec2(m_sprite_size.x, m_sprite_size.y));
    m_sprite_size = glm::ivec2(sprite_size_vec2.x, sprite_size_vec2.y);

    m_frame_count = GetExportVariableValue<int>("frame_count", m_frame_count);
    m_frames_per_row = GetExportVariableValue<int>("frames_per_row", m_frames_per_row);

    // Update animation properties
    m_auto_play = GetExportVariableValue<bool>("auto_play", m_auto_play);
    m_default_animation = GetExportVariableValue<std::string>("default_animation", m_default_animation);
    m_speed_scale = GetExportVariableValue<float>("speed_scale", m_speed_scale);

    // Update visual properties (matching Sprite3D pattern)
    glm::vec4 new_modulate = GetExportVariableValue<glm::vec4>("modulate", m_modulate);
    if (new_modulate != m_modulate) {
        m_modulate = new_modulate;
    }

    glm::vec2 new_size = GetExportVariableValue<glm::vec2>("size", m_size);
    if (new_size != m_size) {
        m_size = glm::max(new_size, glm::vec2(0.001f));
        UpdateMeshVertices();
    }

    glm::vec2 new_offset = GetExportVariableValue<glm::vec2>("offset", m_offset);
    if (new_offset != m_offset) {
        m_offset = new_offset;
        UpdateMeshVertices();
    }

    bool new_centered = GetExportVariableValue<bool>("centered", m_centered);
    if (new_centered != m_centered) {
        m_centered = new_centered;
        UpdateMeshVertices();
    }

    bool new_flip_h = GetExportVariableValue<bool>("flip_h", m_flip_h);
    if (new_flip_h != m_flip_h) {
        m_flip_h = new_flip_h;
        UpdateMeshVertices();
    }

    bool new_flip_v = GetExportVariableValue<bool>("flip_v", m_flip_v);
    if (new_flip_v != m_flip_v) {
        m_flip_v = new_flip_v;
        UpdateMeshVertices();
    }

    // Update 3D-specific properties
    m_billboard_mode = static_cast<BillboardMode>(GetExportVariableValue<int>("billboard_mode", static_cast<int>(m_billboard_mode)));
    m_alpha_cut_mode = static_cast<AlphaCutMode>(GetExportVariableValue<int>("alpha_cut_mode", static_cast<int>(m_alpha_cut_mode)));
    m_alpha_cut_threshold = GetExportVariableValue<float>("alpha_cut_threshold", m_alpha_cut_threshold);
    m_transparent = GetExportVariableValue<bool>("transparent", m_transparent);
    m_double_sided = GetExportVariableValue<bool>("double_sided", m_double_sided);
    m_receives_lighting = GetExportVariableValue<bool>("receives_lighting", m_receives_lighting);
}

void AnimatedSprite3D::UpdateAnimation(float delta_time) {
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

void AnimatedSprite3D::AdvanceFrame() {
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

    // Update mesh vertices with new texture coordinates
    UpdateMeshVertices();
}

glm::vec4 AnimatedSprite3D::CalculateTextureRegion() const {
    if (IsUsingResource()) {
        return GetFrameRegionFromResource(m_current_frame);
    } else if (IsUsingDirectSheet()) {
        return GetFrameRegionFromSheet(m_current_frame);
    }

    return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // Full texture
}

glm::vec4 AnimatedSprite3D::GetFrameRegionFromSheet(int frame_index) const {
    if (m_frames_per_row <= 0 || m_sprite_size.x <= 0 || m_sprite_size.y <= 0) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    // Special case: if we have only 1 frame, use the entire texture
    if (m_frame_count == 1 && m_frames_per_row == 1) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    // Calculate frame position in grid
    int col = frame_index % m_frames_per_row;
    int row = frame_index / m_frames_per_row;

    // Calculate texture coordinates (assuming texture size can be determined)
    // For now, we'll use the sprite size and frames per row to estimate
    float texture_width = static_cast<float>(m_frames_per_row * m_sprite_size.x);
    float texture_height = static_cast<float>((m_frame_count / m_frames_per_row + 1) * m_sprite_size.y);

    float u = static_cast<float>(col * m_sprite_size.x) / texture_width;
    float v = static_cast<float>(row * m_sprite_size.y) / texture_height;
    float w = static_cast<float>(m_sprite_size.x) / texture_width;
    float h = static_cast<float>(m_sprite_size.y) / texture_height;

    return glm::vec4(u, v, w, h);
}

glm::vec4 AnimatedSprite3D::GetFrameRegionFromResource(int frame_index) const {
    if (!m_sprite_animation_resource || m_current_animation.empty()) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    const SpriteAnimation* animation = m_sprite_animation_resource->GetAnimation(m_current_animation);
    if (!animation || frame_index < 0 || frame_index >= static_cast<int>(animation->frames.size())) {
        return glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }

    return animation->frames[frame_index].texture_region;
}

void AnimatedSprite3D::InitializeMesh() {
    if (m_mesh_initialized) {
        return;
    }

    // Get graphics device
    auto graphics_device = GraphicsDeviceFactory::GetDevice();
    if (!graphics_device) {
        std::cerr << "AnimatedSprite3D: No graphics device available" << std::endl;
        return;
    }

    try {
        // Create vertex array
        m_vertex_array = graphics_device->CreateVertexArray();
        if (!m_vertex_array) {
            std::cerr << "AnimatedSprite3D: Failed to create vertex array" << std::endl;
            return;
        }

        // Create vertex buffer (will be updated in UpdateMeshVertices)
        m_vertex_buffer = graphics_device->CreateBuffer(
            BufferType::Vertex,
            BufferUsage::Dynamic,
            4 * 8 * sizeof(float), // 4 vertices, 8 floats each (pos3, normal3, uv2)
            nullptr
        );
        if (!m_vertex_buffer) {
            std::cerr << "AnimatedSprite3D: Failed to create vertex buffer" << std::endl;
            return;
        }

        // Create index buffer
        unsigned int indices[] = {
            0, 1, 2,  // First triangle
            2, 3, 0   // Second triangle
        };
        m_index_buffer = graphics_device->CreateBuffer(
            BufferType::Index,
            BufferUsage::Static,
            sizeof(indices),
            indices
        );
        if (!m_index_buffer) {
            std::cerr << "AnimatedSprite3D: Failed to create index buffer" << std::endl;
            return;
        }

        // Set up vertex layout
        VertexLayout layout;
        layout.AddAttribute(0, 3, static_cast<uint32_t>(VertexAttributeType::Float), false); // Position
        layout.AddAttribute(1, 3, static_cast<uint32_t>(VertexAttributeType::Float), false); // Normal
        layout.AddAttribute(2, 2, static_cast<uint32_t>(VertexAttributeType::Float), false); // Texture coordinates

        // Set up vertex array with buffers and layout
        m_vertex_array->Bind();
        m_vertex_array->SetVertexBuffer(m_vertex_buffer, layout);
        m_vertex_array->SetIndexBuffer(m_index_buffer);

        m_mesh_initialized = true;
        UpdateMeshVertices();

    } catch (const std::exception& e) {
        std::cerr << "AnimatedSprite3D: Exception during mesh initialization: " << e.what() << std::endl;
        // Cleanup on error
        m_vertex_array = nullptr;
        m_vertex_buffer = nullptr;
        m_index_buffer = nullptr;
    }
}

void AnimatedSprite3D::UpdateMeshVertices() {
    if (!m_mesh_initialized) {
        return;
    }

    // Calculate sprite bounds
    glm::vec2 half_size = m_size * 0.5f;
    glm::vec2 sprite_offset = m_offset;

    if (m_centered) {
        sprite_offset -= half_size;
    }

    // Calculate vertex positions
    float left = sprite_offset.x;
    float right = sprite_offset.x + m_size.x;
    float bottom = sprite_offset.y;
    float top = sprite_offset.y + m_size.y;

    // Get current texture region
    glm::vec4 tex_region = CalculateTextureRegion();
    float tex_left = tex_region.x;
    float tex_bottom = tex_region.y;
    float tex_right = tex_region.x + tex_region.z;
    float tex_top = tex_region.y + tex_region.w;

    // Apply flipping
    if (m_flip_h) {
        std::swap(tex_left, tex_right);
    }
    if (m_flip_v) {
        std::swap(tex_bottom, tex_top);
    }

    // Define vertices (position, normal, texture coordinates)
    float vertices[] = {
        // Bottom-left
        left, bottom, 0.0f,   0.0f, 0.0f, 1.0f,   tex_left, tex_bottom,
        // Bottom-right
        right, bottom, 0.0f,  0.0f, 0.0f, 1.0f,   tex_right, tex_bottom,
        // Top-right
        right, top, 0.0f,     0.0f, 0.0f, 1.0f,   tex_right, tex_top,
        // Top-left
        left, top, 0.0f,      0.0f, 0.0f, 1.0f,   tex_left, tex_top
    };

    // Update vertex buffer with new vertex data
    if (m_vertex_buffer) {
        m_vertex_buffer->UpdateData(vertices, sizeof(vertices));
    }
}

void AnimatedSprite3D::RenderSprite(Node3D* node3d) {
    if (!m_mesh_initialized) {
        std::cerr << "AnimatedSprite3D: Mesh not initialized for rendering" << std::endl;
        return;
    }

    // Debug: Log rendering attempt
    static bool debug_logged = false;
    if (!debug_logged) {
        std::cout << "AnimatedSprite3D: Attempting to render sprite" << std::endl;
        std::cout << "AnimatedSprite3D: Texture loaded: " << m_texture_loaded << std::endl;
        std::cout << "AnimatedSprite3D: Texture path: " << m_texture_path << std::endl;
        std::cout << "AnimatedSprite3D: Using resource: " << IsUsingResource() << std::endl;
        std::cout << "AnimatedSprite3D: Using direct sheet: " << IsUsingDirectSheet() << std::endl;
        std::cout << "AnimatedSprite3D: Frame count: " << m_frame_count << std::endl;
        std::cout << "AnimatedSprite3D: Frames per row: " << m_frames_per_row << std::endl;
        std::cout << "AnimatedSprite3D: Sprite size: " << m_sprite_size.x << "x" << m_sprite_size.y << std::endl;
        debug_logged = true;
    }

    // Calculate transform matrix
    glm::mat4 node_transform = node3d->GetGlobalTransform();
    glm::mat4 final_transform = node_transform;

    // Apply billboard transformation if needed
    if (m_billboard_mode != BillboardMode::Disabled) {
        final_transform = CalculateBillboardTransform(node_transform);
    }

    // Create a temporary mesh for rendering with unified graphics API
    Mesh temp_mesh;
    temp_mesh.VAO = m_vertex_array;
    temp_mesh.vertices.resize(4); // Just for size info
    temp_mesh.indices.resize(6);  // Just for size info

    // Debug: Check texture and modulate values
    static bool debug_render_logged = false;
    if (!debug_render_logged) {
        std::cout << "AnimatedSprite3D: Modulate color: " << m_modulate.r << ", " << m_modulate.g << ", " << m_modulate.b << ", " << m_modulate.a << std::endl;
        if (m_texture_loaded && m_texture && m_texture->IsValid()) {
            std::cout << "AnimatedSprite3D: Rendering with texture, handle: " << m_texture->GetNativeHandle() << std::endl;
        } else {
            std::cout << "AnimatedSprite3D: Rendering with white texture" << std::endl;
        }
        debug_render_logged = true;
    }

    // Use white texture if no texture is loaded
    std::shared_ptr<GraphicsTexture> texture_to_use = (m_texture_loaded && m_texture && m_texture->IsValid()) ?
                                                       m_texture : Renderer::GetWhiteTexture();

    // Render the mesh with texture
    Renderer::RenderMesh(temp_mesh, final_transform, m_modulate, texture_to_use, m_receives_lighting);
}

glm::mat4 AnimatedSprite3D::CalculateBillboardTransform(const glm::mat4& node_transform) const {
    // Extract position from node transform
    glm::vec3 position = glm::vec3(node_transform[3]);

    // Extract scale from node transform
    glm::vec3 scale = glm::vec3(
        glm::length(glm::vec3(node_transform[0])),
        glm::length(glm::vec3(node_transform[1])),
        glm::length(glm::vec3(node_transform[2]))
    );

    // Get camera information from renderer
    glm::mat4 view_matrix = Renderer::GetViewMatrix();
    glm::mat4 inv_view = glm::inverse(view_matrix);

    // Extract camera vectors from inverse view matrix
    glm::vec3 camera_right = glm::normalize(glm::vec3(inv_view[0]));
    glm::vec3 camera_up = glm::normalize(glm::vec3(inv_view[1]));
    glm::vec3 camera_forward = -glm::normalize(glm::vec3(inv_view[2]));

    glm::mat4 billboard_transform = glm::mat4(1.0f);

    switch (m_billboard_mode) {
        case BillboardMode::Enabled: {
            // Full billboard - always face camera
            billboard_transform[0] = glm::vec4(camera_right * scale.x, 0.0f);
            billboard_transform[1] = glm::vec4(camera_up * scale.y, 0.0f);
            billboard_transform[2] = glm::vec4(camera_forward * scale.z, 0.0f);
            billboard_transform[3] = glm::vec4(position, 1.0f);
            break;
        }
        case BillboardMode::YBillboard: {
            // Y-axis billboard - only rotate around Y axis to face camera
            glm::vec3 to_camera = glm::normalize(glm::vec3(inv_view[3]) - position);
            to_camera.y = 0.0f; // Remove Y component
            to_camera = glm::normalize(to_camera);

            glm::vec3 right = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), to_camera);
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 forward = to_camera;

            billboard_transform[0] = glm::vec4(right * scale.x, 0.0f);
            billboard_transform[1] = glm::vec4(up * scale.y, 0.0f);
            billboard_transform[2] = glm::vec4(forward * scale.z, 0.0f);
            billboard_transform[3] = glm::vec4(position, 1.0f);
            break;
        }
        case BillboardMode::ParticlesBillboard: {
            // Special billboard mode for particles (same as full billboard for now)
            billboard_transform[0] = glm::vec4(camera_right * scale.x, 0.0f);
            billboard_transform[1] = glm::vec4(camera_up * scale.y, 0.0f);
            billboard_transform[2] = glm::vec4(camera_forward * scale.z, 0.0f);
            billboard_transform[3] = glm::vec4(position, 1.0f);
            break;
        }
        default:
            return node_transform;
    }

    return billboard_transform;
}

uint32_t AnimatedSprite3D::GetTextureHandle() const {
    return (m_texture && m_texture->IsValid()) ? m_texture->GetNativeHandle() : 0;
}

} // namespace Lupine
