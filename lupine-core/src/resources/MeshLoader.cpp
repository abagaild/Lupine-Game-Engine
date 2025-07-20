#include "lupine/resources/MeshLoader.h"
#include "lupine/resources/ResourceManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <set>

#include <glad/glad.h>

namespace Lupine {

void Mesh::SetupMesh() {
    // Safety check for empty mesh data
    if (vertices.empty()) {
        std::cerr << "Mesh::SetupMesh: Warning - attempting to setup mesh with no vertices" << std::endl;
        return;
    }

    try {
        // Generate OpenGL objects
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Check for OpenGL errors after generation
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "Mesh::SetupMesh: OpenGL error generating buffers: " << error << std::endl;
            return;
        }

        // Verify objects were created
        if (VAO == 0 || VBO == 0 || EBO == 0) {
            std::cerr << "Mesh::SetupMesh: Failed to generate OpenGL objects" << std::endl;
            return;
        }

        // Bind VAO
        glBindVertexArray(VAO);

        // Load vertex data
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        // Check for errors after vertex buffer
        error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "Mesh::SetupMesh: OpenGL error setting up vertex buffer: " << error << std::endl;
            glBindVertexArray(0);
            return;
        }

        // Load index data (only if we have indices)
        if (!indices.empty()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

            // Check for errors after index buffer
            error = glGetError();
            if (error != GL_NO_ERROR) {
                std::cerr << "Mesh::SetupMesh: OpenGL error setting up index buffer: " << error << std::endl;
                glBindVertexArray(0);
                return;
            }
        }

        // Set vertex attribute pointers
        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        // Normal attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        // Texture coordinate attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_coords));

        // Bone IDs attribute (integers) - location 3 to match skinned mesh shader
        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, bone_ids));

        // Bone weights attribute - location 4 to match skinned mesh shader
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bone_weights));

        // Tangent attribute - moved to location 5
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

        // Bitangent attribute - moved to location 6
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

        // Check for final errors
        error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "Mesh::SetupMesh: OpenGL error setting up vertex attributes: " << error << std::endl;
        }

        // Unbind VAO
        glBindVertexArray(0);

    } catch (const std::exception& e) {
        std::cerr << "Mesh::SetupMesh: Exception during setup: " << e.what() << std::endl;
        // Cleanup on error
        glBindVertexArray(0);
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (EBO != 0) {
            glDeleteBuffers(1, &EBO);
            EBO = 0;
        }
    } catch (...) {
        std::cerr << "Mesh::SetupMesh: Unknown exception during setup" << std::endl;
        // Cleanup on error
        glBindVertexArray(0);
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (EBO != 0) {
            glDeleteBuffers(1, &EBO);
            EBO = 0;
        }
    }
}

void Mesh::Draw() const {
    // Safety check - don't draw if VAO is not set up
    if (VAO == 0) {
        return;
    }

    // Bind diffuse textures
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;

    for (unsigned int i = 0; i < material.diffuse_maps.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        std::string number;
        std::string name = material.diffuse_maps[i].type;
        if (name == "texture_diffuse")
            number = std::to_string(diffuseNr++);
        else if (name == "texture_specular")
            number = std::to_string(specularNr++);
        else if (name == "texture_normal")
            number = std::to_string(normalNr++);
        else if (name == "texture_height")
            number = std::to_string(heightNr++);

        // Note: This would normally set a shader uniform, but we don't have shader system yet
        // glUniform1i(glGetUniformLocation(shader, (name + number).c_str()), i);
        glBindTexture(GL_TEXTURE_2D, material.diffuse_maps[i].id);
    }

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Reset to default
    glActiveTexture(GL_TEXTURE0);
}

Model::Model(const std::string& path) : m_loaded(false), m_has_animations(false) {
    LoadModel(path);
}

void Model::Draw() const {
    for (const auto& mesh : m_meshes) {
        // Ensure mesh is set up for OpenGL rendering (lazy initialization)
        if (mesh.VAO == 0) {
            // Cast away const to allow lazy initialization
            const_cast<Mesh&>(mesh).SetupMesh();
        }
        mesh.Draw();
    }
}

const SkeletalAnimationClip* Model::GetAnimation(const std::string& name) const {
    for (const auto& animation : m_animations) {
        if (animation.name == name) {
            return &animation;
        }
    }
    return nullptr;
}

