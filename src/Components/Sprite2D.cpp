#include "lupine/components/Sprite2D.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/resources/ResourceManager.h"
#include <iostream>
#include <algorithm>

namespace Lupine {

Sprite2D::Sprite2D()
    : Component("Sprite2D")
    , m_texture_path("")
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_modulate(1.0f, 1.0f, 1.0f, 1.0f)
    , m_size(100.0f, 100.0f)
    , m_texture_region(0.0f, 0.0f, 1.0f, 1.0f)
    , m_flip_h(false)
    , m_flip_v(false)
    , m_centered(true)
    , m_offset(0.0f, 0.0f)
    , m_frame(0)
    , m_frame_coords(0, 0)
    , m_hframes(1)
    , m_vframes(1)
    , m_region_enabled(false)
    , m_region_filter_clip_enabled(false)
    , m_region_rect(0.0f, 0.0f, 0.0f, 0.0f)
    , m_texture(nullptr)
    , m_texture_loaded(false) {

    // Initialize export variables
    InitializeExportVariables();
}

void Sprite2D::SetTexturePath(const std::string& path) {
    m_texture_path = path;
    SetExportVariable("texture_path", path);
    m_texture_loaded = false;
    LoadTexture();
}

void Sprite2D::SetColor(const glm::vec4& color) {
    m_color = color;
    SetExportVariable("color", color);
}

void Sprite2D::SetModulate(const glm::vec4& modulate) {
    m_modulate = modulate;
    SetExportVariable("modulate", modulate);
}

void Sprite2D::SetOpacity(float opacity) {
    m_modulate.a = opacity;
    SetExportVariable("modulate", m_modulate);
}

void Sprite2D::SetSize(const glm::vec2& size) {
    m_size = size;
    SetExportVariable("size", size);
}

void Sprite2D::SetTextureRegion(const glm::vec4& region) {
    m_texture_region = region;
    SetExportVariable("texture_region", region);
}

void Sprite2D::SetFlipH(bool flip_h) {
    m_flip_h = flip_h;
    SetExportVariable("flip_h", flip_h);
}

void Sprite2D::SetFlipV(bool flip_v) {
    m_flip_v = flip_v;
    SetExportVariable("flip_v", flip_v);
}

void Sprite2D::SetCentered(bool centered) {
    m_centered = centered;
    SetExportVariable("centered", centered);
}

void Sprite2D::SetOffset(const glm::vec2& offset) {
    m_offset = offset;
    SetExportVariable("offset", offset);
}

void Sprite2D::SetFrame(int frame) {
    m_frame = frame;
    SetExportVariable("frame", frame);

    // Update frame coordinates based on frame index
    if (m_hframes > 0 && m_vframes > 0) {
        m_frame_coords.x = frame % m_hframes;
        m_frame_coords.y = frame / m_hframes;
        SetExportVariable("frame_coords", glm::vec2(m_frame_coords.x, m_frame_coords.y));
    }
}

void Sprite2D::SetFrameCoords(const glm::ivec2& frame_coords) {
    m_frame_coords = frame_coords;
    SetExportVariable("frame_coords", glm::vec2(frame_coords.x, frame_coords.y));

    // Update frame index based on coordinates
    m_frame = frame_coords.y * m_hframes + frame_coords.x;
    SetExportVariable("frame", m_frame);
}

void Sprite2D::SetHFrames(int hframes) {
    m_hframes = hframes;
    SetExportVariable("hframes", hframes);

    // Recalculate frame coordinates
    if (hframes > 0) {
        SetFrame(m_frame); // This will update frame_coords
    }
}

void Sprite2D::SetVFrames(int vframes) {
    m_vframes = vframes;
    SetExportVariable("vframes", vframes);

    // Recalculate frame coordinates
    if (vframes > 0) {
        SetFrame(m_frame); // This will update frame_coords
    }
}

void Sprite2D::SetRegionEnabled(bool region_enabled) {
    m_region_enabled = region_enabled;
    SetExportVariable("region_enabled", region_enabled);
}

void Sprite2D::SetRegionFilterClipEnabled(bool region_filter_clip_enabled) {
    m_region_filter_clip_enabled = region_filter_clip_enabled;
    SetExportVariable("region_filter_clip_enabled", region_filter_clip_enabled);
}

void Sprite2D::SetRegionRect(const glm::vec4& region_rect) {
    m_region_rect = region_rect;
    SetExportVariable("region_rect", region_rect);
}

void Sprite2D::OnReady() {
    UpdateFromExportVariables();
    LoadTexture();
}

void Sprite2D::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();

    // Always render - use white texture if no texture is loaded
    std::shared_ptr<GraphicsTexture> texture_to_use = (m_texture_loaded && m_texture && m_texture->IsValid()) ?
                                                       m_texture : Renderer::GetWhiteTexture();

