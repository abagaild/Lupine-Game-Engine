#include "lupine/terrain/AssetScatterBrush.h"
#include "lupine/terrain/TerrainData.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/components/StaticMesh.h"
#include "lupine/resources/ResourceManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lupine {

// AssetScatterBrush Implementation

AssetScatterBrush::AssetScatterBrush()
    : m_surface_target(nullptr)
    , m_stroke_active(false)
    , m_current_terrain(nullptr)
    , m_current_target_node(nullptr)
    , m_last_position(0.0f)
    , m_accumulated_distance(0.0f)
    , m_random_generator(std::random_device{}())
    , m_uniform_dist(0.0f, 1.0f)
{
    // Initialize with default brush settings
    m_brush_settings.size = 10.0f;
    m_brush_settings.strength = 1.0f;
    m_brush_settings.falloff = BrushFalloff::Smooth;
    m_brush_settings.falloff_curve = 0.5f;
    m_brush_settings.spacing = 0.5f;
}

void AssetScatterBrush::SetBrushSettings(const BrushSettings& settings) {
    m_brush_settings = settings;
}

void AssetScatterBrush::SetScatterParams(const AssetScatterParams& params) {
    m_scatter_params = params;
}

int AssetScatterBrush::AddAsset(const ScatterAssetInfo& asset_info) {
    m_asset_palette.push_back(asset_info);
    return static_cast<int>(m_asset_palette.size() - 1);
}

void AssetScatterBrush::RemoveAsset(int asset_index) {
    if (asset_index >= 0 && asset_index < static_cast<int>(m_asset_palette.size())) {
        m_asset_palette.erase(m_asset_palette.begin() + asset_index);
    }
}

const ScatterAssetInfo& AssetScatterBrush::GetAsset(int asset_index) const {
    static ScatterAssetInfo empty_asset;
    if (asset_index >= 0 && asset_index < static_cast<int>(m_asset_palette.size())) {
        return m_asset_palette[asset_index];
    }
    return empty_asset;
}

void AssetScatterBrush::UpdateAsset(int asset_index, const ScatterAssetInfo& asset_info) {
    if (asset_index >= 0 && asset_index < static_cast<int>(m_asset_palette.size())) {
        m_asset_palette[asset_index] = asset_info;
    }
}

void AssetScatterBrush::StartStroke(const glm::vec3& world_pos, TerrainData* terrain_data, Node3D* target_node) {
    if (!terrain_data || m_asset_palette.empty()) return;
    
    m_stroke_active = true;
    m_current_terrain = terrain_data;
    m_current_target_node = target_node;
    m_last_position = world_pos;
    m_accumulated_distance = 0.0f;
    
    // Initialize stroke data
    m_current_stroke = AssetScatterStroke();
    m_current_stroke.brush_settings = m_brush_settings;
    m_current_stroke.scatter_params = m_scatter_params;
    m_current_stroke.asset_palette = m_asset_palette;
    m_current_stroke.positions.push_back(world_pos);
    
    // Apply initial scatter dab
    ApplyScatterDab(world_pos, terrain_data, target_node, 1.0f);
}

void AssetScatterBrush::ContinueStroke(const glm::vec3& world_pos, float /*delta_time*/) {
    if (!m_stroke_active || !m_current_terrain) return;
    
    // Calculate distance from last position
    float distance = glm::length(world_pos - m_last_position);
    m_accumulated_distance += distance;
    
    // Check if we should apply brush based on spacing
    float spacing_distance = m_brush_settings.size * m_brush_settings.spacing;
    
    if (m_accumulated_distance >= spacing_distance) {
        // Apply scatter dab
        ApplyScatterDab(world_pos, m_current_terrain, m_current_target_node, 1.0f);
        
        // Record position
        m_current_stroke.positions.push_back(world_pos);
        
        // Reset accumulated distance
        m_accumulated_distance = 0.0f;
    }
    
    m_last_position = world_pos;
}

void AssetScatterBrush::EndStroke() {
    if (!m_stroke_active) return;
    
    m_stroke_active = false;
    
    // Notify callback if set
    if (m_stroke_callback) {
        m_stroke_callback(m_current_stroke);
    }
    
    m_current_terrain = nullptr;
    m_current_target_node = nullptr;
}

void AssetScatterBrush::ApplyScatterDab(const glm::vec3& world_pos, TerrainData* terrain_data, 
                                       Node3D* target_node, float strength_multiplier) {
    if (!terrain_data || m_asset_palette.empty()) return;
    
    // Calculate effective radius and density
    float effective_radius = m_brush_settings.size;
    float effective_density = m_scatter_params.density * m_brush_settings.strength * strength_multiplier;
    
    // Place assets in the brush area
    PlaceAssetsInArea(world_pos, effective_radius, terrain_data, target_node, strength_multiplier);
}