void Model::LoadModel(const std::string& path) {
    // Validate input path
    if (path.empty()) {
        std::cerr << "Model::LoadModel: Empty path provided" << std::endl;
        return;
    }

    // Check if file exists before attempting to load
    if (!std::filesystem::exists(path)) {
        std::cerr << "Model::LoadModel: File does not exist: " << path << std::endl;
        return;
    }

    // Check if it's actually a file and not a directory
    if (std::filesystem::is_directory(path)) {
        std::cerr << "Model::LoadModel: Path is a directory, not a file: " << path << std::endl;
        return;
    }

    std::cout << "Model::LoadModel: Loading model from " << path << std::endl;

    Assimp::Importer importer;

    // Set import settings for better error handling and performance
    importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4); // Limit bone weights to 4 per vertex
    importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f); // Smoothing angle for normal generation

    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_CalcTangentSpace |
        aiProcess_GenNormals |
        aiProcess_LimitBoneWeights |
        aiProcess_ValidateDataStructure |
        aiProcess_ImproveCacheLocality |
        aiProcess_RemoveRedundantMaterials |
        aiProcess_FixInfacingNormals |
        aiProcess_FindDegenerates |
        aiProcess_FindInvalidData |
        aiProcess_GenUVCoords);

    if (!scene) {
        std::cerr << "Model::LoadModel: ASSIMP failed to load scene: " << importer.GetErrorString() << std::endl;
        return;
    }

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        std::cerr << "Model::LoadModel: ASSIMP scene is incomplete: " << importer.GetErrorString() << std::endl;
        return;
    }

    if (!scene->mRootNode) {
        std::cerr << "Model::LoadModel: ASSIMP scene has no root node" << std::endl;
        return;
    }

    // Validate scene has meshes
    if (scene->mNumMeshes == 0) {
        std::cerr << "Model::LoadModel: Scene contains no meshes: " << path << std::endl;
        return;
    }

    std::cout << "Model::LoadModel: Scene loaded successfully - " << scene->mNumMeshes << " meshes, "
              << scene->mNumMaterials << " materials" << std::endl;

    m_directory = std::filesystem::path(path).parent_path().string();

    try {
        // Process skeleton and animations if present
        if (scene->HasAnimations()) {
            std::cout << "Model::LoadModel: Processing " << scene->mNumAnimations << " animations" << std::endl;
            ProcessSkeleton(scene);
            ProcessAnimations(scene);
            m_has_animations = true;
            std::cout << "Model: Loaded " << m_animations.size() << " animations" << std::endl;
        }

        // Process the scene hierarchy
        ProcessNode(scene->mRootNode, scene);

        if (m_meshes.empty()) {
            std::cerr << "Model::LoadModel: No meshes were successfully processed from " << path << std::endl;
            return;
        }

        m_loaded = true;
        std::cout << "Model::LoadModel: Successfully loaded model with " << m_meshes.size() << " meshes" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Model::LoadModel: Exception during processing: " << e.what() << std::endl;
        m_loaded = false;
        m_meshes.clear();
        m_animations.clear();
    }
}

