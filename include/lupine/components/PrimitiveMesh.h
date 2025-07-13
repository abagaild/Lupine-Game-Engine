#pragma once

#include "lupine/core/Component.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>

// Forward declarations
namespace Lupine {
    class Node3D;
    class Node2D;
    class GraphicsTexture;
    class GraphicsVertexArray;
    class GraphicsBuffer;
}

namespace Lupine {

/**
 * @brief Primitive mesh component for rendering basic 3D shapes
 *
 * PrimitiveMesh component renders basic 3D geometric shapes.
 * It can be attached to Node3D or Node2D nodes.
 */
class PrimitiveMesh : public Component {
public:
    /**
     * @brief Primitive mesh types
     */
    enum class MeshType {
        Cube,
        Sphere,
        Cylinder,
        Plane,
        Cone,
        External  // Load from external mesh file
    };
    
    /**
     * @brief Constructor
     */
    PrimitiveMesh();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~PrimitiveMesh() = default;
    
    /**
     * @brief Get mesh type
     * @return Mesh type
     */
    MeshType GetMeshType() const { return m_mesh_type; }
    
    /**
     * @brief Set mesh type
     * @param type Mesh type
     */
    void SetMeshType(MeshType type);
    
    /**
     * @brief Get mesh size
     * @return Mesh size (width, height, depth)
     */
    const glm::vec3& GetSize() const { return m_size; }
    
    /**
     * @brief Set mesh size
     * @param size Mesh size (width, height, depth)
     */
    void SetSize(const glm::vec3& size);
    
    /**
     * @brief Get mesh color
     * @return Mesh color (RGBA)
     */
    const glm::vec4& GetColor() const { return m_color; }
    
    /**
     * @brief Set mesh color
     * @param color Mesh color (RGBA)
     */
    void SetColor(const glm::vec4& color);
    
    /**
     * @brief Get subdivision level (for sphere, cylinder, cone)
     * @return Subdivision level
     */
    int GetSubdivisions() const { return m_subdivisions; }
    
    /**
     * @brief Set subdivision level (for sphere, cylinder, cone)
     * @param subdivisions Subdivision level
     */
    void SetSubdivisions(int subdivisions);
    
    /**
     * @brief Get wireframe mode flag
     * @return True if wireframe mode is enabled
     */
    bool GetWireframe() const { return m_wireframe; }
    
    /**
     * @brief Set wireframe mode flag
     * @param wireframe True to enable wireframe mode
     */
    void SetWireframe(bool wireframe);
    
    /**
     * @brief Get double-sided flag
     * @return True if double-sided rendering is enabled
     */
    bool GetDoubleSided() const { return m_double_sided; }
    
    /**
     * @brief Set double-sided flag
     * @param double_sided True to enable double-sided rendering
     */
    void SetDoubleSided(bool double_sided);

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
     * @brief Get mesh file path (for External mesh type)
     * @return Path to mesh file
     */
    const std::string& GetMeshPath() const { return m_mesh_path; }

    /**
     * @brief Set mesh file path (for External mesh type)
     * @param path Path to mesh file
     */
    void SetMeshPath(const std::string& path);

    /**
     * @brief Get component type name
     * @return Type name string
     */
    std::string GetTypeName() const override { return "PrimitiveMesh"; }

    /**
     * @brief Get component category (for editor organization)
     * @return Category string
     */
    std::string GetCategory() const override { return "3D"; }

    /**
     * @brief Render mesh for shadow mapping (depth only)
     */
    void RenderForShadows();

    // Lifecycle methods
    void OnReady() override;
    void OnUpdate(float delta_time) override;

protected:
    void InitializeExportVariables() override;

private:
    MeshType m_mesh_type;
    glm::vec3 m_size;
    glm::vec4 m_color;
    int m_subdivisions;
    bool m_wireframe;
    bool m_double_sided;
    bool m_casts_shadows;
    bool m_receives_shadows;
    std::string m_mesh_path;  // Path to external mesh file
    
    // Internal rendering data
    std::shared_ptr<GraphicsVertexArray> m_vertex_array;
    std::shared_ptr<GraphicsBuffer> m_vertex_buffer;
    std::shared_ptr<GraphicsBuffer> m_index_buffer;
    int m_vertex_count;
    int m_index_count;
    bool m_mesh_generated;

    // Texture data for external meshes
    std::vector<std::shared_ptr<GraphicsTexture>> m_textures;
    bool m_has_textures;
    
    /**
     * @brief Generate mesh geometry
     */
    void GenerateMesh();
    
    /**
     * @brief Generate cube mesh
     */
    void GenerateCube();
    
    /**
     * @brief Generate sphere mesh
     */
    void GenerateSphere();
    
    /**
     * @brief Generate cylinder mesh
     */
    void GenerateCylinder();
    
    /**
     * @brief Generate plane mesh
     */
    void GeneratePlane();
    
    /**
     * @brief Generate cone mesh
     */
    void GenerateCone();

    /**
     * @brief Load external mesh from file
     */
    void LoadExternalMesh();

    /**
     * @brief Create OpenGL buffers for mesh data
     * @param vertices Vertex data (position, normal, texcoord)
     * @param indices Index data
     */
    void CreateMeshBuffers(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    
    /**
     * @brief Update export variables from internal state
     */
    void UpdateExportVariables();
    
    /**
     * @brief Update internal state from export variables
     */
    void UpdateFromExportVariables();
    
    /**
     * @brief Convert MeshType enum to int for export variables
     */
    int MeshTypeToInt(MeshType type) const;
    
    /**
     * @brief Convert int to MeshType enum from export variables
     */
    MeshType IntToMeshType(int type) const;
};

} // namespace Lupine
