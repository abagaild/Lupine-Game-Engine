#pragma once

#include "lupine/core/Component.h"
#include "lupine/rendering/GraphicsTexture.h"
#include <glm/glm.hpp>
#include <memory>

namespace Lupine {

/**
 * @brief 2D sprite component for rendering textures
 * 
 * Sprite2D component renders a texture on a 2D quad.
 * It should be attached to Node2D nodes.
 */
class Sprite2D : public Component {
public:
    /**
     * @brief Constructor
     */
    Sprite2D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Sprite2D() = default;
    
    /**
     * @brief Get texture path
     * @return Path to texture file
     */
    const std::string& GetTexturePath() const { return m_texture_path; }
    
    /**
     * @brief Set texture path
     * @param path Path to texture file
     */
    void SetTexturePath(const std::string& path);
    
    /**
     * @brief Get sprite color tint
     * @return Color tint (RGBA)
     */
    const glm::vec4& GetColor() const { return m_color; }
    
    /**
     * @brief Set sprite color tint
     * @param color Color tint (RGBA)
     */
    void SetColor(const glm::vec4& color);

    /**
     * @brief Get sprite modulate color
     * @return Modulate color (RGBA) - affects entire sprite including opacity
     */
    const glm::vec4& GetModulate() const { return m_modulate; }

    /**
     * @brief Set sprite modulate color
     * @param modulate Modulate color (RGBA) - affects entire sprite including opacity
     */
    void SetModulate(const glm::vec4& modulate);

    /**
     * @brief Get sprite opacity (alpha component of modulate)
     * @return Opacity value (0.0 = transparent, 1.0 = opaque)
     */
    float GetOpacity() const { return m_modulate.a; }

    /**
     * @brief Set sprite opacity (alpha component of modulate)
     * @param opacity Opacity value (0.0 = transparent, 1.0 = opaque)
     */
    void SetOpacity(float opacity);
    
    /**
     * @brief Get sprite size
     * @return Sprite size in pixels
     */
    const glm::vec2& GetSize() const { return m_size; }
    
    /**
     * @brief Set sprite size
     * @param size Sprite size in pixels
     */
    void SetSize(const glm::vec2& size);
    
    /**
     * @brief Get texture region (UV coordinates)
     * @return Texture region (x, y, width, height) in 0-1 range
     */
    const glm::vec4& GetTextureRegion() const { return m_texture_region; }
    
    /**
     * @brief Set texture region (UV coordinates)
     * @param region Texture region (x, y, width, height) in 0-1 range
     */
    void SetTextureRegion(const glm::vec4& region);
    
    /**
     * @brief Get flip horizontal flag
     * @return True if horizontally flipped
     */
    bool GetFlipH() const { return m_flip_h; }
    
    /**
     * @brief Set flip horizontal flag
     * @param flip_h True to flip horizontally
     */
    void SetFlipH(bool flip_h);
    
    /**
     * @brief Get flip vertical flag
     * @return True if vertically flipped
     */
    bool GetFlipV() const { return m_flip_v; }
    
    /**
     * @brief Set flip vertical flag
     * @param flip_v True to flip vertically
     */
    void SetFlipV(bool flip_v);

    /**
     * @brief Get centered flag
     * @return True if sprite is centered
     */
    bool GetCentered() const { return m_centered; }

    /**
     * @brief Set centered flag
     * @param centered True to center sprite
     */
    void SetCentered(bool centered);

    /**
     * @brief Get sprite offset
     * @return Sprite offset in pixels
     */
    const glm::vec2& GetOffset() const { return m_offset; }

    /**
     * @brief Set sprite offset
     * @param offset Sprite offset in pixels
     */
    void SetOffset(const glm::vec2& offset);

    /**
     * @brief Get current frame
     * @return Current frame index
     */
    int GetFrame() const { return m_frame; }

    /**
     * @brief Set current frame
     * @param frame Frame index
     */
    void SetFrame(int frame);