void Model::ProcessNode(aiNode* node, const aiScene* scene) {
    // Validate input parameters
    if (!node || !scene) {
        std::cerr << "Model::ProcessNode: Invalid node or scene pointer" << std::endl;
        return;
    }

    // Process all meshes in the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        unsigned int mesh_index = node->mMeshes[i];

        // Validate mesh index
        if (mesh_index >= scene->mNumMeshes) {
            std::cerr << "Model::ProcessNode: Invalid mesh index " << mesh_index
                     << " (max: " << scene->mNumMeshes << ")" << std::endl;
            continue;
        }

        aiMesh* mesh = scene->mMeshes[mesh_index];
        if (!mesh) {
            std::cerr << "Model::ProcessNode: Null mesh at index " << mesh_index << std::endl;
            continue;
        }

        try {
            Mesh processed_mesh = ProcessMesh(mesh, scene);
            m_meshes.push_back(std::move(processed_mesh));
        } catch (const std::exception& e) {
            std::cerr << "Model::ProcessNode: Exception processing mesh " << mesh_index
                     << ": " << e.what() << std::endl;
        }
    }

    // Process all child nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        aiNode* child = node->mChildren[i];
        if (child) {
            ProcessNode(child, scene);
        } else {
            std::cerr << "Model::ProcessNode: Null child node at index " << i << std::endl;
        }
    }
}

Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
    // Validate input parameters
    if (!mesh || !scene) {
        std::cerr << "Model::ProcessMesh: Invalid mesh or scene pointer" << std::endl;
        return Mesh(); // Return empty mesh
    }

    // Validate mesh has vertices
    if (mesh->mNumVertices == 0) {
        std::cerr << "Model::ProcessMesh: Mesh has no vertices" << std::endl;
        return Mesh();
    }

    // Validate vertex data pointer
    if (!mesh->mVertices) {
        std::cerr << "Model::ProcessMesh: Mesh vertices pointer is null" << std::endl;
        return Mesh();
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    Material material;

    // Reserve space for better performance
    vertices.reserve(mesh->mNumVertices);

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;

        // Position (always required)
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;

        // Validate position for NaN or infinite values
        if (!std::isfinite(vertex.position.x) || !std::isfinite(vertex.position.y) || !std::isfinite(vertex.position.z)) {
            std::cerr << "Model::ProcessMesh: Invalid vertex position at index " << i << std::endl;
            vertex.position = glm::vec3(0.0f); // Use default position
        }

        // Normal
        if (mesh->HasNormals() && mesh->mNormals) {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;

            // Validate normal
            if (!std::isfinite(vertex.normal.x) || !std::isfinite(vertex.normal.y) || !std::isfinite(vertex.normal.z)) {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default up normal
            }
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default normal
        }

        // Texture coordinates
        if (mesh->mTextureCoords[0]) {
            vertex.tex_coords.x = mesh->mTextureCoords[0][i].x;
            vertex.tex_coords.y = mesh->mTextureCoords[0][i].y;

            // Validate texture coordinates
            if (!std::isfinite(vertex.tex_coords.x) || !std::isfinite(vertex.tex_coords.y)) {
                vertex.tex_coords = glm::vec2(0.0f, 0.0f);
            }
        } else {
            vertex.tex_coords = glm::vec2(0.0f, 0.0f);
        }

        // Tangent and Bitangent
        if (mesh->HasTangentsAndBitangents() && mesh->mTangents && mesh->mBitangents) {
            vertex.tangent.x = mesh->mTangents[i].x;
            vertex.tangent.y = mesh->mTangents[i].y;
            vertex.tangent.z = mesh->mTangents[i].z;

            vertex.bitangent.x = mesh->mBitangents[i].x;
            vertex.bitangent.y = mesh->mBitangents[i].y;
            vertex.bitangent.z = mesh->mBitangents[i].z;

            // Validate tangent and bitangent
            if (!std::isfinite(vertex.tangent.x) || !std::isfinite(vertex.tangent.y) || !std::isfinite(vertex.tangent.z)) {
                vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f); // Default tangent
            }
            if (!std::isfinite(vertex.bitangent.x) || !std::isfinite(vertex.bitangent.y) || !std::isfinite(vertex.bitangent.z)) {
                vertex.bitangent = glm::vec3(0.0f, 0.0f, 1.0f); // Default bitangent
            }
        } else {
            vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            vertex.bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        vertices.push_back(vertex);
    }

    // Extract bone data if present
    if (mesh->HasBones()) {
        try {
            ExtractBoneData(mesh, vertices);
        } catch (const std::exception& e) {
            std::cerr << "Model::ProcessMesh: Exception extracting bone data: " << e.what() << std::endl;
        }
    }

    // Process indices
    if (mesh->mNumFaces > 0 && mesh->mFaces) {
        // Reserve space for indices (estimate 3 indices per face)
        indices.reserve(mesh->mNumFaces * 3);

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace& face = mesh->mFaces[i];

            // Validate face has indices
            if (face.mNumIndices == 0 || !face.mIndices) {
                std::cerr << "Model::ProcessMesh: Face " << i << " has no indices" << std::endl;
                continue;
            }

            // Only process triangulated faces (should be guaranteed by aiProcess_Triangulate)
            if (face.mNumIndices != 3) {
                std::cerr << "Model::ProcessMesh: Face " << i << " is not triangulated (has "
                         << face.mNumIndices << " indices)" << std::endl;
                continue;
            }

            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                unsigned int index = face.mIndices[j];

                // Validate index is within vertex range
                if (index >= mesh->mNumVertices) {
                    std::cerr << "Model::ProcessMesh: Invalid vertex index " << index
                             << " (max: " << mesh->mNumVertices << ")" << std::endl;
                    continue;
                }

                indices.push_back(index);
            }
        }
    } else {
        std::cerr << "Model::ProcessMesh: Mesh has no faces" << std::endl;
    }

    // Process material
    if (mesh->mMaterialIndex >= 0 && static_cast<unsigned int>(mesh->mMaterialIndex) < scene->mNumMaterials) {
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

        if (mat) {
            try {
                // Load diffuse maps
                material.diffuse_maps = LoadMaterialTextures(mat, aiTextureType_DIFFUSE, "texture_diffuse");

                // Load specular maps
                material.specular_maps = LoadMaterialTextures(mat, aiTextureType_SPECULAR, "texture_specular");

                // Load normal maps
                material.normal_maps = LoadMaterialTextures(mat, aiTextureType_HEIGHT, "texture_normal");

                // Load height maps
                material.height_maps = LoadMaterialTextures(mat, aiTextureType_AMBIENT, "texture_height");
            } catch (const std::exception& e) {
                std::cerr << "Model::ProcessMesh: Exception loading material textures: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Model::ProcessMesh: Material pointer is null for index " << mesh->mMaterialIndex << std::endl;
        }
    } else if (mesh->mMaterialIndex >= 0) {
        std::cerr << "Model::ProcessMesh: Invalid material index " << mesh->mMaterialIndex
                 << " (max: " << scene->mNumMaterials << ")" << std::endl;
    }

    // Validate we have valid data before creating mesh
    if (vertices.empty()) {
        std::cerr << "Model::ProcessMesh: No vertices processed" << std::endl;
        return Mesh(); // Return empty mesh
    }

    if (indices.empty()) {
        std::cerr << "Model::ProcessMesh: No indices processed" << std::endl;
        return Mesh(); // Return empty mesh
    }

    Mesh result;
    result.vertices = std::move(vertices);
    result.indices = std::move(indices);
    result.material = std::move(material);
    // Don't call SetupMesh() here - defer it until render time when OpenGL context is active

    return result;
}

std::vector<MeshTexture> Model::LoadMaterialTextures(aiMaterial* mat, unsigned int type, const std::string& type_name) {
    std::vector<MeshTexture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(static_cast<aiTextureType>(type)); i++) {
        aiString str;
        mat->GetTexture(static_cast<aiTextureType>(type), i, &str);

        // Validate texture path from material
        std::string texture_path = str.C_Str();
        if (texture_path.empty()) {
            std::cerr << "LoadMaterialTextures: Empty texture path in material for type " << type_name << std::endl;
            continue;
        }

        // Check if texture was loaded before
        bool skip = false;
        for (const auto& loaded_texture : m_loaded_textures) {
            if (std::strcmp(loaded_texture.path.data(), texture_path.c_str()) == 0) {
                textures.push_back(loaded_texture);
                skip = true;
                break;
            }
        }

        if (!skip) {
            MeshTexture texture;
            texture.id = LoadTextureFromFile(texture_path, m_directory);
            texture.type = type_name;
            texture.path = texture_path;

            // Only add texture if it was successfully loaded
            if (texture.id != 0) {
                textures.push_back(texture);
                m_loaded_textures.push_back(texture);
            } else {
                std::cerr << "LoadMaterialTextures: Failed to load texture '" << texture_path
                         << "' for material type " << type_name << std::endl;
            }
        }
    }

    return textures;
}

