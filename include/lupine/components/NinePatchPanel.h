#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>

namespace Lupine {

class Control;
class Node2D;

/**
 * @brief NinePatchPanel UI component for scalable UI panels with border preservation
 *
 * NinePatchPanel component displays textures using nine-patch scaling,
 * which preserves border details while scaling the center and edges.
 * This is commonly used for UI panels, buttons, and frames that need
 * to scale without distorting border artwork.
 */
class NinePatchPanel : public Component {
public:
    /**
     * @brief Constructor
     */
    NinePatchPanel();

    /**
     * @brief Virtual destructor
     */
    virtual ~NinePatchPanel() = default;

    /**
     * @brief Get texture path
     * @return Texture file path
     */
    const std::string& GetTexturePath() const { return m_texture_path; }

    /**
     * @brief Set texture path
     * @param path Texture file path
     */
    void SetTexturePath(const std::string& path);

    /**
     * @brief Get texture color modulation
     * @return Color modulation (RGBA)
     */
    const glm::vec4& GetModulateColor() const { return m_modulate_color; }

    /**
     * @brief Set texture color modulation
     * @param color Color modulation (RGBA)
     */
    void SetModulateColor(const glm::vec4& color);

    /**
     * @brief Get left border size
     * @return Left border size in pixels
     */
    float GetLeftBorder() const { return m_left_border; }

    /**
     * @brief Set left border size
     * @param size Left border size in pixels
     */
    void SetLeftBorder(float size);

    /**
     * @brief Get right border size
     * @return Right border size in pixels
     */
    float GetRightBorder() const { return m_right_border; }

    /**
     * @brief Set right border size
     * @param size Right border size in pixels
     */
    void SetRightBorder(float size);

    /**
     * @brief Get top border size
     * @return Top border size in pixels
     */
    float GetTopBorder() const { return m_top_border; }

    /**
     * @brief Set top border size
     * @param size Top border size in pixels
     */
    void SetTopBorder(float size);

    /**
     * @brief Get bottom border size
     * @return Bottom border size in pixels
     */
    float GetBottomBorder() const { return m_bottom_border; }

    /**
     * @brief Set bottom border size
     * @param size Bottom border size in pixels
     */
    void SetBottomBorder(float size);

    /**
     * @brief Set all border sizes at once
     * @param left Left border size
     * @param top Top border size
     * @param right Right border size
     * @param bottom Bottom border size
     */
    void SetBorders(float left, float top, float right, float bottom);

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "NinePatchPanel"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "UI"; }

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
    std::string m_texture_path;
    glm::vec4 m_modulate_color;
    float m_left_border;
    float m_right_border;
    float m_top_border;
    float m_bottom_border;
    
    // Texture data
    unsigned int m_texture_id;
    bool m_texture_loaded;
    glm::vec2 m_texture_size;

    /**
     * @brief Load texture from file
     */
    void LoadTexture();

    /**
     * @brief Render nine-patch panel
     * @param control Control node for positioning
     */
    void RenderNinePatch(Control* control);

    /**
     * @brief Render nine-patch panel for Node2D
     * @param node2d Node2D for positioning
     */
    void RenderNinePatchNode2D(Node2D* node2d);

    /**
     * @brief Render a single patch of the nine-patch
     * @param position Patch position
     * @param size Patch size
     * @param uv_min UV minimum coordinates
     * @param uv_max UV maximum coordinates
     */
    void RenderPatch(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec2& uv_min, const glm::vec2& uv_max);
};

} // namespace Lupine
