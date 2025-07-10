#include "lupine/components/SkinnedMesh.h"
#include "lupine/components/AnimationPlayer.h"
#include "lupine/core/Component.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/resources/MeshLoader.h"
#include <glad/glad.h>
#include <iostream>

namespace Lupine {

SkinnedMesh::SkinnedMesh()
    : Component("SkinnedMesh")
    , m_mesh_path("")
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_mesh_loaded(false)
    , m_casts_shadows(true)
    , m_receives_shadows(true)
    , m_current_animation("")
    , m_animation_time(0.0f)
    , m_auto_play(false) {

    // Initialize export variables after member variables are set
    InitializeExportVariables();
}

void SkinnedMesh::SetMeshPath(const std::string& path) {
    if (m_mesh_path != path) {
        m_mesh_path = path;
        m_mesh_loaded = false;
        m_model.reset();
        LoadMesh();
    }
}

void SkinnedMesh::SetCastsShadows(bool casts_shadows) {
    m_casts_shadows = casts_shadows;
}

void SkinnedMesh::SetReceivesShadows(bool receives_shadows) {
    m_receives_shadows = receives_shadows;
}

void SkinnedMesh::OnReady() {
    UpdateFromExportVariables();
    LoadMesh();
}

void SkinnedMesh::OnUpdate(float delta_time) {
    (void)delta_time; // Suppress unused parameter warning

    // Check if export variables have changed
    UpdateFromExportVariables();

    // Check rendering context - SkinnedMesh should only render in 3D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor2D) {
        return; // Don't render 3D meshes in 2D editor view
    }

    // TODO: Animation system not implemented yet
    // For now, SkinnedMesh works like StaticMesh without animation

    // Render the mesh if loaded
    if (m_mesh_loaded && m_model) {
        glm::mat4 transform = glm::mat4(1.0f);
        bool hasValidOwner = false;

        if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
            // Use Node3D transform properties
            glm::vec3 pos = node3d->GetGlobalPosition();
            transform = glm::translate(transform, pos);

            glm::quat rotation = node3d->GetGlobalRotation();
            transform = transform * glm::mat4_cast(rotation);

            glm::vec3 scale = node3d->GetGlobalScale();
            transform = glm::scale(transform, scale);
            hasValidOwner = true;
        } else if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
            // Use Node2D transform properties (for 2D-in-3D rendering)
            glm::vec2 pos = node2d->GetGlobalPosition();
            transform = glm::translate(transform, glm::vec3(pos.x, pos.y, 0.0f));

            float rotation = node2d->GetGlobalRotation();
            transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));

            glm::vec2 scale = node2d->GetGlobalScale();
            transform = glm::scale(transform, glm::vec3(scale.x, scale.y, 1.0f));
            hasValidOwner = true;
        }

        if (hasValidOwner) {
            // Update bone transforms if we have animations
            if (m_model->HasAnimations() && !m_current_animation.empty()) {
                // Get animation player if available
                if (auto* anim_player = GetOwner()->GetComponent<AnimationPlayer>()) {
                    float current_time = anim_player->GetCurrentTime();
                    UpdateBoneTransforms(current_time, m_current_animation);
                } else {
                    // Use default animation time if no animation player
                    UpdateBoneTransforms(m_animation_time, m_current_animation);
                }
            }

            // Render each mesh in the model
            for (const auto& mesh : m_model->GetMeshes()) {
                // Ensure mesh is set up for OpenGL rendering (lazy initialization)
                if (mesh.VAO == 0) {
                    // Cast away const to allow lazy initialization
                    const_cast<Mesh&>(mesh).SetupMesh();
                }

                // Create a temporary mesh structure for rendering
                Mesh temp_mesh;
                temp_mesh.VAO = mesh.VAO;
                temp_mesh.vertices = mesh.vertices; // Copy for size info
                temp_mesh.indices = mesh.indices;   // Copy for size info

                // Use skinned mesh rendering if we have bone transforms
                if (m_model->HasAnimations() && !m_bone_transforms.empty()) {
                    Renderer::RenderSkinnedMesh(temp_mesh, transform, m_color,
                                              Renderer::GetWhiteTexture(), m_bone_transforms, m_receives_shadows);
                } else {
                    // Fall back to regular mesh rendering
                    Renderer::RenderMesh(temp_mesh, transform, m_color, Renderer::GetWhiteTexture(), m_receives_shadows);
                }
            }
        }
    }
}

