#pragma once

#include "lupine/core/Component.h"
#include "lupine/resources/TilesetResource.h"
#include "lupine/components/Tilemap2D.h"  // For TilemapProject and advanced structures
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Lupine {

class Node3D;

/**
 * @brief 2.5D tilemap component for rendering tile-based maps in 3D space
 * 
 * Tilemap2.5D component renders a grid of 2D tiles in 3D space using a tileset resource.
 * It's based on Sprite3D and should be attached to Node3D nodes.
 * Uses the same tilemap painter system as Tilemap2D but renders in 3D space.
 */
class Tilemap25D : public Component {
public:
    /**
     * @brief Billboard mode options for tiles
     */
    enum class BillboardMode {
        Disabled,       // No billboard behavior
        Enabled,        // Always face camera
        YBillboard,     // Only rotate around Y axis
        ParticlesBillboard  // Special mode for particles
    };

    /**
     * @brief Constructor
     */
    Tilemap25D();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Tilemap25D();
    
    // Tileset management
    const std::string& GetTilesetPath() const { return m_tileset_path; }
    void SetTilesetPath(const std::string& path);
    
    // Tilemap data
    const TilemapData& GetTilemapData() const { return m_tilemap_data; }
    void SetTilemapData(const TilemapData& data);
    
    // Map size
    const glm::ivec2& GetMapSize() const { return m_tilemap_data.size; }
    void SetMapSize(const glm::ivec2& size);
    
    // Tile size (in world units)
    const glm::vec2& GetTileSize() const { return m_tile_size; }
    void SetTileSize(const glm::vec2& size);
    
    // Individual tile operations
    int GetTile(int x, int y) const { return m_tilemap_data.GetTile(x, y); }
    void SetTile(int x, int y, int tile_id);
    
    // Rendering properties
    const glm::vec4& GetModulate() const { return m_modulate; }
    void SetModulate(const glm::vec4& modulate);
    
    BillboardMode GetBillboardMode() const { return m_billboard_mode; }
    void SetBillboardMode(BillboardMode mode);
    
    bool GetTransparent() const { return m_transparent; }
    void SetTransparent(bool transparent);
    
    bool GetDoubleSided() const { return m_double_sided; }
    void SetDoubleSided(bool double_sided);
    
    bool GetReceivesLighting() const { return m_receives_lighting; }
    void SetReceivesLighting(bool receives_lighting);
    
    // Grid display
    bool GetShowGrid() const { return m_show_grid; }
    void SetShowGrid(bool show_grid);
    
    const glm::vec4& GetGridColor() const { return m_grid_color; }
    void SetGridColor(const glm::vec4& color);
    
    // Collision
    bool GetCollisionEnabled() const { return m_collision_enabled; }
    void SetCollisionEnabled(bool enabled);
    
    // Utility functions
    glm::vec3 MapToLocal(const glm::ivec2& map_pos) const;
    glm::ivec2 LocalToMap(const glm::vec3& local_pos) const;
    glm::vec3 GetTileWorldPosition(int x, int y) const;
    
    // Tilemap operations
    void ClearTilemap();
    void FillTilemap(int tile_id);
    void FloodFill(int x, int y, int tile_id);
    
    // Serialization helpers
    std::string SerializeTilemapData() const;
    bool DeserializeTilemapData(const std::string& data);
    
    // Component interface
    std::string GetTypeName() const override { return "Tilemap2.5D"; }
    std::string GetCategory() const override { return "3D"; }
    
    // Lifecycle methods
    void OnReady() override;
    void OnUpdate(float delta_time) override;

protected:
    void InitializeExportVariables() override;

private:
    // Tileset resource
    std::string m_tileset_path;
    std::unique_ptr<Tileset2DResource> m_tileset_resource;
    bool m_tileset_loaded;
    
    // Tilemap data
    TilemapData m_tilemap_data;
    std::string m_tilemap_data_serialized;  // For export variables
    
    // Tile properties
    glm::vec2 m_tile_size;
    
    // Rendering properties
    glm::vec4 m_modulate;
    BillboardMode m_billboard_mode;
    bool m_transparent;
    bool m_double_sided;
    bool m_receives_lighting;
    
    // Grid display
    bool m_show_grid;
    glm::vec4 m_grid_color;
    
    // Collision
    bool m_collision_enabled;
    
    // Mesh data for each tile
    struct TileMesh {
        unsigned int vertex_array_object;
        unsigned int vertex_buffer_object;
        unsigned int element_buffer_object;
        bool initialized;
        
        TileMesh() : vertex_array_object(0), vertex_buffer_object(0), 
                     element_buffer_object(0), initialized(false) {}
    };
    
    std::vector<TileMesh> m_tile_meshes;
    bool m_meshes_dirty;
    
    // Internal methods
    void LoadTileset();
    void UpdateFromExportVariables();
    void UpdateExportVariables();
    void RenderTilemap();
    void RenderGrid();
    void RenderTile(int x, int y, int tile_id, Node3D* node3d);
    void InitializeTileMeshes();
    void UpdateTileMesh(int tile_index);
    void CleanupMeshes();
    glm::mat4 CalculateTileTransform(int x, int y, Node3D* node3d) const;
    glm::mat4 CalculateBillboardTransform(const glm::mat4& tile_transform) const;
    glm::vec4 GetTileTextureRegion(int tile_id) const;
    
    // Optimization
    void UpdateVisibleTiles();
    glm::ivec4 m_visible_tile_bounds;  // min_x, min_y, max_x, max_y
    bool m_bounds_dirty;
};

} // namespace Lupine
