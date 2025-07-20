#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace Lupine {

/**
 * @brief Terrain texture blend data for a single point
 */
struct TerrainTextureBlend {
    std::vector<float> layer_weights;  // Weight for each texture layer (0.0 to 1.0)
    
    TerrainTextureBlend() = default;
    explicit TerrainTextureBlend(size_t layer_count) : layer_weights(layer_count, 0.0f) {}
    
    void SetLayerWeight(size_t layer_index, float weight) {
        if (layer_index < layer_weights.size()) {
            layer_weights[layer_index] = glm::clamp(weight, 0.0f, 1.0f);
        }
    }
    
    float GetLayerWeight(size_t layer_index) const {
        return (layer_index < layer_weights.size()) ? layer_weights[layer_index] : 0.0f;
    }
    
    void NormalizeWeights() {
        float total = 0.0f;
        for (float weight : layer_weights) {
            total += weight;
        }
        if (total > 0.0f) {
            for (float& weight : layer_weights) {
                weight /= total;
            }
        }
    }
};

/**
 * @brief Terrain asset instance data
 */
struct TerrainAsset {
    std::string asset_path;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
    float height_offset;
    bool visible;
    uint32_t asset_id;  // For efficient lookup
    
    TerrainAsset() 
        : position(0.0f), rotation(0.0f), scale(1.0f), height_offset(0.0f), visible(true), asset_id(0) {}
    
    TerrainAsset(const std::string& path, const glm::vec3& pos, const glm::vec3& rot = glm::vec3(0.0f), 
                const glm::vec3& scl = glm::vec3(1.0f), float offset = 0.0f)
        : asset_path(path), position(pos), rotation(rot), scale(scl), height_offset(offset), visible(true), asset_id(0) {}
};

/**
 * @brief Terrain chunk data for efficient streaming and LOD
 */
class TerrainChunk {
public:
    /**
     * @brief Constructor
     * @param chunk_x Chunk X coordinate
     * @param chunk_z Chunk Z coordinate
     * @param chunk_size Size of chunk in world units
     * @param resolution Height samples per world unit
     */
    TerrainChunk(int chunk_x, int chunk_z, float chunk_size, float resolution);
    
    /**
     * @brief Destructor
     */
    ~TerrainChunk() = default;
    
    // === Chunk Properties ===
    
    /**
     * @brief Get chunk coordinates
     * @return Chunk coordinates (x, z)
     */
    glm::ivec2 GetChunkCoords() const { return glm::ivec2(m_chunk_x, m_chunk_z); }
    
    /**
     * @brief Get chunk size in world units
     * @return Chunk size
     */
    float GetChunkSize() const { return m_chunk_size; }
    
    /**
     * @brief Get height resolution
     * @return Height samples per world unit
     */
    float GetResolution() const { return m_resolution; }
    
    /**
     * @brief Get height map dimensions
     * @return Height map dimensions (width, height)
     */
    glm::ivec2 GetHeightMapSize() const { return m_heightmap_size; }
    
    /**
     * @brief Get chunk world bounds
     * @return World bounds (min, max)
     */
    std::pair<glm::vec3, glm::vec3> GetWorldBounds() const;
    
    // === Height Data ===
    
    /**
     * @brief Set height at local coordinates
     * @param x Local X coordinate (0 to heightmap_width-1)
     * @param z Local Z coordinate (0 to heightmap_height-1)
     * @param height Height value
     */
    void SetHeight(int x, int z, float height);
    
    /**
     * @brief Get height at local coordinates
     * @param x Local X coordinate
     * @param z Local Z coordinate
     * @return Height value
     */
    float GetHeight(int x, int z) const;
    
    /**
     * @brief Get interpolated height at local floating-point coordinates
     * @param x Local X coordinate (floating-point)
     * @param z Local Z coordinate (floating-point)
     * @return Interpolated height value
     */
    float GetHeightInterpolated(float x, float z) const;
    
    /**
     * @brief Get height data array
     * @return Reference to height data
     */
    const std::vector<float>& GetHeightData() const { return m_height_data; }
    