void AssetScatterBrush::EraseAssets(const glm::vec3& world_pos, TerrainData* terrain_data, 
                                   Node3D* target_node, float radius) {
    if (!target_node) return;
    
    // Find and remove assets within radius
    std::vector<Node3D*> assets_to_remove;
    
    // Iterate through child nodes to find assets within radius
    const auto& children = target_node->GetChildren();
    for (const auto& child : children) {
        if (auto* node3d = dynamic_cast<Node3D*>(child.get())) {
            glm::vec3 asset_pos = node3d->GetGlobalPosition();
            float distance = glm::length(glm::vec2(asset_pos.x - world_pos.x, asset_pos.z - world_pos.z));

            if (distance <= radius) {
                assets_to_remove.push_back(node3d);
            }
        }
    }

    // Remove the assets
    for (auto* asset : assets_to_remove) {
        target_node->RemoveChild(asset->GetUUID());
        // Note: In a full implementation, we'd also record this for undo
    }
    
    std::cout << "AssetScatterBrush: Erased " << assets_to_remove.size() << " assets" << std::endl;
}

void AssetScatterBrush::SetSurfaceSnappingTarget(Node3D* target_mesh) {
    m_surface_target = target_mesh;
}

bool AssetScatterBrush::CalculateSurfaceSnapping(const glm::vec3& world_pos, TerrainData* terrain_data,
                                                 glm::vec3& surface_pos, glm::vec3& surface_normal) const {
    switch (m_scatter_params.snapping_mode) {
        case SurfaceSnappingMode::TerrainNormals:
            if (terrain_data) {
                surface_pos = world_pos;
                surface_pos.y = terrain_data->GetHeightAtWorldPos(world_pos);
                surface_normal = terrain_data->GetNormalAtWorldPos(world_pos);
                return true;
            }
            break;
            
        case SurfaceSnappingMode::TerrainFlat:
            if (terrain_data) {
                surface_pos = world_pos;
                surface_pos.y = terrain_data->GetHeightAtWorldPos(world_pos);
                surface_normal = glm::vec3(0.0f, 1.0f, 0.0f);
                return true;
            }
            break;
            
        case SurfaceSnappingMode::MeshSurface:
            // Mesh surface snapping would require ray casting against the target mesh
            // For now, fall back to terrain snapping
            if (terrain_data) {
                surface_pos = world_pos;
                surface_pos.y = terrain_data->GetHeightAtWorldPos(world_pos);
                surface_normal = glm::vec3(0.0f, 1.0f, 0.0f);
                return true;
            }
            break;
            
        case SurfaceSnappingMode::WorldUp:
            surface_pos = world_pos;
            surface_normal = glm::vec3(0.0f, 1.0f, 0.0f);
            return true;
            
        case SurfaceSnappingMode::Custom:
            surface_pos = world_pos;
            surface_normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default to up
            return true;
    }
    
    return false;
}

void AssetScatterBrush::SetStrokeCallback(std::function<void(const AssetScatterStroke&)> callback) {
    m_stroke_callback = callback;
}

void AssetScatterBrush::PlaceAssetsInArea(const glm::vec3& center_pos, float radius, TerrainData* terrain_data, 
                                         Node3D* target_node, float strength_multiplier) {
    if (!target_node) return;
    
    // Calculate number of assets to place based on density and area
    float area = M_PI * radius * radius;
    int target_count = static_cast<int>(area * m_scatter_params.density * strength_multiplier);
    target_count = std::max(1, target_count); // At least one asset
    
    // Generate scatter positions
    std::vector<glm::vec3> positions = GenerateScatterPositions(center_pos, radius, target_count);
    
    // Place assets at valid positions
    std::vector<glm::vec3> placed_positions;
    for (const glm::vec3& pos : positions) {
        if (IsValidPlacementPosition(pos, terrain_data, placed_positions, m_scatter_params.min_distance)) {
            // Calculate surface snapping
            glm::vec3 surface_pos, surface_normal;
            if (CalculateSurfaceSnapping(pos, terrain_data, surface_pos, surface_normal)) {
                // Check slope constraints
                if (CheckSlope(surface_normal) && CheckWaterLevel(surface_pos)) {
                    // Select random asset
                    const ScatterAssetInfo& asset_info = SelectRandomAsset();
                    
                    // Calculate transformations
                    glm::vec3 rotation = CalculateAssetRotation(surface_normal, asset_info);
                    glm::vec3 scale = CalculateAssetScale(asset_info);
                    
                    // Apply height offset
                    float height_offset = m_uniform_dist(m_random_generator) * 
                                         (m_scatter_params.height_offset_range.y - m_scatter_params.height_offset_range.x) + 
                                         m_scatter_params.height_offset_range.x;
                    surface_pos.y += height_offset;
                    
                    // Create asset instance
                    Node3D* asset_instance = CreateAssetInstance(asset_info, surface_pos, rotation, scale, target_node);
                    if (asset_instance) {
                        placed_positions.push_back(surface_pos);
                        
                        // Record asset ID for undo
                        if (m_stroke_active) {
                            m_current_stroke.created_asset_ids.push_back(GenerateAssetId());
                        }
                    }
                }
            }
        }
    }
    
    std::cout << "AssetScatterBrush: Placed " << placed_positions.size() << " assets out of " << target_count << " attempts" << std::endl;
}