    // Check rendering context - Sprite2D should only render in 2D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor3D) {
        return; // Don't render 2D sprites in 3D editor view
    }

    // Calculate final texture region based on frame animation and region settings
    glm::vec4 final_texture_region = CalculateTextureRegion();

    // Build transform matrix
    glm::mat4 transform = CalculateTransform();

    // Calculate final color (color * modulate)
    glm::vec4 final_color = m_color * m_modulate;

    // Render sprite with texture
    Renderer::RenderQuad(transform, final_color, texture_to_use, final_texture_region, m_flip_h, m_flip_v);
}

glm::vec4 Sprite2D::CalculateTextureRegion() const {
    glm::vec4 region = m_texture_region;

    // If region is enabled, use the region_rect instead
    if (m_region_enabled && m_texture_loaded) {
        // Get texture dimensions from ResourceManager
        Texture texture = ResourceManager::GetTexture(m_texture_path);
        if (texture.IsValid()) {
            // Convert pixel coordinates to normalized coordinates
            region.x = m_region_rect.x / texture.width;
            region.y = m_region_rect.y / texture.height;
            region.z = m_region_rect.z / texture.width;
            region.w = m_region_rect.w / texture.height;
        }
    }

    // Apply frame animation if we have multiple frames
    if (m_hframes > 1 || m_vframes > 1) {
        // Calculate frame size in normalized coordinates
        float frame_width = region.z / m_hframes;
        float frame_height = region.w / m_vframes;

        // Calculate frame position
        int frame_x = m_frame_coords.x;
        int frame_y = m_frame_coords.y;

        // Clamp frame coordinates
        frame_x = std::max(0, std::min(frame_x, m_hframes - 1));
        frame_y = std::max(0, std::min(frame_y, m_vframes - 1));

        // Calculate final region
        region.x = region.x + frame_x * frame_width;
        region.y = region.y + frame_y * frame_height;
        region.z = frame_width;
        region.w = frame_height;
    }

    return region;
}

glm::mat4 Sprite2D::CalculateTransform() const {
    glm::mat4 transform = glm::mat4(1.0f);

    if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
        // Use Node2D transform properties
        glm::vec2 pos = node2d->GetGlobalPosition();

        // Apply offset
        pos += m_offset;

        transform = glm::translate(transform, glm::vec3(pos.x, pos.y, 0.0f));

        float rotation = node2d->GetGlobalRotation();
        transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));

        glm::vec2 scale = node2d->GetGlobalScale();
        // Use actual sprite size
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

glm::vec4 Sprite2D::GetRect() const {
    // Calculate sprite rectangle in local coordinates
    glm::vec2 size = m_size;
    glm::vec2 offset = m_offset;

    if (m_centered) {
        // Centered sprite: rectangle extends from -size/2 to +size/2, plus offset
        return glm::vec4(
            offset.x - size.x * 0.5f,  // x
            offset.y - size.y * 0.5f,  // y
            size.x,                    // width
            size.y                     // height
        );
    } else {
        // Non-centered sprite: rectangle starts at offset
        return glm::vec4(
            offset.x,      // x
            offset.y,      // y
            size.x,        // width
            size.y         // height
        );
    }
}

bool Sprite2D::IsPixelOpaque(const glm::vec2& pos) const {
    // If no texture is loaded, return false
    if (!m_texture_loaded || !m_texture || !m_texture->IsValid()) {
        return false;
    }

    // Get sprite rectangle
    glm::vec4 rect = GetRect();

    // Check if position is within sprite bounds
    if (pos.x < rect.x || pos.x > rect.x + rect.z ||
        pos.y < rect.y || pos.y > rect.y + rect.w) {
        return false;
    }

    // Convert local position to UV coordinates
    float u = (pos.x - rect.x) / rect.z;
    float v = (pos.y - rect.y) / rect.w;

    // Apply flipping
    if (m_flip_h) {
        u = 1.0f - u;
    }
    if (m_flip_v) {
        v = 1.0f - v;
    }

    // Get current texture region
    glm::vec4 texture_region = CalculateTextureRegion();

    // Convert to texture coordinates
    u = texture_region.x + u * texture_region.z;
    v = texture_region.y + v * texture_region.w;

    // For now, we can't easily check pixel opacity without loading texture data
    // This would require reading texture data from GPU or keeping a copy in memory
    // For simplicity, assume all pixels within bounds are opaque
    // TODO: Implement proper pixel opacity checking if needed
    return true;
}