void SkinnedMesh::InitializeExportVariables() {
    // Mesh file path
    AddExportVariable("mesh_path", m_mesh_path, "Path to mesh file", ExportVariableType::FilePath);

    // Color
    AddExportVariable("color", m_color, "Mesh color", ExportVariableType::Color);

    // Shadow properties
    AddExportVariable("casts_shadows", m_casts_shadows, "Enable shadow casting", ExportVariableType::Bool);
    AddExportVariable("receives_shadows", m_receives_shadows, "Enable shadow receiving", ExportVariableType::Bool);

    // Animation settings
    AddExportVariable("current_animation", m_current_animation, "Current animation name", ExportVariableType::String);
    AddExportVariable("auto_play", m_auto_play, "Auto-play animation", ExportVariableType::Bool);
}

void SkinnedMesh::LoadMesh() {
    if (m_mesh_path.empty()) {
        return;
    }

    try {
        m_model = MeshLoader::LoadModel(m_mesh_path);
        if (m_model && m_model->IsLoaded()) {
            m_mesh_loaded = true;

            // Check if model has animations
            if (m_model->HasAnimations()) {
                const auto& animations = m_model->GetAnimations();
                std::cout << "SkinnedMesh: Model has " << animations.size() << " animations" << std::endl;

                // Auto-create AnimationPlayer component if not present
                if (!GetOwner()->GetComponent<AnimationPlayer>()) {
                    auto* anim_player = GetOwner()->AddComponent<AnimationPlayer>();
                    if (anim_player) {
                        std::cout << "SkinnedMesh: Auto-created AnimationPlayer component" << std::endl;

                        // Set up auto-play if enabled and animation is specified
                        if (m_auto_play && !m_current_animation.empty()) {
                            anim_player->SetAutoPlay(true, m_current_animation);
                        } else if (m_auto_play && !animations.empty()) {
                            // Use first animation as default
                            anim_player->SetAutoPlay(true, animations[0].name);
                            m_current_animation = animations[0].name;
                        }
                    }
                }
            }

            std::cout << "SkinnedMesh: Successfully loaded mesh from " << m_mesh_path << std::endl;
        } else {
            std::cerr << "SkinnedMesh: Failed to load mesh from " << m_mesh_path << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "SkinnedMesh: Exception loading mesh: " << e.what() << std::endl;
    }
}

void SkinnedMesh::UpdateFromExportVariables() {
    // Check if mesh path changed
    std::string mesh_path_var = GetExportVariableValue<std::string>("mesh_path", m_mesh_path);
    if (mesh_path_var != m_mesh_path) {
        SetMeshPath(mesh_path_var);
    }

    // Update color
    m_color = GetExportVariableValue<glm::vec4>("color", m_color);

    // Update shadow properties
    m_casts_shadows = GetExportVariableValue<bool>("casts_shadows", m_casts_shadows);
    m_receives_shadows = GetExportVariableValue<bool>("receives_shadows", m_receives_shadows);

    // Update animation settings
    m_current_animation = GetExportVariableValue<std::string>("current_animation", m_current_animation);
    m_auto_play = GetExportVariableValue<bool>("auto_play", m_auto_play);
}

void SkinnedMesh::UpdateBoneTransforms(float animation_time, const std::string& animation_name) {
    // Validate input parameters
    if (!m_model || !m_model->HasAnimations()) {
        return;
    }

    if (animation_name.empty()) {
        std::cerr << "SkinnedMesh::UpdateBoneTransforms: Empty animation name" << std::endl;
        return;
    }

    if (!std::isfinite(animation_time)) {
        std::cerr << "SkinnedMesh::UpdateBoneTransforms: Invalid animation time: " << animation_time << std::endl;
        return;
    }

    const SkeletalAnimationClip* animation = m_model->GetAnimation(animation_name);
    if (!animation) {
        std::cerr << "SkinnedMesh::UpdateBoneTransforms: Animation '" << animation_name << "' not found" << std::endl;
        return;
    }

    const Skeleton& skeleton = m_model->GetSkeleton();

    // Validate skeleton has bones
    if (skeleton.bones.empty()) {
        std::cerr << "SkinnedMesh::UpdateBoneTransforms: Skeleton has no bones" << std::endl;
        return;
    }

    // Resize bone transforms array if needed
    if (m_bone_transforms.size() != skeleton.bones.size()) {
        m_bone_transforms.resize(skeleton.bones.size(), glm::mat4(1.0f));
    }

    // Calculate bone transformations
    for (const auto& bone : skeleton.bones) {
        // Validate bone ID
        if (bone.id < 0 || static_cast<size_t>(bone.id) >= m_bone_transforms.size()) {
            std::cerr << "SkinnedMesh::UpdateBoneTransforms: Invalid bone ID " << bone.id
                     << " for bone '" << bone.name << "'" << std::endl;
            continue;
        }

        try {
            glm::mat4 bone_transform = GetBoneTransform(bone.name, animation, animation_time);

            // Validate transformation matrices
            bool valid_transform = true;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    if (!std::isfinite(bone_transform[i][j]) ||
                        !std::isfinite(skeleton.global_inverse_transform[i][j]) ||
                        !std::isfinite(bone.offset_matrix[i][j])) {
                        valid_transform = false;
                        break;
                    }
                }
                if (!valid_transform) break;
            }

            if (valid_transform) {
                m_bone_transforms[bone.id] = skeleton.global_inverse_transform * bone_transform * bone.offset_matrix;
            } else {
                std::cerr << "SkinnedMesh::UpdateBoneTransforms: Invalid transformation matrices for bone '"
                         << bone.name << "'" << std::endl;
                m_bone_transforms[bone.id] = glm::mat4(1.0f); // Use identity matrix as fallback
            }
        } catch (const std::exception& e) {
            std::cerr << "SkinnedMesh::UpdateBoneTransforms: Exception processing bone '"
                     << bone.name << "': " << e.what() << std::endl;
            m_bone_transforms[bone.id] = glm::mat4(1.0f); // Use identity matrix as fallback
        }
    }
}