std::vector<glm::vec3> AssetScatterBrush::GenerateScatterPositions(const glm::vec3& center_pos, float radius, 
                                                                   int target_count) const {
    switch (m_scatter_params.pattern) {
        case ScatterPattern::Random:
            return GenerateRandomPattern(center_pos, radius, target_count);
        case ScatterPattern::Grid:
            return GenerateGridPattern(center_pos, radius, target_count);
        case ScatterPattern::Poisson:
            return GeneratePoissonPattern(center_pos, radius, target_count);
        case ScatterPattern::Cluster:
            return GenerateClusterPattern(center_pos, radius, target_count);
        case ScatterPattern::Custom:
            return GenerateRandomPattern(center_pos, radius, target_count); // Default to random
        default:
            return GenerateRandomPattern(center_pos, radius, target_count);
    }
}

bool AssetScatterBrush::IsValidPlacementPosition(const glm::vec3& world_pos, TerrainData* terrain_data,
                                                 const std::vector<glm::vec3>& existing_positions, 
                                                 float min_distance) const {
    // Check terrain bounds
    if (terrain_data && !terrain_data->IsWithinBounds(world_pos)) {
        return false;
    }
    
    // Check minimum distance to existing positions
    for (const glm::vec3& existing_pos : existing_positions) {
        float distance = glm::length(glm::vec2(world_pos.x - existing_pos.x, world_pos.z - existing_pos.z));
        if (distance < min_distance) {
            return false;
        }
    }
    
    return true;
}

const ScatterAssetInfo& AssetScatterBrush::SelectRandomAsset() const {
    if (m_asset_palette.empty()) {
        static ScatterAssetInfo empty_asset;
        return empty_asset;
    }
    
    // Calculate total weight
    float total_weight = 0.0f;
    for (const auto& asset : m_asset_palette) {
        if (asset.enabled) {
            total_weight += asset.weight;
        }
    }
    
    if (total_weight <= 0.0f) {
        return m_asset_palette[0];
    }
    
    // Select based on weight
    float random_value = m_uniform_dist(m_random_generator) * total_weight;
    float current_weight = 0.0f;
    
    for (const auto& asset : m_asset_palette) {
        if (asset.enabled) {
            current_weight += asset.weight;
            if (random_value <= current_weight) {
                return asset;
            }
        }
    }
    
    return m_asset_palette[0];
}

Node3D* AssetScatterBrush::CreateAssetInstance(const ScatterAssetInfo& asset_info, const glm::vec3& position,
                                              const glm::vec3& rotation, const glm::vec3& scale, 
                                              Node3D* parent_node) {
    if (!parent_node || asset_info.asset_path.empty()) return nullptr;
    
    // Create new node for the asset
    Node3D* asset_node = parent_node->CreateChild<Node3D>("ScatteredAsset");
    if (!asset_node) return nullptr;
    
    // Set transformation
    asset_node->SetPosition(position + asset_info.pivot_offset);
    asset_node->SetRotation(rotation);
    asset_node->SetScale(scale);
    
    // Add static mesh component
    auto* static_mesh = asset_node->AddComponent<StaticMesh>();
    if (static_mesh) {
        // Load mesh resource
        // In a full implementation, this would use the ResourceManager to load the mesh
        std::cout << "AssetScatterBrush: Creating asset instance from " << asset_info.asset_path << std::endl;
    }
    
    return asset_node;
}

