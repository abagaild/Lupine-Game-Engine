#pragma once

#include "lupine/core/Component.h"
#include "lupine/resources/Tileset3DResource.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Lupine {

class Node3D;

/**
 * @brief 3D tilemap data structure for storing 3D tile arrangements
 */
struct Tilemap3DData {
    glm::ivec3 size = glm::ivec3(10, 1, 10);        // Map size in tiles (width, height, depth)
    std::vector<int> tiles;                         // Tile IDs (-1 = empty)
    
    Tilemap3DData() {
        tiles.resize(size.x * size.y * size.z, -1);
    }
    
    Tilemap3DData(const glm::ivec3& map_size) : size(map_size) {
        tiles.resize(size.x * size.y * size.z, -1);
    }
    
    // Get/Set tile at position
    int GetTile(int x, int y, int z) const {
        if (x < 0 || x >= size.x || y < 0 || y >= size.y || z < 0 || z >= size.z) return -1;
        return tiles[z * size.x * size.y + y * size.x + x];
    }
    
    void SetTile(int x, int y, int z, int tile_id) {
        if (x < 0 || x >= size.x || y < 0 || y >= size.y || z < 0 || z >= size.z) return;
        tiles[z * size.x * size.y + y * size.x + x] = tile_id;
    }
    
    // Resize the tilemap
    void Resize(const glm::ivec3& new_size) {
        std::vector<int> new_tiles(new_size.x * new_size.y * new_size.z, -1);
        
        // Copy existing tiles
        for (int z = 0; z < std::min(size.z, new_size.z); ++z) {
            for (int y = 0; y < std::min(size.y, new_size.y); ++y) {
                for (int x = 0; x < std::min(size.x, new_size.x); ++x) {
                    new_tiles[z * new_size.x * new_size.y + y * new_size.x + x] = GetTile(x, y, z);
                }
            }
        }
        
        size = new_size;
        tiles = std::move(new_tiles);
    }
    
    // Clear all tiles
    void Clear() {
        std::fill(tiles.begin(), tiles.end(), -1);
    }
};

/**
 * @brief 3D tilemap component for rendering collections of 3D meshes
 * 
 * Tilemap3D component renders a grid of 3D meshes using a Tileset3D resource.
 * It should be attached to Node3D nodes and provides frustum culling for performance.
 * Uses 3D tile objects from Tileset3D resources arranged in a 3D grid.
 */
class Tilemap3D : public Component {
public:
    /**
     * @brief Constructor
     */
    Tilemap3D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Tilemap3D() = default;
    
    // Tileset management
    const std::string& GetTilesetPath() const { return m_tileset_path; }
    void SetTilesetPath(const std::string& path);
    
    // Tilemap data
    const Tilemap3DData& GetTilemapData() const { return m_tilemap_data; }
    void SetTilemapData(const Tilemap3DData& data);
    
    // Map size
    const glm::ivec3& GetMapSize() const { return m_tilemap_data.size; }
    void SetMapSize(const glm::ivec3& size);
    
    // Tile size (in world units)
    const glm::vec3& GetTileSize() const { return m_tile_size; }
    void SetTileSize(const glm::vec3& size);
    
    // Individual tile operations
    int GetTile(int x, int y, int z) const { return m_tilemap_data.GetTile(x, y, z); }
    void SetTile(int x, int y, int z, int tile_id);
    
    // Rendering properties
    const glm::vec4& GetModulate() const { return m_modulate; }
    void SetModulate(const glm::vec4& modulate);
    
    bool GetCastShadows() const { return m_cast_shadows; }
    void SetCastShadows(bool cast_shadows);
    
    bool GetReceiveShadows() const { return m_receive_shadows; }
    void SetReceiveShadows(bool receive_shadows);
    
    // Frustum culling
    bool GetFrustumCullingEnabled() const { return m_frustum_culling_enabled; }
    void SetFrustumCullingEnabled(bool enabled);
    
    float GetCullingDistance() const { return m_culling_distance; }
    void SetCullingDistance(float distance);
    
    // LOD (Level of Detail)
    bool GetLODEnabled() const { return m_lod_enabled; }
    void SetLODEnabled(bool enabled);
    
    float GetLODDistance() const { return m_lod_distance; }
    void SetLODDistance(float distance);
    
    // Collision
    bool GetCollisionEnabled() const { return m_collision_enabled; }
    void SetCollisionEnabled(bool enabled);
    
    // Utility functions
    glm::vec3 MapToLocal(const glm::ivec3& map_pos) const;
    glm::ivec3 LocalToMap(const glm::vec3& local_pos) const;
    glm::vec3 GetTileWorldPosition(int x, int y, int z) const;
    
    // Tilemap operations
    void ClearTilemap();
    void FillTilemap(int tile_id);
    void FillLayer(int layer, int tile_id);
    void FloodFill3D(int x, int y, int z, int tile_id);
    
    // Serialization helpers
    std::string SerializeTilemapData() const;
    bool DeserializeTilemapData(const std::string& data);
    
    // Performance statistics
    int GetVisibleTileCount() const { return m_visible_tile_count; }
    int GetCulledTileCount() const { return m_culled_tile_count; }
    
    // Component interface
    std::string GetTypeName() const override { return "Tilemap3D"; }
    std::string GetCategory() const override { return "3D"; }
    
    // Lifecycle methods
    void OnReady() override;
    void OnUpdate(float delta_time) override;

protected:
    void InitializeExportVariables() override;

private:
    // Tileset resource
    std::string m_tileset_path;
    std::unique_ptr<Tileset3DResource> m_tileset_resource;
    bool m_tileset_loaded;
    
    // Tilemap data
    Tilemap3DData m_tilemap_data;
    std::string m_tilemap_data_serialized;  // For export variables
    
    // Tile properties
    glm::vec3 m_tile_size;
    
    // Rendering properties
    glm::vec4 m_modulate;
    bool m_cast_shadows;
    bool m_receive_shadows;
    
    // Culling and optimization
    bool m_frustum_culling_enabled;
    float m_culling_distance;
    bool m_lod_enabled;
    float m_lod_distance;
    
    // Collision
    bool m_collision_enabled;
    
    // Performance tracking
    mutable int m_visible_tile_count;
    mutable int m_culled_tile_count;
    
    // Internal methods
    void LoadTileset();
    void UpdateFromExportVariables();
    void UpdateExportVariables();
    void RenderTilemap();
    void RenderTile(int x, int y, int z, int tile_id, Node3D* node3d);
    glm::mat4 CalculateTileTransform(int x, int y, int z, Node3D* node3d) const;
    
    // Culling and optimization
    void UpdateVisibleTiles();
    bool IsTileVisible(int x, int y, int z, const glm::vec3& camera_pos) const;
    bool IsTileInFrustum(const glm::vec3& tile_world_pos) const;
    float GetDistanceToCamera(const glm::vec3& tile_world_pos) const;
    
    // Visible tile bounds for optimization
    glm::ivec3 m_visible_tile_min;
    glm::ivec3 m_visible_tile_max;
    bool m_bounds_dirty;
};

} // namespace Lupine
