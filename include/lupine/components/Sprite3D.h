#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace Lupine {

// Forward declarations
class Node3D;
class GraphicsTexture;
class GraphicsVertexArray;
class GraphicsBuffer;

/**
 * @brief Sprite3D component for rendering 2D textures in 3D space
 *
 * Sprite3D component provides Godot-like functionality for displaying
 * 2D sprites in 3D space with billboard behavior, proper depth handling,
 * and various alignment options. It should be attached to Node3D nodes.
 */
class Sprite3D : public Component {
public:
    /**
     * @brief Billboard mode options
     */
    enum class BillboardMode {
        Disabled,       // No billboard behavior
        Enabled,        // Always face camera
        YBillboard,     // Only rotate around Y axis
        ParticlesBillboard  // Special mode for particles
    };

    /**
     * @brief Alpha cut mode for transparency
     */
    enum class AlphaCutMode {
        Disabled,       // No alpha cutting
        Discard,        // Discard pixels below threshold
        OpaquePrepass   // Two-pass rendering
    };

    /**
     * @brief Constructor
     */
    Sprite3D();

    /**
     * @brief Virtual destructor
     */
    virtual ~Sprite3D() = default;

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
     * @brief Get sprite color modulation
     * @return Color modulation (RGBA)
     */
    const glm::vec4& GetModulate() const { return m_modulate; }

    /**
     * @brief Set sprite color modulation
     * @param modulate Color modulation (RGBA)
     */
    void SetModulate(const glm::vec4& modulate);

    /**
     * @brief Get sprite size
     * @return Sprite size in world units
     */
    const glm::vec2& GetSize() const { return m_size; }

    /**
     * @brief Set sprite size
     * @param size Sprite size in world units
     */
    void SetSize(const glm::vec2& size);

    /**
     * @brief Get sprite offset
     * @return Sprite offset from node position
     */
    const glm::vec2& GetOffset() const { return m_offset; }

    /**
     * @brief Set sprite offset
     * @param offset Sprite offset from node position
     */
    void SetOffset(const glm::vec2& offset);

    /**
     * @brief Get whether sprite is centered
     * @return True if sprite is centered on node position
     */
    bool IsCentered() const { return m_centered; }

    /**
     * @brief Set whether sprite is centered
     * @param centered True to center sprite on node position
     */
    void SetCentered(bool centered);

    /**
     * @brief Get horizontal flip state
     * @return True if horizontally flipped
     */
    bool IsFlippedH() const { return m_flip_h; }

    /**
     * @brief Set horizontal flip state
     * @param flip_h True to flip horizontally
     */
    void SetFlippedH(bool flip_h);

    /**
     * @brief Get vertical flip state
     * @return True if vertically flipped
     */
    bool IsFlippedV() const { return m_flip_v; }

    /**
     * @brief Set vertical flip state
     * @param flip_v True to flip vertically
     */
    void SetFlippedV(bool flip_v);

    /**
     * @brief Get billboard mode
     * @return Current billboard mode
     */
    BillboardMode GetBillboardMode() const { return m_billboard_mode; }

    /**
     * @brief Set billboard mode
     * @param mode Billboard mode
     */
    void SetBillboardMode(BillboardMode mode);

    /**
     * @brief Get alpha cut mode
     * @return Current alpha cut mode
     */
    AlphaCutMode GetAlphaCutMode() const { return m_alpha_cut_mode; }

    /**
     * @brief Set alpha cut mode
     * @param mode Alpha cut mode
     */
    void SetAlphaCutMode(AlphaCutMode mode);

    /**
     * @brief Get alpha cut threshold
     * @return Alpha threshold (0.0 to 1.0)
     */
    float GetAlphaCutThreshold() const { return m_alpha_cut_threshold; }

    /**
     * @brief Set alpha cut threshold
     * @param threshold Alpha threshold (0.0 to 1.0)
     */
    void SetAlphaCutThreshold(float threshold);