glm::vec3 AssetScatterBrush::CalculateAssetRotation(const glm::vec3& surface_normal, const ScatterAssetInfo& asset_info) const {
    glm::vec3 rotation(0.0f);
    
    // Random rotation within specified ranges
    rotation.x = m_uniform_dist(m_random_generator) * m_scatter_params.rotation_range.x;
    rotation.y = m_uniform_dist(m_random_generator) * m_scatter_params.rotation_range.y;
    rotation.z = m_uniform_dist(m_random_generator) * m_scatter_params.rotation_range.z;
    
    // Align to surface if requested
    if (m_scatter_params.align_to_surface && m_scatter_params.snapping_mode == SurfaceSnappingMode::TerrainNormals) {
        // Calculate rotation to align with surface normal
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 axis = glm::cross(up, surface_normal);
        float angle = std::acos(glm::dot(up, surface_normal));
        
        // Apply surface alignment (simplified - would need proper quaternion math)
        rotation.x += glm::degrees(angle * axis.x);
        rotation.z += glm::degrees(angle * axis.z);
    }
    
    return rotation;
}

glm::vec3 AssetScatterBrush::CalculateAssetScale(const ScatterAssetInfo& asset_info) const {
    float scale_factor = m_uniform_dist(m_random_generator) * 
                        (m_scatter_params.scale_range.y - m_scatter_params.scale_range.x) + 
                        m_scatter_params.scale_range.x;
    
    if (m_scatter_params.uniform_scale) {
        return glm::vec3(scale_factor);
    } else {
        // Non-uniform scaling
        float scale_x = m_uniform_dist(m_random_generator) * 
                       (m_scatter_params.scale_range.y - m_scatter_params.scale_range.x) + 
                       m_scatter_params.scale_range.x;
        float scale_y = scale_factor; // Keep Y consistent
        float scale_z = m_uniform_dist(m_random_generator) * 
                       (m_scatter_params.scale_range.y - m_scatter_params.scale_range.x) + 
                       m_scatter_params.scale_range.x;
        return glm::vec3(scale_x, scale_y, scale_z);
    }
}

float AssetScatterBrush::CalculateBrushWeight(const glm::vec2& offset, float brush_radius) const {
    float distance = glm::length(offset);
    
    if (distance >= brush_radius) return 0.0f;
    
    float normalized_distance = distance / brush_radius;
    float falloff_weight = 1.0f;
    
    switch (m_brush_settings.falloff) {
        case BrushFalloff::Linear:
            falloff_weight = 1.0f - normalized_distance;
            break;
        case BrushFalloff::Smooth:
            falloff_weight = glm::pow(1.0f - normalized_distance, 1.0f + m_brush_settings.falloff_curve * 3.0f);
            break;
        case BrushFalloff::Sharp:
            falloff_weight = glm::pow(1.0f - normalized_distance, 2.0f + m_brush_settings.falloff_curve * 8.0f);
            break;
        case BrushFalloff::Constant:
            falloff_weight = 1.0f;
            break;
        case BrushFalloff::Custom:
            falloff_weight = 1.0f - normalized_distance;
            break;
    }
    
    return falloff_weight * m_brush_settings.strength;
}

bool AssetScatterBrush::CheckSlope(const glm::vec3& surface_normal) const {
    float angle = glm::degrees(std::acos(glm::dot(surface_normal, glm::vec3(0.0f, 1.0f, 0.0f))));
    return angle >= m_scatter_params.min_slope_angle && angle <= m_scatter_params.max_slope_angle;
}

bool AssetScatterBrush::CheckWaterLevel(const glm::vec3& position) const {
    if (!m_scatter_params.avoid_water) return true;
    return position.y > m_scatter_params.water_level;
}

uint32_t AssetScatterBrush::GenerateAssetId() {
    static uint32_t next_id = 1;
    return next_id++;
}

std::vector<glm::vec3> AssetScatterBrush::GenerateRandomPattern(const glm::vec3& center, float radius, int count) const {
    std::vector<glm::vec3> positions;
    positions.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        // Generate random position within circle
        float angle = m_uniform_dist(m_random_generator) * 2.0f * M_PI;
        float distance = std::sqrt(m_uniform_dist(m_random_generator)) * radius;
        
        glm::vec3 pos = center;
        pos.x += std::cos(angle) * distance;
        pos.z += std::sin(angle) * distance;
        
        positions.push_back(pos);
    }
    
    return positions;
}