glm::mat4 SkinnedMesh::GetNodeTransform(const std::string& node_name,
                                       const SkeletalAnimationClip* animation,
                                       float time) {
    if (!animation) {
        return glm::mat4(1.0f);
    }

    // Find bone animation for this node
    for (const auto& bone_anim : animation->bone_animations) {
        if (bone_anim.bone_name == node_name) {
            return bone_anim.GetTransformAtTime(time);
        }
    }

    return glm::mat4(1.0f);
}

glm::mat4 SkinnedMesh::GetBoneTransform(const std::string& bone_name,
                                       const SkeletalAnimationClip* animation,
                                       float time) {
    return GetNodeTransform(bone_name, animation, time);
}

void SkinnedMesh::CalculateBoneTransform(const std::string& node_name,
                                        const glm::mat4& parent_transform,
                                        const SkeletalAnimationClip* animation,
                                        float time) {
    // This method is kept for compatibility but is no longer used
    // The new UpdateBoneTransforms method handles bone calculations directly
    (void)node_name;
    (void)parent_transform;
    (void)animation;
    (void)time;
}

std::vector<std::string> SkinnedMesh::GetAvailableAnimations() const {
    std::vector<std::string> animation_names;

    if (m_model && m_model->HasAnimations()) {
        const auto& animations = m_model->GetAnimations();
        for (const auto& anim : animations) {
            animation_names.push_back(anim.name);
        }
    }

    return animation_names;
}

void SkinnedMesh::SetCurrentAnimation(const std::string& animation_name) {
    if (m_current_animation != animation_name) {
        m_current_animation = animation_name;

        // Update AnimationPlayer if present
        if (auto* anim_player = GetOwner()->GetComponent<AnimationPlayer>()) {
            if (!animation_name.empty()) {
                anim_player->Play(animation_name);
            } else {
                anim_player->Stop();
            }
        }
    }
}

void SkinnedMesh::RenderForShadows() {
    // Only render for shadows if model is loaded and shadow casting is enabled
    if (!m_mesh_loaded || !m_model || !m_casts_shadows) {
        return;
    }

    // Render all meshes in the model for shadow mapping
    const auto& meshes = m_model->GetMeshes();

    for (size_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = meshes[i];

        // Ensure mesh is set up for OpenGL rendering (lazy initialization)
        if (mesh.VAO == 0) {
            // Cast away const to allow lazy initialization
            const_cast<Mesh&>(mesh).SetupMesh();
        }

        // Skip empty meshes
        if (mesh.vertices.empty() && mesh.indices.empty()) {
            continue;
        }

        // Bind vertex array and render for shadow mapping
        glBindVertexArray(mesh.VAO);

        if (!mesh.indices.empty()) {
            // Render with indices
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
        } else if (!mesh.vertices.empty()) {
            // Render without indices
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mesh.vertices.size()));
        }

        glBindVertexArray(0);
    }
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(SkinnedMesh, "3D", "Animated 3D mesh component with skeletal animation support")