    /**
     * @brief Set height data array
     * @param height_data New height data
     */
    void SetHeightData(const std::vector<float>& height_data);
    
    // === Texture Blending ===
    
    /**
     * @brief Set texture blend data at local coordinates
     * @param x Local X coordinate
     * @param z Local Z coordinate
     * @param blend_data Texture blend data
     */
    void SetTextureBlend(int x, int z, const TerrainTextureBlend& blend_data);
    
    /**
     * @brief Get texture blend data at local coordinates
     * @param x Local X coordinate
     * @param z Local Z coordinate
     * @return Texture blend data
     */
    const TerrainTextureBlend& GetTextureBlend(int x, int z) const;
    
    /**
     * @brief Get interpolated texture blend at local floating-point coordinates
     * @param x Local X coordinate (floating-point)
     * @param z Local Z coordinate (floating-point)
     * @return Interpolated texture blend data
     */
    TerrainTextureBlend GetTextureBlendInterpolated(float x, float z) const;
    
    // === Asset Management ===
    
    /**
     * @brief Add asset to chunk
     * @param asset Asset data
     */
    void AddAsset(const TerrainAsset& asset);
    
    /**
     * @brief Remove asset from chunk
     * @param asset_id Asset ID to remove
     */
    void RemoveAsset(uint32_t asset_id);
    
    /**
     * @brief Get all assets in chunk
     * @return Vector of assets
     */
    const std::vector<TerrainAsset>& GetAssets() const { return m_assets; }
    
    /**
     * @brief Clear all assets
     */
    void ClearAssets() { m_assets.clear(); SetDirty(true); }
    
    // === Mesh Generation ===
    
    /**
     * @brief Generate mesh vertices for rendering
     * @param vertices Output vertex data
     * @param indices Output index data
     * @param include_normals Include normal vectors
     * @param include_uvs Include UV coordinates
     */
    void GenerateMesh(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices,
                     bool include_normals = true, bool include_uvs = true) const;
    
    /**
     * @brief Calculate normal at local coordinates
     * @param x Local X coordinate
     * @param z Local Z coordinate
     * @return Surface normal
     */
    glm::vec3 CalculateNormal(int x, int z) const;
    
    // === State Management ===
    
    /**
     * @brief Check if chunk data has been modified
     * @return True if chunk is dirty
     */
    bool IsDirty() const { return m_is_dirty; }
    
    /**
     * @brief Set dirty state
     * @param dirty Dirty state
     */
    void SetDirty(bool dirty) { m_is_dirty = dirty; }
    
    /**
     * @brief Check if chunk is loaded
     * @return True if chunk is loaded
     */
    bool IsLoaded() const { return m_is_loaded; }
    
    /**
     * @brief Set loaded state
     * @param loaded Loaded state
     */
    void SetLoaded(bool loaded) { m_is_loaded = loaded; }

private:
    // Chunk identification
    int m_chunk_x;
    int m_chunk_z;
    float m_chunk_size;
    float m_resolution;
    glm::ivec2 m_heightmap_size;
    
    // Height data
    std::vector<float> m_height_data;
    
    // Texture blending data
    std::vector<TerrainTextureBlend> m_texture_blend_data;
    
    // Asset data
    std::vector<TerrainAsset> m_assets;
    uint32_t m_next_asset_id;
    
    // State
    bool m_is_dirty;
    bool m_is_loaded;
    
    // Helper methods
    int GetHeightIndex(int x, int z) const;
    bool IsValidCoordinate(int x, int z) const;
    glm::vec3 LocalToWorldPosition(int x, int z, float height) const;
};

/**
 * @brief Main terrain data container
 */
class TerrainData {
public:
    /**
     * @brief Constructor
     * @param width Terrain width in world units
     * @param height Terrain height in world units
     * @param resolution Height samples per world unit
     * @param chunk_size Size of each chunk in world units
     */
    TerrainData(float width, float height, float resolution = 1.0f, float chunk_size = 64.0f);
    
    /**
     * @brief Destructor
     */
    ~TerrainData() = default;
    
    // === Terrain Properties ===
    