std::vector<glm::vec3> AssetScatterBrush::GenerateGridPattern(const glm::vec3& center, float radius, int count) const {
    std::vector<glm::vec3> positions;
    
    int grid_size = static_cast<int>(std::ceil(std::sqrt(count)));
    float spacing = (radius * 2.0f) / grid_size;
    
    for (int x = 0; x < grid_size; ++x) {
        for (int z = 0; z < grid_size; ++z) {
            if (positions.size() >= static_cast<size_t>(count)) break;
            
            glm::vec3 pos = center;
            pos.x += (x - grid_size * 0.5f) * spacing;
            pos.z += (z - grid_size * 0.5f) * spacing;
            
            // Check if within radius
            if (glm::length(glm::vec2(pos.x - center.x, pos.z - center.z)) <= radius) {
                positions.push_back(pos);
            }
        }
    }
    
    return positions;
}

std::vector<glm::vec3> AssetScatterBrush::GeneratePoissonPattern(const glm::vec3& center, float radius, int count) const {
    // Simplified Poisson disk sampling
    // In a full implementation, this would use proper Poisson disk sampling algorithm
    return GenerateRandomPattern(center, radius, count);
}

std::vector<glm::vec3> AssetScatterBrush::GenerateClusterPattern(const glm::vec3& center, float radius, int count) const {
    std::vector<glm::vec3> positions;
    
    // Create several cluster centers
    int cluster_count = std::max(1, count / 5);
    std::vector<glm::vec3> cluster_centers;
    
    for (int i = 0; i < cluster_count; ++i) {
        float angle = m_uniform_dist(m_random_generator) * 2.0f * M_PI;
        float distance = m_uniform_dist(m_random_generator) * radius * 0.7f;
        
        glm::vec3 cluster_center = center;
        cluster_center.x += std::cos(angle) * distance;
        cluster_center.z += std::sin(angle) * distance;
        cluster_centers.push_back(cluster_center);
    }
    
    // Distribute assets around cluster centers
    int assets_per_cluster = count / cluster_count;
    for (const glm::vec3& cluster_center : cluster_centers) {
        for (int i = 0; i < assets_per_cluster; ++i) {
            float angle = m_uniform_dist(m_random_generator) * 2.0f * M_PI;
            float distance = m_uniform_dist(m_random_generator) * radius * 0.3f;
            
            glm::vec3 pos = cluster_center;
            pos.x += std::cos(angle) * distance;
            pos.z += std::sin(angle) * distance;
            
            positions.push_back(pos);
        }
    }
    
    return positions;
}

// AssetScatterHistory Implementation

AssetScatterHistory::AssetScatterHistory(size_t max_history_size)
    : m_current_index(0)
    , m_max_history_size(max_history_size)
{
}

void AssetScatterHistory::RecordStroke(const AssetScatterStroke& stroke) {
    // Remove any redo history
    if (m_current_index < m_history.size()) {
        m_history.erase(m_history.begin() + m_current_index, m_history.end());
    }
    
    // Add new stroke
    m_history.push_back(stroke);
    m_current_index = m_history.size();
    
    // Trim history if needed
    TrimHistory();
}

bool AssetScatterHistory::Undo(Node3D* target_node) {
    if (!CanUndo() || !target_node) return false;
    
    --m_current_index;
    const AssetScatterStroke& stroke = m_history[m_current_index];
    ApplyStroke(stroke, target_node, true); // Reverse = true for undo
    
    return true;
}

bool AssetScatterHistory::Redo(Node3D* target_node) {
    if (!CanRedo() || !target_node) return false;
    
    const AssetScatterStroke& stroke = m_history[m_current_index];
    ApplyStroke(stroke, target_node, false); // Reverse = false for redo
    ++m_current_index;
    
    return true;
}

void AssetScatterHistory::Clear() {
    m_history.clear();
    m_current_index = 0;
}

void AssetScatterHistory::TrimHistory() {
    if (m_history.size() > m_max_history_size) {
        size_t excess = m_history.size() - m_max_history_size;
        m_history.erase(m_history.begin(), m_history.begin() + excess);
        m_current_index = std::min(m_current_index, m_history.size());
    }
}

void AssetScatterHistory::ApplyStroke(const AssetScatterStroke& stroke, Node3D* target_node, bool reverse) {
    if (reverse) {
        // Remove assets created in this stroke
        for (uint32_t asset_id : stroke.created_asset_ids) {
            // In a full implementation, we'd find and remove assets by ID
            std::cout << "AssetScatterHistory: Removing asset ID " << asset_id << std::endl;
        }
    } else {
        // Re-create assets from stroke
        std::cout << "AssetScatterHistory: Re-creating " << stroke.created_asset_ids.size() << " assets" << std::endl;
    }
}

} // namespace Lupine
