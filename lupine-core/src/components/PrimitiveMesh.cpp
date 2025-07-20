#include "lupine/components/PrimitiveMesh.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/resources/MeshLoader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lupine {

PrimitiveMesh::PrimitiveMesh()
    : Component("PrimitiveMesh")
    , m_mesh_type(MeshType::Cube)
    , m_size(1.0f, 1.0f, 1.0f)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_subdivisions(16)
    , m_wireframe(false)
    , m_double_sided(false)
    , m_casts_shadows(true)
    , m_receives_shadows(true)
    , m_mesh_path("")
    , m_vertex_array_object(0)
    , m_vertex_buffer_object(0)
    , m_element_buffer_object(0)
    , m_vertex_count(0)
    , m_index_count(0)
    , m_mesh_generated(false)
    , m_has_textures(false) {

    // Initialize export variables
    InitializeExportVariables();
}

void PrimitiveMesh::SetMeshType(MeshType type) {
    m_mesh_type = type;
    SetExportVariable("mesh_type", MeshTypeToInt(type));
    m_mesh_generated = false;
    GenerateMesh();
}

void PrimitiveMesh::SetSize(const glm::vec3& size) {
    m_size = size;
    SetExportVariable("size", size);
    m_mesh_generated = false;
    GenerateMesh();
}

void PrimitiveMesh::SetColor(const glm::vec4& color) {
    m_color = color;
    SetExportVariable("color", color);
}

void PrimitiveMesh::SetSubdivisions(int subdivisions) {
    m_subdivisions = std::max(3, subdivisions); // Minimum 3 subdivisions
    SetExportVariable("subdivisions", m_subdivisions);
    m_mesh_generated = false;
    GenerateMesh();
}

void PrimitiveMesh::SetWireframe(bool wireframe) {
    m_wireframe = wireframe;
    SetExportVariable("wireframe", wireframe);
}

void PrimitiveMesh::SetDoubleSided(bool double_sided) {
    m_double_sided = double_sided;
    SetExportVariable("double_sided", double_sided);
}

void PrimitiveMesh::SetCastsShadows(bool casts_shadows) {
    m_casts_shadows = casts_shadows;
    SetExportVariable("casts_shadows", casts_shadows);
}

void PrimitiveMesh::SetReceivesShadows(bool receives_shadows) {
    m_receives_shadows = receives_shadows;
    SetExportVariable("receives_shadows", receives_shadows);
}

void PrimitiveMesh::SetMeshPath(const std::string& path) {
    m_mesh_path = path;
    SetExportVariable("mesh_path", path);
    m_mesh_generated = false;
    GenerateMesh();
}

void PrimitiveMesh::OnReady() {
    UpdateFromExportVariables();
    GenerateMesh();
}

