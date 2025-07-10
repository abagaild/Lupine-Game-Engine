#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <glm/glm.hpp>
#include <QObject>
#include <QFileSystemWatcher>

namespace Lupine {

/**
 * @brief Primitive mesh types supported by Tile Builder
 */
enum class TilePrimitiveType {
    Cube,
    Rectangle,           // Adjustable dimensions
    TriangularPyramid,   // Half cube
    Pyramid,
    Cone,
    Sphere,
    CylinderOpen,
    CylinderClosed
};

/**
 * @brief Face identifier for texture assignment
 */
enum class MeshFace {
    Front = 0,
    Back = 1,
    Left = 2,
    Right = 3,
    Top = 4,
    Bottom = 5,
    // For cylinders
    Side = 6,
    // For cones/pyramids
    Base = 7,
    // For spheres (UV regions)
    North = 8,
    South = 9,
    East = 10,
    West = 11
};

/**
 * @brief Texture transformation data for a face
 */
struct FaceTextureTransform {
    glm::vec2 offset = glm::vec2(0.0f);     // UV offset
    glm::vec2 scale = glm::vec2(1.0f);      // UV scale
    float rotation = 0.0f;                   // Rotation in degrees
    std::string texture_path;                // Path to texture file
    bool use_full_texture = false;           // Use entire texture vs face region
};

/**
 * @brief Parameters for primitive mesh generation
 */
struct PrimitiveMeshParams {
    TilePrimitiveType type = TilePrimitiveType::Cube;
    
    // Dimensions
    glm::vec3 dimensions = glm::vec3(1.0f);  // Width, Height, Depth
    
    // Subdivision parameters
    int subdivisions = 16;                    // For spheres, cylinders, cones
    
    // Specific parameters
    float radius = 0.5f;                     // For spheres, cylinders, cones
    float height = 1.0f;                     // For cylinders, cones, pyramids
    bool closed = true;                      // For cylinders
    
    // UV mapping parameters
    bool generate_uvs = true;
    float uv_scale = 1.0f;
};

/**
 * @brief Generated mesh data with UV coordinates
 */
struct GeneratedMeshData {
    std::vector<float> vertices;             // Position, Normal, UV (8 floats per vertex)
    std::vector<unsigned int> indices;
    std::vector<std::string> face_names;     // Face names for texture assignment
    std::map<MeshFace, std::vector<int>> face_vertex_indices; // Face to vertex mapping
    
    // Bounding box
    glm::vec3 min_bounds;
    glm::vec3 max_bounds;
    
    // UV bounds for each face
    std::map<MeshFace, glm::vec4> face_uv_bounds; // min_u, min_v, max_u, max_v
};

/**
 * @brief Tile data for the builder
 */
struct TileBuilderData {
    std::string name = "New Tile";
    PrimitiveMeshParams mesh_params;
    GeneratedMeshData mesh_data;
    std::map<MeshFace, FaceTextureTransform> face_textures;
    
    // Generated files
    std::string temp_model_path;             // Temporary OBJ file
    std::string temp_texture_template_path;  // Template texture file
    std::string temp_material_path;          // MTL file
    
    // Export settings
    bool ready_for_export = false;
};

/**
 * @brief Core primitive mesh generator for Tile Builder
 */
class TilePrimitiveMeshGenerator {
public:
    /**
     * @brief Generate mesh data for specified primitive type
     * @param params Mesh generation parameters
     * @return Generated mesh data with UV coordinates
     */
    static GeneratedMeshData GenerateMesh(const PrimitiveMeshParams& params);

    /**
     * @brief Get available faces for a primitive type
     * @param type Primitive type
     * @return Vector of available faces
     */
    static std::vector<MeshFace> GetAvailableFaces(TilePrimitiveType type);

    /**
     * @brief Get face name for display
     * @param face Face identifier
     * @return Human-readable face name
     */
    static std::string GetFaceName(MeshFace face);

private:
    static GeneratedMeshData GenerateCube(const PrimitiveMeshParams& params);
    static GeneratedMeshData GenerateRectangle(const PrimitiveMeshParams& params);
    static GeneratedMeshData GenerateTriangularPyramid(const PrimitiveMeshParams& params);
    static GeneratedMeshData GeneratePyramid(const PrimitiveMeshParams& params);
    static GeneratedMeshData GenerateCone(const PrimitiveMeshParams& params);
    static GeneratedMeshData GenerateSphere(const PrimitiveMeshParams& params);
    static GeneratedMeshData GenerateCylinder(const PrimitiveMeshParams& params);

    static void CalculateBounds(GeneratedMeshData& mesh_data);
    static void CalculateFaceUVBounds(GeneratedMeshData& mesh_data);
};

/**
 * @brief Texture template generator for creating UV-mapped texture templates
 */
class TileTextureTemplateGenerator {
public:
    /**
     * @brief Generate texture template for a mesh
     * @param mesh_data Generated mesh data
     * @param template_size Size of template image (width, height)
     * @param output_path Path to save template image
     * @return True if successful
     */
    static bool GenerateTemplate(const GeneratedMeshData& mesh_data,
                               const glm::ivec2& template_size,
                               const std::string& output_path);

    /**
     * @brief Generate face-specific template
     * @param face Face to generate template for
     * @param face_bounds UV bounds for the face
     * @param template_size Size of template image
     * @param output_path Path to save template
     * @return True if successful
     */
    static bool GenerateFaceTemplate(MeshFace face,
                                   const glm::vec4& face_bounds,
                                   const glm::ivec2& template_size,
                                   const std::string& output_path);

private:
    static void DrawUVGrid(unsigned char* image_data, int width, int height, int channels);
    static void DrawFaceLabels(unsigned char* image_data, int width, int height,
                             const GeneratedMeshData& mesh_data);
};

/**
 * @brief File watcher for automatic texture reloading
 */
class TileTextureWatcher : public QObject {
    Q_OBJECT

public:
    explicit TileTextureWatcher(QObject* parent = nullptr);
    ~TileTextureWatcher();

    /**
     * @brief Add file to watch list
     * @param file_path Path to file to watch
     */
    void AddFile(const QString& file_path);

    /**
     * @brief Remove file from watch list
     * @param file_path Path to file to remove
     */
    void RemoveFile(const QString& file_path);

    /**
     * @brief Clear all watched files
     */
    void ClearFiles();

signals:
    /**
     * @brief Emitted when a watched file changes
     * @param file_path Path to changed file
     */
    void FileChanged(const QString& file_path);

private slots:
    void OnFileChanged(const QString& path);

private:
    QFileSystemWatcher* m_watcher;
    QStringList m_watched_files;
};

/**
 * @brief OBJ exporter for Tile Builder
 */
class TileOBJExporter {
public:
    /**
     * @brief Export tile data to OBJ format
     * @param tile_data Tile data to export
     * @param output_path Path to save OBJ file
     * @param export_materials Whether to export materials
     * @return True if successful
     */
    static bool ExportToOBJ(const TileBuilderData& tile_data,
                           const std::string& output_path,
                           bool export_materials = true);

private:
    static bool WriteMaterialFile(const TileBuilderData& tile_data,
                                const std::string& mtl_path);
    static std::string GetRelativePath(const std::string& from, const std::string& to);
};

} // namespace Lupine