unsigned int Model::LoadTextureFromFile(const std::string& path, const std::string& directory) {
    // Validate input parameters
    if (path.empty()) {
        std::cerr << "LoadTextureFromFile: Empty texture path provided" << std::endl;
        return 0;
    }

    // Clean and validate the path
    std::string clean_path = path;

    // Remove any trailing dots or slashes that might cause issues
    while (!clean_path.empty() && (clean_path.back() == '.' || clean_path.back() == '/' || clean_path.back() == '\\')) {
        clean_path.pop_back();
    }

    if (clean_path.empty()) {
        std::cerr << "LoadTextureFromFile: Invalid texture path after cleaning: '" << path << "'" << std::endl;
        return 0;
    }

    // Construct full filename with proper path separator
    std::string filename;
    if (directory.empty()) {
        filename = clean_path;
    } else {
        filename = directory + '/' + clean_path;
    }

    // Normalize path separators for the current platform
    std::replace(filename.begin(), filename.end(), '\\', '/');

    // Check if file exists
    if (!std::filesystem::exists(filename)) {
        std::cerr << "Texture file not found: " << filename << " (original path: '" << path << "')" << std::endl;
        return 0;
    }

    // Additional validation - check if it's actually a file and not a directory
    if (std::filesystem::is_directory(filename)) {
        std::cerr << "LoadTextureFromFile: Path is a directory, not a file: " << filename << std::endl;
        return 0;
    }

    // Load texture using ResourceManager
    Texture texture = ResourceManager::LoadTexture(filename, true);
    if (!texture.IsValid()) {
        std::cerr << "Failed to load texture: " << filename << " (ResourceManager failed)" << std::endl;
        return 0;
    }

    std::cout << "Loaded texture: " << filename << " (ID: " << texture.id << ")" << std::endl;
    return texture.id;
}

std::unique_ptr<Model> MeshLoader::LoadModel(const std::string& path) {
    return std::make_unique<Model>(path);
}

bool MeshLoader::IsFormatSupported(const std::string& path) {
    std::filesystem::path file_path(path);
    std::string extension = file_path.extension().string();

    // Convert to lowercase
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    auto supported = GetSupportedExtensions();
    return std::find(supported.begin(), supported.end(), extension) != supported.end();
}

std::vector<std::string> MeshLoader::GetSupportedExtensions() {
    return {
        ".obj", ".fbx", ".dae", ".gltf", ".glb", ".blend", ".3ds", ".ase", ".ifc",
        ".xgl", ".zgl", ".ply", ".dxf", ".lwo", ".lws", ".lxo", ".stl", ".x", ".ac",
        ".ms3d", ".cob", ".scn", ".bvh", ".csm", ".xml", ".irrmesh", ".irr", ".mdl",
        ".md2", ".md3", ".pk3", ".mdc", ".md5", ".smd", ".vta", ".ogex", ".3d", ".b3d",
        ".q3d", ".q3s", ".nff", ".nff", ".off", ".raw", ".ter", ".hmp", ".ndo"
    };
}

// Skeleton utility methods implementation
void Skeleton::AddBone(const Bone& bone) {
    bones.push_back(bone);
    bone_name_to_id[bone.name] = bone.id;
}

const Bone* Skeleton::GetBone(const std::string& name) const {
    auto it = bone_name_to_id.find(name);
    if (it != bone_name_to_id.end()) {
        return GetBone(it->second);
    }
    return nullptr;
}

const Bone* Skeleton::GetBone(int id) const {
    for (const auto& bone : bones) {
        if (bone.id == id) {
            return &bone;
        }
    }
    return nullptr;
}

// Model skeletal animation processing methods
void Model::ProcessSkeleton(const aiScene* scene) {
    // Set global inverse transform
    aiMatrix4x4 globalTransform = scene->mRootNode->mTransformation;
    globalTransform.Inverse();

    m_skeleton.global_inverse_transform = glm::mat4(
        globalTransform.a1, globalTransform.b1, globalTransform.c1, globalTransform.d1,
        globalTransform.a2, globalTransform.b2, globalTransform.c2, globalTransform.d2,
        globalTransform.a3, globalTransform.b3, globalTransform.c3, globalTransform.d3,
        globalTransform.a4, globalTransform.b4, globalTransform.c4, globalTransform.d4
    );

    // Extract bones from all meshes
    int bone_counter = 0;
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* mesh = scene->mMeshes[m];

        for (unsigned int b = 0; b < mesh->mNumBones; b++) {
            aiBone* bone = mesh->mBones[b];
            std::string bone_name = bone->mName.data;

            // Check if bone already exists
            if (m_skeleton.bone_name_to_id.find(bone_name) == m_skeleton.bone_name_to_id.end()) {
                // Convert offset matrix
                aiMatrix4x4& offset = bone->mOffsetMatrix;
                glm::mat4 offset_matrix(
                    offset.a1, offset.b1, offset.c1, offset.d1,
                    offset.a2, offset.b2, offset.c2, offset.d2,
                    offset.a3, offset.b3, offset.c3, offset.d3,
                    offset.a4, offset.b4, offset.c4, offset.d4
                );

                Bone new_bone(bone_name, bone_counter++, offset_matrix);
                m_skeleton.AddBone(new_bone);
            }
        }
    }

    std::cout << "Model: Processed skeleton with " << m_skeleton.bones.size() << " bones" << std::endl;
}