void PrimitiveMesh::OnUpdate(float delta_time) {
    (void)delta_time;

    // Check if export variables have changed
    UpdateFromExportVariables();

    // Check rendering context - PrimitiveMesh should only render in 3D view or runtime
    RenderingContext context = Renderer::GetRenderingContext();
    if (context == RenderingContext::Editor2D) {
        return; // Don't render 3D meshes in 2D editor view
    }

    // Render the mesh if we have a Node3D or Node2D owner and mesh is generated
    if (m_mesh_generated && m_vertex_array_object != 0) {
        glm::mat4 transform = glm::mat4(1.0f);
        bool hasValidOwner = false;

        if (auto* node3d = dynamic_cast<Node3D*>(GetOwner())) {
            // Get transform from Node3D
            transform = node3d->GetGlobalTransform();
            hasValidOwner = true;
        } else if (auto* node2d = dynamic_cast<Node2D*>(GetOwner())) {
            // Get transform from Node2D and convert to 3D
            glm::vec2 pos = node2d->GetGlobalPosition();
            float rotation = node2d->GetGlobalRotation();
            glm::vec2 scale2d = node2d->GetGlobalScale();

            transform = glm::translate(transform, glm::vec3(pos.x, pos.y, 0.0f));
            transform = glm::rotate(transform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
            transform = glm::scale(transform, glm::vec3(scale2d.x, scale2d.y, 1.0f));
            hasValidOwner = true;
        }

        if (hasValidOwner) {
            // Apply component size scaling
            transform = glm::scale(transform, m_size);

            // Create a temporary mesh structure for rendering
            Mesh temp_mesh;
            temp_mesh.VAO = m_vertex_array_object;
            temp_mesh.vertices.resize(m_vertex_count); // Just for size info
            temp_mesh.indices.resize(m_index_count);   // Just for size info

            // Render the mesh with texture if available
            // TODO: Convert texture IDs to GraphicsTexture - for now use white texture
            Renderer::RenderMesh(temp_mesh, transform, m_color, Renderer::GetWhiteTexture(), m_receives_shadows);
        }
    }
}

void PrimitiveMesh::InitializeExportVariables() {
    // Create enum options for mesh type
    std::vector<std::string> meshTypeOptions = {
        "Cube", "Sphere", "Cylinder", "Plane", "Cone", "External"
    };

    AddEnumExportVariable("mesh_type", MeshTypeToInt(m_mesh_type), "Mesh type", meshTypeOptions);
    AddExportVariable("size", m_size, "Mesh size (width, height, depth)", ExportVariableType::Vec3);
    AddExportVariable("color", m_color, "Mesh color (RGBA)", ExportVariableType::Color);
    AddExportVariable("subdivisions", m_subdivisions, "Subdivision level for curved surfaces", ExportVariableType::Int);
    AddExportVariable("wireframe", m_wireframe, "Enable wireframe rendering", ExportVariableType::Bool);
    AddExportVariable("double_sided", m_double_sided, "Enable double-sided rendering", ExportVariableType::Bool);
    AddExportVariable("casts_shadows", m_casts_shadows, "Enable shadow casting", ExportVariableType::Bool);
    AddExportVariable("receives_shadows", m_receives_shadows, "Enable shadow receiving", ExportVariableType::Bool);
    AddExportVariable("mesh_path", m_mesh_path, "Path to external mesh file (for External mesh type)", ExportVariableType::FilePath);
}

void PrimitiveMesh::GenerateMesh() {
    if (m_mesh_generated) {
        return;
    }
    
    switch (m_mesh_type) {
        case MeshType::Cube:
            GenerateCube();
            break;
        case MeshType::Sphere:
            GenerateSphere();
            break;
        case MeshType::Cylinder:
            GenerateCylinder();
            break;
        case MeshType::Plane:
            GeneratePlane();
            break;
        case MeshType::Cone:
            GenerateCone();
            break;
        case MeshType::External:
            LoadExternalMesh();
            break;
    }
    
    m_mesh_generated = true;
}

void PrimitiveMesh::GenerateCube() {
    // Cube vertices (position, normal, texcoord)
    std::vector<float> vertices = {
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,

        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

        // Left face
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

        // Right face
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,

        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f
    };

    // Cube indices
    std::vector<unsigned int> indices = {
        0,  1,  2,   2,  3,  0,   // Front face
        4,  5,  6,   6,  7,  4,   // Back face
        8,  9,  10,  10, 11, 8,   // Left face
        12, 13, 14,  14, 15, 12,  // Right face
        16, 17, 18,  18, 19, 16,  // Bottom face
        20, 21, 22,  22, 23, 20   // Top face
    };

    CreateMeshBuffers(vertices, indices);
    m_vertex_count = 24;
    m_index_count = 36;
}

void PrimitiveMesh::GenerateSphere() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    int rings = m_subdivisions;
    int sectors = m_subdivisions * 2;
    float radius = 0.5f;

    // Generate vertices
    for (int r = 0; r <= rings; ++r) {
        float phi = M_PI * r / rings; // 0 to PI
        float y = radius * cos(phi);
        float ringRadius = radius * sin(phi);

        for (int s = 0; s <= sectors; ++s) {
            float theta = 2.0f * M_PI * s / sectors; // 0 to 2*PI
            float x = ringRadius * cos(theta);
            float z = ringRadius * sin(theta);

            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Normal (same as position for unit sphere)
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);

            // Texture coordinates
            vertices.push_back((float)s / sectors);
            vertices.push_back((float)r / rings);
        }
    }

    // Generate indices
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            int current = r * (sectors + 1) + s;
            int next = current + sectors + 1;

            // First triangle
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            // Second triangle
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }

    CreateMeshBuffers(vertices, indices);
    m_vertex_count = (rings + 1) * (sectors + 1);
    m_index_count = rings * sectors * 6;
}

