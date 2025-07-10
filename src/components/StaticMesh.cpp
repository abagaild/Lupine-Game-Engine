#include "lupine/components/StaticMesh.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Node2D.h"
#include <glad/glad.h>
#include <iostream>

namespace Lupine {

StaticMesh::StaticMesh()
    : Component("StaticMesh")
    , m_mesh_path("")
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_scale(1.0f, 1.0f, 1.0f)
    , m_model(nullptr)
    , m_model_loaded(false)
    , m_casts_shadows(true)
    , m_receives_shadows(true) {

    // Initialize export variables after member variables are set
    InitializeExportVariables();
}

void StaticMesh::SetMeshPath(const std::string& path) {
    m_mesh_path = path;
    SetExportVariable("mesh_path", path);
    m_model_loaded = false;
    LoadModel();
}

void StaticMesh::SetColor(const glm::vec4& color) {
    m_color = color;
    SetExportVariable("color", color);
}

void StaticMesh::SetScale(const glm::vec3& scale) {
    m_scale = scale;
    SetExportVariable("scale", scale);
}

void StaticMesh::SetCastsShadows(bool casts_shadows) {
    m_casts_shadows = casts_shadows;
    SetExportVariable("casts_shadows", m_casts_shadows);
}

void StaticMesh::SetReceivesShadows(bool receives_shadows) {
    m_receives_shadows = receives_shadows;
    SetExportVariable("receives_shadows", m_receives_shadows);
}

size_t StaticMesh::GetMeshCount() const {
    if (m_model && m_model->IsLoaded()) {
        return m_model->GetMeshes().size();
    }
    return 0;
}

void StaticMesh::OnReady() {
    UpdateFromExportVariables();
    LoadModel();
}

void StaticMesh::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();

    // Check rendering context - StaticMesh should only render in 3D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor2D) {
        return; // Don't render 3D meshes in 2D editor view
    }

    // Render all meshes if model is loaded
    if (m_model_loaded && m_model && m_model->IsLoaded()) {
        // Calculate transform matrix
        glm::mat4 transform = glm::mat4(1.0f);
        bool hasValidOwner = false;

        // Try to get transform from Node3D first
        if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
            glm::vec3 pos = node3d->GetGlobalPosition();
            glm::quat rotation = node3d->GetGlobalRotation();
            glm::vec3 scale3d = node3d->GetGlobalScale();

            transform = glm::translate(transform, pos);
            transform = transform * glm::mat4_cast(rotation);
            transform = glm::scale(transform, scale3d);
            hasValidOwner = true;
        }
        // Fallback to Node2D
        else if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
            glm::vec2 pos = node2d->GetGlobalPosition();
            float rotation = node2d->GetGlobalRotation();
            glm::vec2 scale2d = node2d->GetGlobalScale();

            transform = glm::translate(transform, glm::vec3(pos.x, pos.y, 0.0f));
            transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
            transform = glm::scale(transform, glm::vec3(scale2d.x, scale2d.y, 1.0f));
            hasValidOwner = true;
        }

        if (hasValidOwner) {
            try {
                // Apply component scale
                transform = glm::scale(transform, m_scale);

                // Render all meshes in the model
                const auto& meshes = m_model->GetMeshes();

                // Safety check for empty mesh collection
                if (meshes.empty()) {
                    return;
                }

                for (const auto& mesh : meshes) {
                    // Additional safety check for mesh validity
                    if (&mesh == nullptr) {
                        continue;
                    }

                    // Ensure mesh is set up for OpenGL rendering (lazy initialization)
                    if (mesh.VAO == 0) {
                        // Cast away const to allow lazy initialization
                        const_cast<Mesh&>(mesh).SetupMesh();

                        // Verify VAO was created successfully
                        if (mesh.VAO == 0) {
                            std::cerr << "StaticMesh::Render: Failed to create VAO for mesh" << std::endl;
                            continue;
                        }
                    }

                    // Check if mesh has textures
                    if (!mesh.material.diffuse_maps.empty() && mesh.material.diffuse_maps[0].id != 0) {
                        // Render with texture
                        Renderer::RenderMesh(mesh, transform, m_color, mesh.material.diffuse_maps[0].id, m_receives_shadows);
                    } else {
                        // Render without texture
                        Renderer::RenderMesh(mesh, transform, m_color, Renderer::GetWhiteTexture(), m_receives_shadows);
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "StaticMesh::Render: Exception during rendering: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "StaticMesh::Render: Unknown exception during rendering" << std::endl;
            }
        }
    }
}

void StaticMesh::RenderForShadows() {
    // Only render for shadows if model is loaded and shadow casting is enabled
    if (!m_model_loaded || !m_model || !m_model->IsLoaded() || !m_casts_shadows) {
        return;
    }

    // Render all meshes in the model for shadow mapping
    const auto& meshes = m_model->GetMeshes();
    for (const auto& mesh : meshes) {
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

void StaticMesh::LoadModel() {
    if (m_mesh_path.empty()) {
        m_model.reset();
        m_model_loaded = false;
        return;
    }

    try {
        m_model = MeshLoader::LoadModel(m_mesh_path);
        if (m_model && m_model->IsLoaded()) {
            m_model_loaded = true;
            std::cout << "StaticMesh: Loaded model " << m_mesh_path
                      << " (" << m_model->GetMeshes().size() << " meshes)" << std::endl;

            // Check if model has animations and suggest using SkinnedMesh
            if (m_model->HasAnimations()) {
                std::cout << "StaticMesh: WARNING - Model has " << m_model->GetAnimations().size()
                         << " animations. Consider using SkinnedMesh component for animated models." << std::endl;
            }
        } else {
            std::cerr << "StaticMesh: Failed to load model from " << m_mesh_path << std::endl;
            m_model_loaded = false;
        }
    } catch (const std::exception& e) {
        std::cerr << "StaticMesh: Exception loading model " << m_mesh_path << ": " << e.what() << std::endl;
        m_model.reset();
        m_model_loaded = false;
    }
}

void StaticMesh::InitializeExportVariables() {
    AddExportVariable("mesh_path", m_mesh_path, "Path to 3D model file", ExportVariableType::FilePath);
    AddExportVariable("color", m_color, "Color tint (RGBA)", ExportVariableType::Color);
    AddExportVariable("scale", m_scale, "Scale factor (XYZ)", ExportVariableType::Vec3);
    AddExportVariable("casts_shadows", m_casts_shadows, "Enable shadow casting", ExportVariableType::Bool);
    AddExportVariable("receives_shadows", m_receives_shadows, "Enable shadow receiving", ExportVariableType::Bool);
}

void StaticMesh::UpdateExportVariables() {
    SetExportVariable("mesh_path", m_mesh_path);
    SetExportVariable("color", m_color);
    SetExportVariable("scale", m_scale);
    SetExportVariable("casts_shadows", m_casts_shadows);
    SetExportVariable("receives_shadows", m_receives_shadows);
}

void StaticMesh::UpdateFromExportVariables() {
    std::string new_mesh_path = GetExportVariableValue<std::string>("mesh_path", "");
    if (new_mesh_path != m_mesh_path) {
        m_mesh_path = new_mesh_path;
        m_model_loaded = false;
        LoadModel();
    }

    m_color = GetExportVariableValue<glm::vec4>("color", glm::vec4(1.0f));
    m_scale = GetExportVariableValue<glm::vec3>("scale", glm::vec3(1.0f));
    m_casts_shadows = GetExportVariableValue<bool>("casts_shadows", true);
    m_receives_shadows = GetExportVariableValue<bool>("receives_shadows", true);
}

} // namespace Lupine