void Model::ProcessAnimations(const aiScene* scene) {
    for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* anim = scene->mAnimations[i];

        std::string anim_name = anim->mName.data;
        if (anim_name.empty()) {
            anim_name = "Animation_" + std::to_string(i);
        }

        float duration = static_cast<float>(anim->mDuration);
        float ticks_per_second = static_cast<float>(anim->mTicksPerSecond);
        if (ticks_per_second == 0.0f) {
            ticks_per_second = 25.0f; // Default fallback
        }

        SkeletalAnimationClip clip(anim_name, duration / ticks_per_second, ticks_per_second);

        // Process bone animations
        for (unsigned int c = 0; c < anim->mNumChannels; c++) {
            aiNodeAnim* channel = anim->mChannels[c];
            std::string bone_name = channel->mNodeName.data;

            BoneAnimation bone_anim;
            bone_anim.bone_name = bone_name;

            // Collect all unique time stamps
            std::set<float> time_stamps;
            for (unsigned int p = 0; p < channel->mNumPositionKeys; p++) {
                time_stamps.insert(static_cast<float>(channel->mPositionKeys[p].mTime));
            }
            for (unsigned int r = 0; r < channel->mNumRotationKeys; r++) {
                time_stamps.insert(static_cast<float>(channel->mRotationKeys[r].mTime));
            }
            for (unsigned int s = 0; s < channel->mNumScalingKeys; s++) {
                time_stamps.insert(static_cast<float>(channel->mScalingKeys[s].mTime));
            }

            // Create keyframes for each timestamp
            for (float time : time_stamps) {
                glm::vec3 position = InterpolatePosition(time, channel);
                glm::quat rotation = InterpolateRotation(time, channel);
                glm::vec3 scale = InterpolateScale(time, channel);

                bone_anim.keyframes.emplace_back(time / ticks_per_second, position, rotation, scale);
            }

            clip.bone_animations.push_back(bone_anim);
        }

        m_animations.push_back(clip);
        std::cout << "Model: Loaded animation '" << anim_name << "' (" << duration / ticks_per_second << "s)" << std::endl;
    }
}

void Model::ExtractBoneData(aiMesh* mesh, std::vector<Vertex>& vertices) {
    // Validate input parameters
    if (!mesh || !mesh->mBones || mesh->mNumBones == 0) {
        std::cerr << "Model::ExtractBoneData: Invalid mesh or no bones" << std::endl;
        return;
    }

    if (vertices.empty()) {
        std::cerr << "Model::ExtractBoneData: No vertices to process" << std::endl;
        return;
    }

    for (unsigned int bone_index = 0; bone_index < mesh->mNumBones; bone_index++) {
        aiBone* bone = mesh->mBones[bone_index];

        // Validate bone pointer
        if (!bone) {
            std::cerr << "Model::ExtractBoneData: Null bone at index " << bone_index << std::endl;
            continue;
        }

        std::string bone_name = bone->mName.data;
        if (bone_name.empty()) {
            std::cerr << "Model::ExtractBoneData: Empty bone name at index " << bone_index << std::endl;
            continue;
        }

        // Get bone ID from skeleton
        auto it = m_skeleton.bone_name_to_id.find(bone_name);
        if (it == m_skeleton.bone_name_to_id.end()) {
            std::cerr << "Model::ExtractBoneData: Bone '" << bone_name << "' not found in skeleton" << std::endl;
            continue; // Bone not found in skeleton
        }

        int bone_id = it->second;

        // Validate bone weights array
        if (!bone->mWeights || bone->mNumWeights == 0) {
            std::cerr << "Model::ExtractBoneData: Bone '" << bone_name << "' has no weights" << std::endl;
            continue;
        }

        // Process bone weights
        for (unsigned int weight_index = 0; weight_index < bone->mNumWeights; weight_index++) {
            aiVertexWeight weight = bone->mWeights[weight_index];
            unsigned int vertex_id = weight.mVertexId;
            float bone_weight = weight.mWeight;

            // Validate vertex ID
            if (vertex_id >= vertices.size()) {
                std::cerr << "Model::ExtractBoneData: Invalid vertex ID " << vertex_id
                         << " for bone '" << bone_name << "' (max: " << vertices.size() << ")" << std::endl;
                continue;
            }

            // Validate bone weight
            if (bone_weight <= 0.0f || !std::isfinite(bone_weight)) {
                std::cerr << "Model::ExtractBoneData: Invalid bone weight " << bone_weight
                         << " for vertex " << vertex_id << ", bone '" << bone_name << "'" << std::endl;
                continue;
            }

            // Find an empty slot in the vertex bone data
            bool weight_assigned = false;
            for (int i = 0; i < 4; i++) {
                if (vertices[vertex_id].bone_weights[i] == 0.0f) {
                    vertices[vertex_id].bone_ids[i] = bone_id;
                    vertices[vertex_id].bone_weights[i] = bone_weight;
                    weight_assigned = true;
                    break;
                }
            }

            if (!weight_assigned) {
                std::cerr << "Model::ExtractBoneData: Vertex " << vertex_id
                         << " already has 4 bone weights, skipping bone '" << bone_name << "'" << std::endl;
            }
        }
    }

    // Normalize bone weights to ensure they sum to 1.0 and validate bone data
    unsigned int vertices_with_bones = 0;
    unsigned int vertices_with_invalid_weights = 0;
    unsigned int vertices_with_too_many_bones = 0;

    for (size_t i = 0; i < vertices.size(); i++) {
        auto& vertex = vertices[i];
        float total_weight = vertex.bone_weights.x + vertex.bone_weights.y +
                           vertex.bone_weights.z + vertex.bone_weights.w;

        // Count active bone influences
        int active_bones = 0;
        for (int j = 0; j < 4; j++) {
            if (vertex.bone_weights[j] > 0.0f && vertex.bone_ids[j] >= 0) {
                active_bones++;
            }
        }

        if (total_weight > 0.0f) {
            vertices_with_bones++;

            if (std::isfinite(total_weight)) {
                // Check if total weight is reasonable (not too far from 1.0)
                if (total_weight < 0.001f || total_weight > 10.0f) {
                    std::cerr << "Model::ExtractBoneData: Unusual total weight " << total_weight
                             << " for vertex " << i << std::endl;
                    vertices_with_invalid_weights++;
                }

                // Normalize weights
                vertex.bone_weights /= total_weight;

                // Verify normalization worked
                float normalized_total = vertex.bone_weights.x + vertex.bone_weights.y +
                                       vertex.bone_weights.z + vertex.bone_weights.w;
                if (std::abs(normalized_total - 1.0f) > 0.001f) {
                    std::cerr << "Model::ExtractBoneData: Normalization failed for vertex " << i
                             << ", total: " << normalized_total << std::endl;
                }
            } else {
                std::cerr << "Model::ExtractBoneData: Invalid total weight " << total_weight
                         << " for vertex " << i << std::endl;
                vertex.bone_weights = glm::vec4(0.0f);
                vertex.bone_ids = glm::ivec4(-1);
                vertices_with_invalid_weights++;
            }
        } else {
            // Vertex has no bone influences - ensure bone IDs are set to -1
            vertex.bone_ids = glm::ivec4(-1);
            vertex.bone_weights = glm::vec4(0.0f);
        }

        // Check for vertices with more than 4 bone influences (should not happen due to aiProcess_LimitBoneWeights)
        if (active_bones > 4) {
            vertices_with_too_many_bones++;
        }
    }

    // Print statistics
    std::cout << "Model::ExtractBoneData: Processed " << vertices.size() << " vertices" << std::endl;
    std::cout << "  - Vertices with bone influences: " << vertices_with_bones << std::endl;
    std::cout << "  - Vertices with invalid weights: " << vertices_with_invalid_weights << std::endl;
    std::cout << "  - Vertices with too many bones: " << vertices_with_too_many_bones << std::endl;
}