    /**
     * @brief Get whether sprite is transparent
     * @return True if sprite uses transparency
     */
    bool IsTransparent() const { return m_transparent; }

    /**
     * @brief Set whether sprite is transparent
     * @param transparent True to enable transparency
     */
    void SetTransparent(bool transparent);

    /**
     * @brief Get whether sprite is double-sided
     * @return True if sprite is rendered on both sides
     */
    bool IsDoubleSided() const { return m_double_sided; }

    /**
     * @brief Set whether sprite is double-sided
     * @param double_sided True to render on both sides
     */
    void SetDoubleSided(bool double_sided);

    /**
     * @brief Get whether sprite receives lighting
     * @return True if sprite receives lighting
     */
    bool GetReceivesLighting() const { return m_receives_lighting; }

    /**
     * @brief Set whether sprite receives lighting
     * @param receives_lighting True to enable lighting, false for unlit rendering
     */
    void SetReceivesLighting(bool receives_lighting);

    /**
     * @brief Get texture region
     * @return Texture region (x, y, width, height) in normalized coordinates
     */
    const glm::vec4& GetRegionRect() const { return m_region_rect; }

    /**
     * @brief Set texture region
     * @param region Texture region (x, y, width, height) in normalized coordinates
     */
    void SetRegionRect(const glm::vec4& region);

    /**
     * @brief Get whether region is enabled
     * @return True if using texture region
     */
    bool IsRegionEnabled() const { return m_region_enabled; }

    /**
     * @brief Set whether region is enabled
     * @param enabled True to use texture region
     */
    void SetRegionEnabled(bool enabled);

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "Sprite3D"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "3D"; }

    // Component lifecycle methods
    virtual void OnReady() override;
    virtual void OnUpdate(float delta_time) override;

protected:
    /**
     * @brief Initialize export variables
     */
    virtual void InitializeExportVariables() override;

    /**
     * @brief Update export variables
     */
    void UpdateExportVariables();

    /**
     * @brief Update from export variables
     */
    void UpdateFromExportVariables();

private:
    // Texture properties
    std::string m_texture_path;
    glm::vec4 m_modulate;
    glm::vec2 m_size;
    glm::vec2 m_offset;
    bool m_centered;
    bool m_flip_h;
    bool m_flip_v;

    // Region properties
    bool m_region_enabled;
    glm::vec4 m_region_rect;

    // Rendering properties
    BillboardMode m_billboard_mode;
    AlphaCutMode m_alpha_cut_mode;
    float m_alpha_cut_threshold;
    bool m_transparent;
    bool m_double_sided;
    bool m_receives_lighting;

    // Internal rendering data
    std::shared_ptr<GraphicsTexture> m_texture;
    bool m_texture_loaded;
    std::shared_ptr<GraphicsVertexArray> m_vertex_array;
    std::shared_ptr<GraphicsBuffer> m_vertex_buffer;
    std::shared_ptr<GraphicsBuffer> m_index_buffer;
    bool m_mesh_initialized;

    /**
     * @brief Load texture from file
     */
    void LoadTexture();

    /**
     * @brief Initialize sprite mesh (quad)
     */
    void InitializeMesh();

    /**
     * @brief Update mesh vertices based on current properties
     */
    void UpdateMeshVertices();

    /**
     * @brief Render the sprite
     * @param node3d Node3D for positioning and transformation
     */
    void RenderSprite(Node3D* node3d);

    /**
     * @brief Calculate billboard transformation matrix
     * @param node_transform Node's world transform
     * @return Billboard transform matrix
     */
    glm::mat4 CalculateBillboardTransform(const glm::mat4& node_transform) const;

    /**
     * @brief Get sprite bounds in local coordinates
     * @return Rectangle representing the sprite's boundary
     */
    glm::vec4 GetLocalBounds() const;
};

} // namespace Lupine
