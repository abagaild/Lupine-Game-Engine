#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;
struct aiNodeAnim;

namespace Lupine {

/**
 * @brief Vertex data structure
 */
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 tex_coords;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    // Skeletal animation data
    glm::ivec4 bone_ids;      // Up to 4 bone indices per vertex
    glm::vec4 bone_weights;   // Corresponding bone weights (should sum to 1.0)

    /**
     * @brief Constructor with default values
     */
    Vertex() : position(0.0f), normal(0.0f), tex_coords(0.0f),
               tangent(0.0f), bitangent(0.0f),
               bone_ids(-1), bone_weights(0.0f) {}
};

/**
 * @brief Mesh texture information
 */
struct MeshTexture {
    unsigned int id;
    std::string type;
    std::string path;
};

/**
 * @brief Material properties
 */
struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;

    std::vector<MeshTexture> diffuse_maps;
    std::vector<MeshTexture> specular_maps;
    std::vector<MeshTexture> normal_maps;
    std::vector<MeshTexture> height_maps;
};

/**
 * @brief Bone data structure for skeletal animation
 */
struct Bone {
    std::string name;
    int id;
    glm::mat4 offset_matrix;  // Transforms from model space to bone space

    /**
     * @brief Constructor
     */
    Bone(const std::string& bone_name, int bone_id, const glm::mat4& offset)
        : name(bone_name), id(bone_id), offset_matrix(offset) {}
};

/**
 * @brief Animation keyframe for a single bone
 */
struct BoneKeyframe {
    float time;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    /**
     * @brief Constructor
     */
    BoneKeyframe(float t, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl)
        : time(t), position(pos), rotation(rot), scale(scl) {}
};

/**
 * @brief Animation channel for a single bone
 */
struct BoneAnimation {
    std::string bone_name;
    std::vector<BoneKeyframe> keyframes;

    /**
     * @brief Get interpolated transformation at given time
     * @param time Animation time
     * @return Interpolated transformation matrix
     */
    glm::mat4 GetTransformAtTime(float time) const;
};

/**
 * @brief Complete skeletal animation clip
 */
struct SkeletalAnimationClip {
    std::string name;
    float duration;
    float ticks_per_second;
    std::vector<BoneAnimation> bone_animations;

    /**
     * @brief Constructor
     */
    SkeletalAnimationClip(const std::string& anim_name, float dur, float tps)
        : name(anim_name), duration(dur), ticks_per_second(tps) {}
};

/**
 * @brief Skeleton structure containing bone hierarchy
 */
struct Skeleton {
    std::vector<Bone> bones;
    std::map<std::string, int> bone_name_to_id;
    glm::mat4 global_inverse_transform;

    /**
     * @brief Add a bone to the skeleton
     * @param bone Bone to add
     */
    void AddBone(const Bone& bone);

    /**
     * @brief Get bone by name
     * @param name Bone name
     * @return Pointer to bone or nullptr if not found
     */
    const Bone* GetBone(const std::string& name) const;

    /**
     * @brief Get bone by ID
     * @param id Bone ID
     * @return Pointer to bone or nullptr if not found
     */
    const Bone* GetBone(int id) const;
};

/**
 * @brief Mesh data structure
 */
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Material material;

    // Skeletal animation data
    bool has_bones;
    std::vector<Bone> bones;

    // OpenGL objects
    unsigned int VAO, VBO, EBO;

    /**
     * @brief Constructor
     */
    Mesh() : has_bones(false), VAO(0), VBO(0), EBO(0) {}

    /**
     * @brief Setup mesh for rendering
     */
    void SetupMesh();

    /**
     * @brief Draw the mesh
     */
    void Draw() const;
};

/**
 * @brief Model class containing multiple meshes
 */
class Model {
public:
    /**
     * @brief Constructor
     * @param path Path to model file
     */
    explicit Model(const std::string& path);

    /**
     * @brief Destructor
     */
    ~Model() = default;

    /**
     * @brief Draw the entire model
     */
    void Draw() const;