glm::vec3 Model::InterpolatePosition(float time, const aiNodeAnim* channel) const {
    // Validate input parameters
    if (!channel || !channel->mPositionKeys || channel->mNumPositionKeys == 0) {
        std::cerr << "Model::InterpolatePosition: Invalid channel or no position keys" << std::endl;
        return glm::vec3(0.0f);
    }

    // Validate time parameter
    if (!std::isfinite(time)) {
        std::cerr << "Model::InterpolatePosition: Invalid time value: " << time << std::endl;
        return glm::vec3(0.0f);
    }

    if (channel->mNumPositionKeys == 1) {
        aiVector3D pos = channel->mPositionKeys[0].mValue;
        glm::vec3 result(pos.x, pos.y, pos.z);

        // Validate result
        if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
            std::cerr << "Model::InterpolatePosition: Invalid position values in keyframe" << std::endl;
            return glm::vec3(0.0f);
        }

        return result;
    }

    unsigned int position_index = 0;
    for (unsigned int i = 0; i < channel->mNumPositionKeys - 1; i++) {
        if (time < channel->mPositionKeys[i + 1].mTime) {
            position_index = i;
            break;
        }
    }

    unsigned int next_position_index = position_index + 1;
    if (next_position_index >= channel->mNumPositionKeys) {
        aiVector3D pos = channel->mPositionKeys[position_index].mValue;
        glm::vec3 result(pos.x, pos.y, pos.z);

        // Validate result
        if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
            std::cerr << "Model::InterpolatePosition: Invalid position values in final keyframe" << std::endl;
            return glm::vec3(0.0f);
        }

        return result;
    }

    float delta_time = static_cast<float>(channel->mPositionKeys[next_position_index].mTime -
                                        channel->mPositionKeys[position_index].mTime);

    // Avoid division by zero
    if (delta_time <= 0.0f) {
        aiVector3D pos = channel->mPositionKeys[position_index].mValue;
        glm::vec3 result(pos.x, pos.y, pos.z);

        // Validate result
        if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
            std::cerr << "Model::InterpolatePosition: Invalid position values in keyframe" << std::endl;
            return glm::vec3(0.0f);
        }

        return result;
    }

    float factor = (time - static_cast<float>(channel->mPositionKeys[position_index].mTime)) / delta_time;
    factor = std::clamp(factor, 0.0f, 1.0f);

    aiVector3D start = channel->mPositionKeys[position_index].mValue;
    aiVector3D end = channel->mPositionKeys[next_position_index].mValue;

    glm::vec3 start_pos(start.x, start.y, start.z);
    glm::vec3 end_pos(end.x, end.y, end.z);

    // Validate input positions
    if (!std::isfinite(start_pos.x) || !std::isfinite(start_pos.y) || !std::isfinite(start_pos.z) ||
        !std::isfinite(end_pos.x) || !std::isfinite(end_pos.y) || !std::isfinite(end_pos.z)) {
        std::cerr << "Model::InterpolatePosition: Invalid position values in keyframes" << std::endl;
        return glm::vec3(0.0f);
    }

    glm::vec3 result = glm::mix(start_pos, end_pos, factor);

    // Validate result
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
        std::cerr << "Model::InterpolatePosition: Invalid interpolated position" << std::endl;
        return glm::vec3(0.0f);
    }

    return result;
}