    /**
     * @brief Get terrain dimensions
     * @return Terrain dimensions (width, height)
     */
    glm::vec2 GetDimensions() const { return glm::vec2(m_width, m_height); }
    
    /**
     * @brief Get terrain resolution
     * @return Height samples per world unit
     */
    float GetResolution() const { return m_resolution; }
    
    /**
     * @brief Get chunk size
     * @return Chunk size in world units
     */
    float GetChunkSize() const { return m_chunk_size; }
    
    /**
     * @brief Get chunk grid dimensions
     * @return Chunk grid dimensions (width, height)
     */
    glm::ivec2 GetChunkGridSize() const { return m_chunk_grid_size; }
    
    /**
     * @brief Get terrain bounds
     * @return Terrain bounds (min, max)
     */
    std::pair<glm::vec3, glm::vec3> GetBounds() const;
    
    // === Chunk Management ===
    
    /**
     * @brief Get chunk at chunk coordinates
     * @param chunk_x Chunk X coordinate
     * @param chunk_z Chunk Z coordinate
     * @return Pointer to chunk, or nullptr if not found
     */
    TerrainChunk* GetChunk(int chunk_x, int chunk_z);
    
    /**
     * @brief Get chunk at world position
     * @param world_pos World position
     * @return Pointer to chunk, or nullptr if outside bounds
     */
    TerrainChunk* GetChunkAtWorldPos(const glm::vec3& world_pos);
    
    /**
     * @brief Get all chunks
     * @return Vector of chunk pointers
     */
    std::vector<TerrainChunk*> GetAllChunks();
    
    /**
     * @brief Create chunk if it doesn't exist
     * @param chunk_x Chunk X coordinate
     * @param chunk_z Chunk Z coordinate
     * @return Pointer to chunk
     */
    TerrainChunk* CreateChunk(int chunk_x, int chunk_z);

    /**
     * @brief Initialize flat terrain with specified height
     * @param height Height for the flat terrain (default: 0.0f)
     * @param chunk_count_x Number of chunks in X direction (default: 4)
     * @param chunk_count_z Number of chunks in Z direction (default: 4)
     */
    void InitializeFlatTerrain(float height = 0.0f, int chunk_count_x = 4, int chunk_count_z = 4);
    
    // === Height Operations ===
    
    /**
     * @brief Get height at world position
     * @param world_pos World position
     * @return Height value, or NaN if outside bounds
     */
    float GetHeightAtWorldPos(const glm::vec3& world_pos) const;
    
    /**
     * @brief Set height at world position
     * @param world_pos World position
     * @param height Height value
     */
    void SetHeightAtWorldPos(const glm::vec3& world_pos, float height);
    
    /**
     * @brief Get normal at world position
     * @param world_pos World position
     * @return Surface normal
     */
    glm::vec3 GetNormalAtWorldPos(const glm::vec3& world_pos) const;
    
    // === Coordinate Conversion ===
    
    /**
     * @brief Convert world position to chunk coordinates
     * @param world_pos World position
     * @return Chunk coordinates
     */
    glm::ivec2 WorldToChunkCoords(const glm::vec3& world_pos) const;
    
    /**
     * @brief Convert world position to local chunk coordinates
     * @param world_pos World position
     * @return Local coordinates within chunk
     */
    glm::vec2 WorldToLocalChunkCoords(const glm::vec3& world_pos) const;
    
    /**
     * @brief Check if world position is within terrain bounds
     * @param world_pos World position
     * @return True if within bounds
     */
    bool IsWithinBounds(const glm::vec3& world_pos) const;

private:
    // Terrain properties
    float m_width;
    float m_height;
    float m_resolution;
    float m_chunk_size;
    glm::ivec2 m_chunk_grid_size;
    
    // Chunk storage
    std::unordered_map<uint64_t, std::unique_ptr<TerrainChunk>> m_chunks;
    
    // Helper methods
    uint64_t GetChunkKey(int chunk_x, int chunk_z) const;
    glm::ivec2 KeyToChunkCoords(uint64_t key) const;
};

} // namespace Lupine