    /**
     * @brief Get frame coordinates
     * @return Frame coordinates (column, row)
     */
    const glm::ivec2& GetFrameCoords() const { return m_frame_coords; }

    /**
     * @brief Set frame coordinates
     * @param frame_coords Frame coordinates (column, row)
     */
    void SetFrameCoords(const glm::ivec2& frame_coords);

    /**
     * @brief Get horizontal frames count
     * @return Number of horizontal frames
     */
    int GetHFrames() const { return m_hframes; }

    /**
     * @brief Set horizontal frames count
     * @param hframes Number of horizontal frames
     */
    void SetHFrames(int hframes);

    /**
     * @brief Get vertical frames count
     * @return Number of vertical frames
     */
    int GetVFrames() const { return m_vframes; }

    /**
     * @brief Set vertical frames count
     * @param vframes Number of vertical frames
     */
    void SetVFrames(int vframes);

    /**
     * @brief Get region enabled flag
     * @return True if region is enabled
     */
    bool GetRegionEnabled() const { return m_region_enabled; }

    /**
     * @brief Set region enabled flag
     * @param region_enabled True to enable region
     */
    void SetRegionEnabled(bool region_enabled);

    /**
     * @brief Get region filter clip enabled flag
     * @return True if region filter clip is enabled
     */
    bool GetRegionFilterClipEnabled() const { return m_region_filter_clip_enabled; }

    /**
     * @brief Set region filter clip enabled flag
     * @param region_filter_clip_enabled True to enable region filter clip
     */
    void SetRegionFilterClipEnabled(bool region_filter_clip_enabled);

    /**
     * @brief Get region rectangle
     * @return Region rectangle (x, y, width, height) in pixels
     */
    const glm::vec4& GetRegionRect() const { return m_region_rect; }

    /**
     * @brief Set region rectangle
     * @param region_rect Region rectangle (x, y, width, height) in pixels
     */
    void SetRegionRect(const glm::vec4& region_rect);

    /**
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Sprite2D"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "2D"; }

    /**
     * @brief Get sprite rectangle in local coordinates
     * @return Rectangle representing the sprite's boundary
     */
    glm::vec4 GetRect() const;

    /**
     * @brief Check if pixel at given position is opaque
     * @param pos Position in local coordinates
     * @return True if pixel is opaque, false otherwise
     */
    bool IsPixelOpaque(const glm::vec2& pos) const;

    // Lifecycle methods
    void OnReady() override;
    void OnUpdate(float delta_time) override;

protected:
    void InitializeExportVariables() override;

private:
    std::string m_texture_path;
    glm::vec4 m_color;
    glm::vec4 m_modulate;  // Overall color modulation (includes opacity)
    glm::vec2 m_size;
    glm::vec4 m_texture_region;
    bool m_flip_h;
    bool m_flip_v;

    // Additional Godot-like properties
    bool m_centered;
    glm::vec2 m_offset;
    int m_frame;
    glm::ivec2 m_frame_coords;
    int m_hframes;
    int m_vframes;
    bool m_region_enabled;
    bool m_region_filter_clip_enabled;
    glm::vec4 m_region_rect;  // x, y, width, height in pixels

    // Internal rendering data
    std::shared_ptr<GraphicsTexture> m_texture;
    bool m_texture_loaded;
    
    /**
     * @brief Load texture from file
     */
    void LoadTexture();
    
    /**
     * @brief Update export variables from internal state
     */
    void UpdateExportVariables();
    
    /**
     * @brief Update internal state from export variables
     */
    void UpdateFromExportVariables();

    /**
     * @brief Calculate final texture region based on frame animation and region settings
     * @return Texture region (x, y, width, height) in 0-1 range
     */
    glm::vec4 CalculateTextureRegion() const;

    /**
     * @brief Calculate transform matrix for rendering
     * @return Transform matrix
     */
    glm::mat4 CalculateTransform() const;
};

} // namespace Lupine