void PrimitiveMesh::GenerateCylinder() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    int sides = m_subdivisions;
    float radius = 0.5f;
    float height = 1.0f;
    float halfHeight = height * 0.5f;

    // Generate side vertices
    for (int i = 0; i <= sides; ++i) {
        float angle = 2.0f * M_PI * i / sides;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        // Bottom vertex
        vertices.insert(vertices.end(), {x, -halfHeight, z, x/radius, 0.0f, z/radius, (float)i/sides, 0.0f});

        // Top vertex
        vertices.insert(vertices.end(), {x, halfHeight, z, x/radius, 0.0f, z/radius, (float)i/sides, 1.0f});
    }

    // Generate side indices
    for (int i = 0; i < sides; ++i) {
        int bottom1 = i * 2;
        int top1 = i * 2 + 1;
        int bottom2 = (i + 1) * 2;
        int top2 = (i + 1) * 2 + 1;

        // First triangle
        indices.insert(indices.end(), {static_cast<unsigned int>(bottom1), static_cast<unsigned int>(bottom2), static_cast<unsigned int>(top1)});
        // Second triangle
        indices.insert(indices.end(), {static_cast<unsigned int>(top1), static_cast<unsigned int>(bottom2), static_cast<unsigned int>(top2)});
    }

    // Add center vertices for caps
    int centerBottom = vertices.size() / 8;
    vertices.insert(vertices.end(), {0.0f, -halfHeight, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 0.5f});

    int centerTop = vertices.size() / 8;
    vertices.insert(vertices.end(), {0.0f, halfHeight, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f});

    // Generate cap vertices and indices
    for (int i = 0; i < sides; ++i) {
        float angle = 2.0f * M_PI * i / sides;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        // Bottom cap vertex
        int bottomCapVertex = vertices.size() / 8;
        vertices.insert(vertices.end(), {x, -halfHeight, z, 0.0f, -1.0f, 0.0f,
                                       0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)});

        // Top cap vertex
        int topCapVertex = vertices.size() / 8;
        vertices.insert(vertices.end(), {x, halfHeight, z, 0.0f, 1.0f, 0.0f,
                                       0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)});

        // Bottom cap triangle
        int nextBottomCap = centerBottom + 2 + ((i + 1) % sides) * 2;
        indices.insert(indices.end(), {static_cast<unsigned int>(centerBottom), static_cast<unsigned int>(nextBottomCap), static_cast<unsigned int>(bottomCapVertex)});

        // Top cap triangle
        int nextTopCap = centerTop + 2 + ((i + 1) % sides) * 2 + 1;
        indices.insert(indices.end(), {static_cast<unsigned int>(centerTop), static_cast<unsigned int>(topCapVertex), static_cast<unsigned int>(nextTopCap)});
    }

    CreateMeshBuffers(vertices, indices);
    m_vertex_count = vertices.size() / 8;
    m_index_count = indices.size();
}

void PrimitiveMesh::GeneratePlane() {
    // Plane vertices (position, normal, texcoord)
    std::vector<float> vertices = {
        // Plane (XY plane, facing +Z)
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f
    };

    // Plane indices
    std::vector<unsigned int> indices = {
        0, 1, 2,  2, 3, 0
    };

    CreateMeshBuffers(vertices, indices);
    m_vertex_count = 4;
    m_index_count = 6;
}