void Sprite2D::InitializeExportVariables() {
    AddExportVariable("texture_path", m_texture_path, "Path to texture file", ExportVariableType::FilePath);
    AddExportVariable("color", m_color, "Color tint (RGBA)", ExportVariableType::Color);
    AddExportVariable("modulate", m_modulate, "Modulate color (RGBA) - affects entire sprite including opacity", ExportVariableType::Color);
    AddExportVariable("size", m_size, "Sprite size in pixels", ExportVariableType::Vec2);
    AddExportVariable("texture_region", m_texture_region, "Texture region (x, y, width, height) in 0-1 range", ExportVariableType::Vec4);
    AddExportVariable("flip_h", m_flip_h, "Flip horizontally", ExportVariableType::Bool);
    AddExportVariable("flip_v", m_flip_v, "Flip vertically", ExportVariableType::Bool);

    // Additional Godot-like properties
    AddExportVariable("centered", m_centered, "Center the sprite", ExportVariableType::Bool);
    AddExportVariable("offset", m_offset, "Sprite offset in pixels", ExportVariableType::Vec2);
    AddExportVariable("frame", m_frame, "Current frame index", ExportVariableType::Int);
    AddExportVariable("frame_coords", glm::vec2(m_frame_coords.x, m_frame_coords.y), "Frame coordinates (column, row)", ExportVariableType::Vec2);
    AddExportVariable("hframes", m_hframes, "Number of horizontal frames", ExportVariableType::Int);
    AddExportVariable("vframes", m_vframes, "Number of vertical frames", ExportVariableType::Int);
    AddExportVariable("region_enabled", m_region_enabled, "Enable texture region", ExportVariableType::Bool);
    AddExportVariable("region_filter_clip_enabled", m_region_filter_clip_enabled, "Enable region filter clipping", ExportVariableType::Bool);
    AddExportVariable("region_rect", m_region_rect, "Region rectangle (x, y, width, height) in pixels", ExportVariableType::Vec4);
}

void Sprite2D::LoadTexture() {
    if (m_texture_path.empty()) {
        m_texture_loaded = false;
        m_texture = nullptr;
        return;
    }

    // Initialize ResourceManager if not already done
    if (!ResourceManager::IsInitialized()) {
        ResourceManager::Initialize();
    }

    // Load texture using ResourceManager with unified graphics API
    Texture texture = ResourceManager::LoadTexture(m_texture_path);
    if (texture.IsValid()) {
        m_texture = texture.graphics_texture;
        m_texture_loaded = true;
    } else {
        m_texture = nullptr;
        m_texture_loaded = false;
    }
}

void Sprite2D::UpdateExportVariables() {
    SetExportVariable("texture_path", m_texture_path);
    SetExportVariable("color", m_color);
    SetExportVariable("modulate", m_modulate);
    SetExportVariable("size", m_size);
    SetExportVariable("texture_region", m_texture_region);
    SetExportVariable("flip_h", m_flip_h);
    SetExportVariable("flip_v", m_flip_v);

    // Additional Godot-like properties
    SetExportVariable("centered", m_centered);
    SetExportVariable("offset", m_offset);
    SetExportVariable("frame", m_frame);
    SetExportVariable("frame_coords", glm::vec2(m_frame_coords.x, m_frame_coords.y));
    SetExportVariable("hframes", m_hframes);
    SetExportVariable("vframes", m_vframes);
    SetExportVariable("region_enabled", m_region_enabled);
    SetExportVariable("region_filter_clip_enabled", m_region_filter_clip_enabled);
    SetExportVariable("region_rect", m_region_rect);
}

void Sprite2D::UpdateFromExportVariables() {
    std::string new_texture_path = GetExportVariableValue<std::string>("texture_path", m_texture_path);
    if (new_texture_path != m_texture_path) {
        m_texture_path = new_texture_path;
        m_texture_loaded = false;
        LoadTexture();
    }

    m_color = GetExportVariableValue<glm::vec4>("color", m_color);
    m_modulate = GetExportVariableValue<glm::vec4>("modulate", m_modulate);
    m_size = GetExportVariableValue<glm::vec2>("size", m_size);
    m_texture_region = GetExportVariableValue<glm::vec4>("texture_region", m_texture_region);
    m_flip_h = GetExportVariableValue<bool>("flip_h", m_flip_h);
    m_flip_v = GetExportVariableValue<bool>("flip_v", m_flip_v);

    // Additional Godot-like properties
    m_centered = GetExportVariableValue<bool>("centered", m_centered);
    m_offset = GetExportVariableValue<glm::vec2>("offset", m_offset);
    m_frame = GetExportVariableValue<int>("frame", m_frame);
    glm::vec2 frame_coords_vec2 = GetExportVariableValue<glm::vec2>("frame_coords", glm::vec2(m_frame_coords.x, m_frame_coords.y));
    m_frame_coords = glm::ivec2(frame_coords_vec2.x, frame_coords_vec2.y);
    m_hframes = GetExportVariableValue<int>("hframes", m_hframes);
    m_vframes = GetExportVariableValue<int>("vframes", m_vframes);
    m_region_enabled = GetExportVariableValue<bool>("region_enabled", m_region_enabled);
    m_region_filter_clip_enabled = GetExportVariableValue<bool>("region_filter_clip_enabled", m_region_filter_clip_enabled);
    m_region_rect = GetExportVariableValue<glm::vec4>("region_rect", m_region_rect);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(Sprite2D, "2D", "2D sprite component for displaying textures")