glm::quat Model::InterpolateRotation(float time, const aiNodeAnim* channel) const {
    // Validate input parameters
    if (!channel || !channel->mRotationKeys || channel->mNumRotationKeys == 0) {
        std::cerr << "Model::InterpolateRotation: Invalid channel or no rotation keys" << std::endl;
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
    }

    // Validate time parameter
    if (!std::isfinite(time)) {
        std::cerr << "Model::InterpolateRotation: Invalid time value: " << time << std::endl;
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    if (channel->mNumRotationKeys == 1) {
        aiQuaternion rot = channel->mRotationKeys[0].mValue;
        glm::quat result(rot.w, rot.x, rot.y, rot.z);

        // Validate quaternion
        if (!std::isfinite(result.w) || !std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
            std::cerr << "Model::InterpolateRotation: Invalid quaternion values in keyframe" << std::endl;
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        // Normalize quaternion
        return glm::normalize(result);
    }

    unsigned int rotation_index = 0;
    for (unsigned int i = 0; i < channel->mNumRotationKeys - 1; i++) {
        if (time < channel->mRotationKeys[i + 1].mTime) {
            rotation_index = i;
            break;
        }
    }

    unsigned int next_rotation_index = rotation_index + 1;
    if (next_rotation_index >= channel->mNumRotationKeys) {
        aiQuaternion rot = channel->mRotationKeys[rotation_index].mValue;
        glm::quat result(rot.w, rot.x, rot.y, rot.z);

        // Validate quaternion
        if (!std::isfinite(result.w) || !std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
            std::cerr << "Model::InterpolateRotation: Invalid quaternion values in final keyframe" << std::endl;
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        return glm::normalize(result);
    }

    float delta_time = static_cast<float>(channel->mRotationKeys[next_rotation_index].mTime -
                                        channel->mRotationKeys[rotation_index].mTime);

    // Avoid division by zero
    if (delta_time <= 0.0f) {
        aiQuaternion rot = channel->mRotationKeys[rotation_index].mValue;
        glm::quat result(rot.w, rot.x, rot.y, rot.z);

        // Validate quaternion
        if (!std::isfinite(result.w) || !std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
            std::cerr << "Model::InterpolateRotation: Invalid quaternion values in keyframe" << std::endl;
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        return glm::normalize(result);
    }

    float factor = (time - static_cast<float>(channel->mRotationKeys[rotation_index].mTime)) / delta_time;
    factor = std::clamp(factor, 0.0f, 1.0f);

    aiQuaternion start = channel->mRotationKeys[rotation_index].mValue;
    aiQuaternion end = channel->mRotationKeys[next_rotation_index].mValue;

    glm::quat start_rot(start.w, start.x, start.y, start.z);
    glm::quat end_rot(end.w, end.x, end.y, end.z);

    // Validate input quaternions
    if (!std::isfinite(start_rot.w) || !std::isfinite(start_rot.x) || !std::isfinite(start_rot.y) || !std::isfinite(start_rot.z) ||
        !std::isfinite(end_rot.w) || !std::isfinite(end_rot.x) || !std::isfinite(end_rot.y) || !std::isfinite(end_rot.z)) {
        std::cerr << "Model::InterpolateRotation: Invalid quaternion values in keyframes" << std::endl;
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    // Normalize input quaternions
    start_rot = glm::normalize(start_rot);
    end_rot = glm::normalize(end_rot);

    glm::quat result = glm::slerp(start_rot, end_rot, factor);

    // Validate result
    if (!std::isfinite(result.w) || !std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
        std::cerr << "Model::InterpolateRotation: Invalid interpolated quaternion" << std::endl;
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    }

    return glm::normalize(result);
}

glm::vec3 Model::InterpolateScale(float time, const aiNodeAnim* channel) const {
    // Validate input parameters
    if (!channel || !channel->mScalingKeys || channel->mNumScalingKeys == 0) {
        std::cerr << "Model::InterpolateScale: Invalid channel or no scaling keys" << std::endl;
        return glm::vec3(1.0f); // Default scale
    }

    // Validate time parameter
    if (!std::isfinite(time)) {
        std::cerr << "Model::InterpolateScale: Invalid time value: " << time << std::endl;
        return glm::vec3(1.0f);
    }

    if (channel->mNumScalingKeys == 1) {
        aiVector3D scale = channel->mScalingKeys[0].mValue;
        glm::vec3 result(scale.x, scale.y, scale.z);

        // Validate scale values
        if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
            std::cerr << "Model::InterpolateScale: Invalid scale values in keyframe" << std::endl;
            return glm::vec3(1.0f);
        }

        // Ensure scale values are positive
        if (result.x <= 0.0f || result.y <= 0.0f || result.z <= 0.0f) {
            std::cerr << "Model::InterpolateScale: Non-positive scale values in keyframe" << std::endl;
            return glm::vec3(1.0f);
        }

        return result;
    }

    unsigned int scaling_index = 0;
    for (unsigned int i = 0; i < channel->mNumScalingKeys - 1; i++) {
        if (time < channel->mScalingKeys[i + 1].mTime) {
            scaling_index = i;
            break;
        }
    }

    unsigned int next_scaling_index = scaling_index + 1;
    if (next_scaling_index >= channel->mNumScalingKeys) {
        aiVector3D scale = channel->mScalingKeys[scaling_index].mValue;
        glm::vec3 result(scale.x, scale.y, scale.z);

        // Validate scale values
        if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
            std::cerr << "Model::InterpolateScale: Invalid scale values in final keyframe" << std::endl;
            return glm::vec3(1.0f);
        }

        // Ensure scale values are positive
        if (result.x <= 0.0f || result.y <= 0.0f || result.z <= 0.0f) {
            std::cerr << "Model::InterpolateScale: Non-positive scale values in final keyframe" << std::endl;
            return glm::vec3(1.0f);
        }

        return result;
    }

    float delta_time = static_cast<float>(channel->mScalingKeys[next_scaling_index].mTime -
                                        channel->mScalingKeys[scaling_index].mTime);

    // Avoid division by zero
    if (delta_time <= 0.0f) {
        aiVector3D scale = channel->mScalingKeys[scaling_index].mValue;
        glm::vec3 result(scale.x, scale.y, scale.z);

        // Validate scale values
        if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
            std::cerr << "Model::InterpolateScale: Invalid scale values in keyframe" << std::endl;
            return glm::vec3(1.0f);
        }

        // Ensure scale values are positive
        if (result.x <= 0.0f || result.y <= 0.0f || result.z <= 0.0f) {
            std::cerr << "Model::InterpolateScale: Non-positive scale values in keyframe" << std::endl;
            return glm::vec3(1.0f);
        }

        return result;
    }

    float factor = (time - static_cast<float>(channel->mScalingKeys[scaling_index].mTime)) / delta_time;
    factor = std::clamp(factor, 0.0f, 1.0f);

    aiVector3D start = channel->mScalingKeys[scaling_index].mValue;
    aiVector3D end = channel->mScalingKeys[next_scaling_index].mValue;

    glm::vec3 start_scale(start.x, start.y, start.z);
    glm::vec3 end_scale(end.x, end.y, end.z);

    // Validate input scales
    if (!std::isfinite(start_scale.x) || !std::isfinite(start_scale.y) || !std::isfinite(start_scale.z) ||
        !std::isfinite(end_scale.x) || !std::isfinite(end_scale.y) || !std::isfinite(end_scale.z)) {
        std::cerr << "Model::InterpolateScale: Invalid scale values in keyframes" << std::endl;
        return glm::vec3(1.0f);
    }

    // Ensure scale values are positive
    if (start_scale.x <= 0.0f || start_scale.y <= 0.0f || start_scale.z <= 0.0f ||
        end_scale.x <= 0.0f || end_scale.y <= 0.0f || end_scale.z <= 0.0f) {
        std::cerr << "Model::InterpolateScale: Non-positive scale values in keyframes" << std::endl;
        return glm::vec3(1.0f);
    }

    glm::vec3 result = glm::mix(start_scale, end_scale, factor);

    // Validate result
    if (!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z)) {
        std::cerr << "Model::InterpolateScale: Invalid interpolated scale" << std::endl;
        return glm::vec3(1.0f);
    }

    // Ensure result scale values are positive
    if (result.x <= 0.0f || result.y <= 0.0f || result.z <= 0.0f) {
        std::cerr << "Model::InterpolateScale: Non-positive interpolated scale values" << std::endl;
        return glm::vec3(1.0f);
    }

    return result;
}

// BoneAnimation implementation
glm::mat4 BoneAnimation::GetTransformAtTime(float time) const {
    if (keyframes.empty()) {
        return glm::mat4(1.0f);
    }

    if (keyframes.size() == 1) {
        const BoneKeyframe& kf = keyframes[0];
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), kf.position);
        glm::mat4 rotation = glm::mat4_cast(kf.rotation);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), kf.scale);
        return translation * rotation * scale;
    }

    // Find surrounding keyframes
    auto next_it = std::lower_bound(keyframes.begin(), keyframes.end(), time,
        [](const BoneKeyframe& kf, float t) {
            return kf.time < t;
        });

    if (next_it == keyframes.begin()) {
        // Before first keyframe
        const BoneKeyframe& kf = keyframes[0];
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), kf.position);
        glm::mat4 rotation = glm::mat4_cast(kf.rotation);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), kf.scale);
        return translation * rotation * scale;
    }

    if (next_it == keyframes.end()) {
        // After last keyframe
        const BoneKeyframe& kf = keyframes.back();
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), kf.position);
        glm::mat4 rotation = glm::mat4_cast(kf.rotation);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), kf.scale);
        return translation * rotation * scale;
    }

    // Interpolate between keyframes
    auto prev_it = next_it - 1;
    const BoneKeyframe& kf1 = *prev_it;
    const BoneKeyframe& kf2 = *next_it;

    float delta_time = kf2.time - kf1.time;
    float factor = (delta_time > 0.0f) ? ((time - kf1.time) / delta_time) : 0.0f;
    factor = std::clamp(factor, 0.0f, 1.0f);

    // Interpolate position, rotation, and scale
    glm::vec3 position = glm::mix(kf1.position, kf2.position, factor);
    glm::quat rotation = glm::slerp(kf1.rotation, kf2.rotation, factor);
    glm::vec3 scale = glm::mix(kf1.scale, kf2.scale, factor);

    // Build transformation matrix
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotation_mat = glm::mat4_cast(rotation);
    glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), scale);

    return translation * rotation_mat * scale_mat;
}

} // namespace Lupine