void PrimitiveMesh::GenerateCone() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    int sides = m_subdivisions;
    float radius = 0.5f;
    float height = 1.0f;
    float halfHeight = height * 0.5f;

    // Apex vertex
    vertices.insert(vertices.end(), {0.0f, halfHeight, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f});

    // Base center vertex
    vertices.insert(vertices.end(), {0.0f, -halfHeight, 0.0f, 0.0f, -1.0f, 0.0f, 0.5f, 0.5f});

    // Generate base vertices
    for (int i = 0; i < sides; ++i) {
        float angle = 2.0f * M_PI * i / sides;
        float x = radius * cos(angle);
        float z = radius * sin(angle);

        // Calculate normal for side face
        glm::vec3 sideNormal = glm::normalize(glm::vec3(x, radius, z));

        // Side vertex (for side faces)
        vertices.insert(vertices.end(), {x, -halfHeight, z, sideNormal.x, sideNormal.y, sideNormal.z,
                                       (float)i / sides, 0.0f});

        // Base vertex (for base face)
        vertices.insert(vertices.end(), {x, -halfHeight, z, 0.0f, -1.0f, 0.0f,
                                       0.5f + 0.5f * cos(angle), 0.5f + 0.5f * sin(angle)});
    }

    // Generate side triangles
    for (int i = 0; i < sides; ++i) {
        int current = 2 + i * 2; // Side vertex
        int next = 2 + ((i + 1) % sides) * 2; // Next side vertex

        indices.insert(indices.end(), {0u, static_cast<unsigned int>(next), static_cast<unsigned int>(current)}); // Apex to base edge
    }

    // Generate base triangles
    for (int i = 0; i < sides; ++i) {
        int current = 2 + i * 2 + 1; // Base vertex
        int next = 2 + ((i + 1) % sides) * 2 + 1; // Next base vertex

        indices.insert(indices.end(), {1u, static_cast<unsigned int>(current), static_cast<unsigned int>(next)}); // Base center to edge
    }

    CreateMeshBuffers(vertices, indices);
    m_vertex_count = vertices.size() / 8;
    m_index_count = indices.size();
}

void PrimitiveMesh::LoadExternalMesh() {
    if (m_mesh_path.empty()) {
        std::cerr << "PrimitiveMesh: No mesh path specified for External mesh type" << std::endl;
        // For External mesh type, we need a valid path - don't generate anything
        return;
    }

    // Load mesh using MeshLoader
    auto model = MeshLoader::LoadModel(m_mesh_path);
    if (!model || model->GetMeshes().empty()) {
        std::cerr << "PrimitiveMesh: Failed to load mesh from " << m_mesh_path << std::endl;
        return;
    }

    // Use the first mesh from the model
    const auto& mesh = model->GetMeshes()[0];

    // Convert mesh data to our format (position, normal, texcoord)
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Convert vertices
    for (const auto& vertex : mesh.vertices) {
        // Position
        vertices.push_back(vertex.position.x);
        vertices.push_back(vertex.position.y);
        vertices.push_back(vertex.position.z);

        // Normal
        vertices.push_back(vertex.normal.x);
        vertices.push_back(vertex.normal.y);
        vertices.push_back(vertex.normal.z);

        // Texture coordinates
        vertices.push_back(vertex.tex_coords.x);
        vertices.push_back(vertex.tex_coords.y);
    }

    // Copy indices
    indices = mesh.indices;

    // Extract texture information
    m_texture_ids.clear();
    m_has_textures = false;

    // Collect all texture IDs from the material
    for (const auto& texture : mesh.material.diffuse_maps) {
        if (texture.id != 0) {
            m_texture_ids.push_back(texture.id);
            m_has_textures = true;
        }
    }
    for (const auto& texture : mesh.material.specular_maps) {
        if (texture.id != 0) {
            m_texture_ids.push_back(texture.id);
            m_has_textures = true;
        }
    }
    for (const auto& texture : mesh.material.normal_maps) {
        if (texture.id != 0) {
            m_texture_ids.push_back(texture.id);
            m_has_textures = true;
        }
    }

    CreateMeshBuffers(vertices, indices);
    m_vertex_count = mesh.vertices.size();
    m_index_count = mesh.indices.size();

    std::cout << "PrimitiveMesh: Loaded external mesh " << m_mesh_path
              << " (" << m_vertex_count << " vertices, " << m_index_count << " indices";
    if (m_has_textures) {
        std::cout << ", " << m_texture_ids.size() << " textures";
    }
    std::cout << ")" << std::endl;
}

