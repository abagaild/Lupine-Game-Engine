#pragma once

#include "lupine/core/Component.h"
#include "lupine/resources/MeshLoader.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <vector>

namespace Lupine {

/**
 * @brief Skinned mesh component for rendering animated 3D models
 *
 * SkinnedMesh component renders 3D models with skeletal animation support.
 * It can be attached to Node3D nodes and supports bone-based animation.
 */
class SkinnedMesh : public Component {
public:
    /**
     * @brief Constructor
     */
    SkinnedMesh();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~SkinnedMesh() = default;
    
    /**
     * @brief Get mesh file path
     * @return Mesh file path
     */
    const std::string& GetMeshPath() const { return m_mesh_path; }
    
    /**
     * @brief Set mesh file path
     * @param path Path to mesh file
     */
    void SetMeshPath(const std::string& path);
    
    /**
     * @brief Get mesh color
     * @return Mesh color
     */
    const glm::vec4& GetColor() const { return m_color; }
    
    /**
     * @brief Set mesh color
     * @param color Mesh color
     */
    void SetColor(const glm::vec4& color) { m_color = color; }
    
    /**
     * @brief Check if mesh is loaded
     * @return True if mesh is loaded
     */
    bool IsMeshLoaded() const { return m_mesh_loaded; }
    
    /**
     * @brief Get loaded model
     * @return Pointer to loaded model or nullptr
     */
    const Model* GetModel() const { return m_model.get(); }
    
    /**
     * @brief Get current bone transforms
     * @return Vector of bone transformation matrices
     */
    const std::vector<glm::mat4>& GetBoneTransforms() const { return m_bone_transforms; }
    
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
     * @brief Update bone transforms for current animation state
     * @param animation_time Current animation time
     * @param animation_name Name of animation to play
     */
    void UpdateBoneTransforms(float animation_time, const std::string& animation_name);

    /**
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "SkinnedMesh"; }

    /**
     * @brief Render mesh for shadow mapping (depth only)
     */
    void RenderForShadows();

    /**
     * @brief Component lifecycle methods
     */
    void OnReady() override;
    void OnUpdate(float delta_time) override;
    
    /**
     * @brief Initialize export variables for editor
     */
    void InitializeExportVariables() override;

private:
    std::string m_mesh_path;
    glm::vec4 m_color;
    bool m_mesh_loaded;
    bool m_casts_shadows;
    bool m_receives_shadows;

    // Model data
    std::unique_ptr<Model> m_model;

    // Animation data
    std::vector<glm::mat4> m_bone_transforms;
    std::string m_current_animation;
    float m_animation_time;
    bool m_auto_play;
    
    /**
     * @brief Load mesh from file
     */
    void LoadMesh();
    
    /**
     * @brief Update from export variables
     */
    void UpdateFromExportVariables();
    
    /**
     * @brief Calculate bone transformation recursively
     * @param node_name Node name
     * @param parent_transform Parent transformation matrix
     * @param animation Animation clip
     * @param time Animation time
     */
    void CalculateBoneTransform(const std::string& node_name,
                               const glm::mat4& parent_transform,
                               const SkeletalAnimationClip* animation,
                               float time);

    /**
     * @brief Get node transformation from animation
     * @param node_name Node name
     * @param animation Animation clip
     * @param time Animation time
     * @return Transformation matrix
     */
    glm::mat4 GetNodeTransform(const std::string& node_name,
                              const SkeletalAnimationClip* animation,
                              float time);

    /**
     * @brief Get bone transformation from animation
     * @param bone_name Bone name
     * @param animation Animation clip
     * @param time Animation time
     * @return Transformation matrix
     */
    glm::mat4 GetBoneTransform(const std::string& bone_name,
                              const SkeletalAnimationClip* animation,
                              float time);

    /**
     * @brief Get available animation names
     * @return Vector of animation names
     */
    std::vector<std::string> GetAvailableAnimations() const;

    /**
     * @brief Get current animation name
     * @return Current animation name
     */
    const std::string& GetCurrentAnimation() const { return m_current_animation; }

    /**
     * @brief Set current animation
     * @param animation_name Animation name
     */
    void SetCurrentAnimation(const std::string& animation_name);

    /**
     * @brief Get auto-play setting
     * @return True if auto-play is enabled
     */
    bool GetAutoPlay() const { return m_auto_play; }

    /**
     * @brief Set auto-play setting
     * @param auto_play Auto-play enabled
     */
    void SetAutoPlay(bool auto_play) { m_auto_play = auto_play; }
};

} // namespace Lupine