    /**
     * @brief Get all meshes
     * @return Vector of meshes
     */
    const std::vector<Mesh>& GetMeshes() const { return m_meshes; }

    /**
     * @brief Check if model loaded successfully
     * @return True if loaded
     */
    bool IsLoaded() const { return m_loaded; }

    /**
     * @brief Check if model has skeletal animations
     * @return True if model has animations
     */
    bool HasAnimations() const { return m_has_animations; }

    /**
     * @brief Get skeleton data
     * @return Reference to skeleton
     */
    const Skeleton& GetSkeleton() const { return m_skeleton; }

    /**
     * @brief Get animation clips
     * @return Vector of animation clips
     */
    const std::vector<SkeletalAnimationClip>& GetAnimations() const { return m_animations; }

    /**
     * @brief Get animation by name
     * @param name Animation name
     * @return Pointer to animation or nullptr if not found
     */
    const SkeletalAnimationClip* GetAnimation(const std::string& name) const;

private:
    std::vector<Mesh> m_meshes;
    std::string m_directory;
    std::vector<MeshTexture> m_loaded_textures;
    bool m_loaded;

    // Skeletal animation data
    Skeleton m_skeleton;
    std::vector<SkeletalAnimationClip> m_animations;
    bool m_has_animations;

    /**
     * @brief Load model using Assimp
     * @param path Path to model file
     */
    void LoadModel(const std::string& path);

    /**
     * @brief Process Assimp node recursively
     * @param node Assimp node
     * @param scene Assimp scene
     */
    void ProcessNode(aiNode* node, const aiScene* scene);

    /**
     * @brief Process Assimp mesh
     * @param mesh Assimp mesh
     * @param scene Assimp scene
     * @return Processed mesh
     */
    Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

    /**
     * @brief Load material textures
     * @param mat Assimp material
     * @param type Texture type
     * @param type_name Type name string
     * @return Vector of textures
     */
    std::vector<MeshTexture> LoadMaterialTextures(aiMaterial* mat, unsigned int type, const std::string& type_name);

    /**
     * @brief Load texture from file
     * @param path Texture file path
     * @param directory Directory containing texture
     * @return Texture ID
     */
    unsigned int LoadTextureFromFile(const std::string& path, const std::string& directory);

    /**
     * @brief Process skeletal data from Assimp scene
     * @param scene Assimp scene
     */
    void ProcessSkeleton(const aiScene* scene);

    /**
     * @brief Process animations from Assimp scene
     * @param scene Assimp scene
     */
    void ProcessAnimations(const aiScene* scene);

    /**
     * @brief Extract bone data from mesh
     * @param mesh Assimp mesh
     * @param vertices Vertex array to update with bone data
     */
    void ExtractBoneData(aiMesh* mesh, std::vector<Vertex>& vertices);

    /**
     * @brief Interpolate position at given time
     * @param time Animation time
     * @param channel Animation channel
     * @return Interpolated position
     */
    glm::vec3 InterpolatePosition(float time, const aiNodeAnim* channel) const;

    /**
     * @brief Interpolate rotation at given time
     * @param time Animation time
     * @param channel Animation channel
     * @return Interpolated rotation
     */
    glm::quat InterpolateRotation(float time, const aiNodeAnim* channel) const;

    /**
     * @brief Interpolate scale at given time
     * @param time Animation time
     * @param channel Animation channel
     * @return Interpolated scale
     */
    glm::vec3 InterpolateScale(float time, const aiNodeAnim* channel) const;
};

/**
 * @brief Mesh loader utility class
 */
class MeshLoader {
public:
    /**
     * @brief Load a model from file
     * @param path Path to model file
     * @return Unique pointer to loaded model
     */
    static std::unique_ptr<Model> LoadModel(const std::string& path);

    /**
     * @brief Check if file format is supported
     * @param path Path to model file
     * @return True if supported
     */
    static bool IsFormatSupported(const std::string& path);

    /**
     * @brief Get supported file extensions
     * @return Vector of supported extensions
     */
    static std::vector<std::string> GetSupportedExtensions();
};

} // namespace Lupine
