#pragma once

#include "lupine/core/Component.h"
#include "lupine/rendering/GraphicsTexture.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace Lupine {

class Control;
class Node2D;

/**
 * @brief TextureRectangle UI component for displaying textures
 *
 * TextureRectangle component displays textures in UI with proper scaling,
 * positioning, and various stretch modes. It can be attached to Control
 * or Node2D nodes.
 */
class TextureRectangle : public Component {
public:
    /**
     * @brief Texture stretch modes
     */
    enum class StretchMode {
        Stretch,        // Stretch texture to fill the entire rectangle
        Tile,           // Tile texture to fill the rectangle
        KeepAspect,     // Keep aspect ratio, fit within rectangle
        KeepAspectCentered, // Keep aspect ratio, center in rectangle
        KeepAspectCovered   // Keep aspect ratio, cover entire rectangle
    };

    /**
     * @brief Constructor
     */
    TextureRectangle();

    /**
     * @brief Virtual destructor
     */
    virtual ~TextureRectangle() = default;

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
     * @brief Get stretch mode
     * @return Current stretch mode
     */
    StretchMode GetStretchMode() const { return m_stretch_mode; }

    /**
     * @brief Set stretch mode
     * @param mode Stretch mode
     */
    void SetStretchMode(StretchMode mode);

    /**
     * @brief Get whether texture is flipped horizontally
     * @return True if flipped horizontally
     */
    bool IsFlippedH() const { return m_flip_h; }

    /**
     * @brief Set horizontal flip
     * @param flip True to flip horizontally
     */
    void SetFlippedH(bool flip);

    /**
     * @brief Get whether texture is flipped vertically
     * @return True if flipped vertically
     */
    bool IsFlippedV() const { return m_flip_v; }

    /**
     * @brief Set vertical flip
     * @param flip True to flip vertically
     */
    void SetFlippedV(bool flip);

    /**
     * @brief Get texture region (UV coordinates)
     * @return Texture region (x, y, width, height) in normalized coordinates
     */
    const glm::vec4& GetTextureRegion() const { return m_texture_region; }

    /**
     * @brief Set texture region (UV coordinates)
     * @param region Texture region (x, y, width, height) in normalized coordinates
     */
    void SetTextureRegion(const glm::vec4& region);

    /**
     * @brief Get component type name (for serialization)
     * @return Type name string
     */
    std::string GetTypeName() const override { return "TextureRectangle"; }

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
    StretchMode m_stretch_mode;
    bool m_flip_h;
    bool m_flip_v;
    glm::vec4 m_texture_region;
    
    // Texture data
    std::shared_ptr<GraphicsTexture> m_texture;
    bool m_texture_loaded;
    glm::vec2 m_texture_size;

    /**
     * @brief Load texture from file
     */
    void LoadTexture();

    /**
     * @brief Render texture rectangle
     * @param control Control node for positioning
     */
    void RenderTexture(Control* control);

    /**
     * @brief Render texture rectangle for Node2D
     * @param node2d Node2D for positioning
     */
    void RenderTextureNode2D(Node2D* node2d);

    /**
     * @brief Calculate texture transform based on stretch mode
     * @param rect_size Size of the rectangle to fill
     * @return Transform matrix for texture rendering
     */
    glm::mat4 CalculateTextureTransform(const glm::vec2& position, const glm::vec2& rect_size) const;

    /**
     * @brief Convert stretch mode to integer for export variables
     * @param mode Stretch mode
     * @return Integer representation
     */
    int StretchModeToInt(StretchMode mode) const;

    /**
     * @brief Convert integer to stretch mode from export variables
     * @param mode Integer representation
     * @return Stretch mode
     */
    StretchMode IntToStretchMode(int mode) const;
};

} // namespace Lupine
