#pragma once

#include "lupine/core/Component.h"
#include "lupine/resources/MeshLoader.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace Lupine {

/**
 * @brief Static mesh component for rendering complex 3D models
 *
 * StaticMesh component can load and render complex 3D models with multiple meshes,
 * materials, and textures. It's designed for static (non-animated) models.
 */
class StaticMesh : public Component {
public:
    /**
     * @brief Constructor
     */
    StaticMesh();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~StaticMesh() = default;
    
    /**
     * @brief Get mesh file path
     * @return Path to mesh file
     */
    const std::string& GetMeshPath() const { return m_mesh_path; }
    
    /**
     * @brief Set mesh file path
     * @param path Path to mesh file
     */
    void SetMeshPath(const std::string& path);
    
    /**
     * @brief Get color tint
     * @return Color tint
     */
    const glm::vec4& GetColor() const { return m_color; }
    
    /**
     * @brief Set color tint
     * @param color Color tint
     */
    void SetColor(const glm::vec4& color);
    
    /**
     * @brief Get scale
     * @return Scale factor
     */
    const glm::vec3& GetScale() const { return m_scale; }
    
    /**
     * @brief Set scale
     * @param scale Scale factor
     */
    void SetScale(const glm::vec3& scale);
    
    /**
     * @brief Check if model is loaded
     * @return True if model is loaded
     */
    bool IsLoaded() const { return m_model && m_model->IsLoaded(); }
    
    /**
     * @brief Get number of meshes in the model
     * @return Number of meshes
     */
    size_t GetMeshCount() const;

    /**
     * @brief Get casts shadows flag
     * @return True if this mesh casts shadows
     */
    bool GetCastsShadows() const { return m_casts_shadows; }

    /**
     * @brief Set casts shadows flag
     * @param casts_shadows True to enable shadow casting
     */
    void SetCastsShadows(bool casts_shadows);

    /**
     * @brief Get receives shadows flag
     * @return True if this mesh receives shadows
     */
    bool GetReceivesShadows() const { return m_receives_shadows; }

    /**
     * @brief Set receives shadows flag
     * @param receives_shadows True to enable shadow receiving
     */
    void SetReceivesShadows(bool receives_shadows);

    /**
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "StaticMesh"; }

    /**
     * @brief Render mesh for shadow mapping (depth only)
     */
    void RenderForShadows();

    // Component interface
    void OnReady() override;
    void OnUpdate(float delta_time) override;

private:
    std::string m_mesh_path;
    glm::vec4 m_color;
    glm::vec3 m_scale;
    std::unique_ptr<Model> m_model;
    bool m_model_loaded;
    bool m_casts_shadows;
    bool m_receives_shadows;
    
    /**
     * @brief Load the model from file
     */
    void LoadModel();
    
    /**
     * @brief Initialize export variables
     */
    void InitializeExportVariables();
    
    /**
     * @brief Update export variables from internal state
     */
    void UpdateExportVariables();
    
    /**
     * @brief Update internal state from export variables
     */
    void UpdateFromExportVariables();
};

} // namespace Lupine