void PrimitiveMesh::UpdateExportVariables() {
    SetExportVariable("mesh_type", MeshTypeToInt(m_mesh_type));
    SetExportVariable("size", m_size);
    SetExportVariable("color", m_color);
    SetExportVariable("subdivisions", m_subdivisions);
    SetExportVariable("wireframe", m_wireframe);
    SetExportVariable("double_sided", m_double_sided);
    SetExportVariable("casts_shadows", m_casts_shadows);
    SetExportVariable("receives_shadows", m_receives_shadows);
    SetExportVariable("mesh_path", m_mesh_path);
}

void PrimitiveMesh::UpdateFromExportVariables() {
    MeshType new_mesh_type = IntToMeshType(GetExportVariableValue<int>("mesh_type", MeshTypeToInt(m_mesh_type)));
    glm::vec3 new_size = GetExportVariableValue<glm::vec3>("size", m_size);
    int new_subdivisions = GetExportVariableValue<int>("subdivisions", m_subdivisions);
    std::string new_mesh_path = GetExportVariableValue<std::string>("mesh_path", m_mesh_path);

    bool needs_regeneration = false;

    if (new_mesh_type != m_mesh_type) {
        m_mesh_type = new_mesh_type;
        needs_regeneration = true;
    }

    if (new_size != m_size) {
        m_size = new_size;
        needs_regeneration = true;
    }

    if (new_subdivisions != m_subdivisions) {
        m_subdivisions = std::max(3, new_subdivisions);
        needs_regeneration = true;
    }

    if (new_mesh_path != m_mesh_path) {
        m_mesh_path = new_mesh_path;
        // Only regenerate if we're using External mesh type
        if (m_mesh_type == MeshType::External) {
            needs_regeneration = true;
        }
    }

    if (needs_regeneration) {
        m_mesh_generated = false;
        GenerateMesh();
    }

    m_color = GetExportVariableValue<glm::vec4>("color", m_color);
    m_wireframe = GetExportVariableValue<bool>("wireframe", m_wireframe);
    m_double_sided = GetExportVariableValue<bool>("double_sided", m_double_sided);
    m_casts_shadows = GetExportVariableValue<bool>("casts_shadows", m_casts_shadows);
    m_receives_shadows = GetExportVariableValue<bool>("receives_shadows", m_receives_shadows);
}

int PrimitiveMesh::MeshTypeToInt(MeshType type) const {
    return static_cast<int>(type);
}

PrimitiveMesh::MeshType PrimitiveMesh::IntToMeshType(int type) const {
    if (type >= 0 && type <= 5) {
        return static_cast<MeshType>(type);
    }
    return MeshType::Cube;
}

void PrimitiveMesh::CreateMeshBuffers(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    // Delete existing buffers if they exist
    if (m_vertex_array_object != 0) {
        glDeleteVertexArrays(1, &m_vertex_array_object);
        glDeleteBuffers(1, &m_vertex_buffer_object);
        glDeleteBuffers(1, &m_element_buffer_object);
    }

    // Generate buffers
    glGenVertexArrays(1, &m_vertex_array_object);
    glGenBuffers(1, &m_vertex_buffer_object);
    glGenBuffers(1, &m_element_buffer_object);

    // Bind VAO
    glBindVertexArray(m_vertex_array_object);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_element_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Set vertex attributes
    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Texture coordinates (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Unbind VAO
    glBindVertexArray(0);
}

void PrimitiveMesh::RenderForShadows() {
    if (!m_mesh_generated || m_vertex_array_object == 0 || !m_casts_shadows) {
        return;
    }

    // Bind vertex array and render for shadow mapping
    glBindVertexArray(m_vertex_array_object);

    if (m_index_count > 0) {
        // Render with indices
        glDrawElements(GL_TRIANGLES, m_index_count, GL_UNSIGNED_INT, 0);
    } else if (m_vertex_count > 0) {
        // Render without indices
        glDrawArrays(GL_TRIANGLES, 0, m_vertex_count);
    }

    glBindVertexArray(0);
}

} // namespace Lupine

// Register the component
REGISTER_COMPONENT(PrimitiveMesh, "3D", "3D primitive mesh component for rendering basic shapes")
