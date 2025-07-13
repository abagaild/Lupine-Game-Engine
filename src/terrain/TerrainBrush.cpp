#include "lupine/terrain/TerrainBrush.h"
#include "lupine/terrain/TerrainData.h"
#include <algorithm>
#include <cmath>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Lupine {

// TerrainBrush Implementation

TerrainBrush::TerrainBrush()
    : m_stroke_active(false)
    , m_current_terrain(nullptr)
    , m_last_position(0.0f)
    , m_accumulated_distance(0.0f)
    , m_custom_brush_size(0)
{
}

void TerrainBrush::SetBrushSettings(const BrushSettings& settings) {
    m_brush_settings = settings;
    
    // Load custom brush if path changed
    if (settings.shape == TerrainBrushShape::Custom && !settings.custom_brush_path.empty()) {
        LoadCustomBrush(settings.custom_brush_path);
    }
}

void TerrainBrush::SetOperationParams(const HeightOperationParams& params) {
    m_operation_params = params;
}

void TerrainBrush::StartStroke(const glm::vec3& world_pos, TerrainData* terrain_data) {
    if (!terrain_data) return;
    
    m_stroke_active = true;
    m_current_terrain = terrain_data;
    m_last_position = world_pos;
    m_accumulated_distance = 0.0f;
    
    // Initialize stroke data
    m_current_stroke = BrushStroke();
    m_current_stroke.brush_settings = m_brush_settings;
    m_current_stroke.operation_params = m_operation_params;
    m_current_stroke.positions.push_back(world_pos);
    
    // Apply initial brush dab
    ApplyBrushDab(world_pos, terrain_data, 1.0f);
}

void TerrainBrush::ContinueStroke(const glm::vec3& world_pos, float /*delta_time*/) {
    if (!m_stroke_active || !m_current_terrain) return;
    
    // Calculate distance from last position
    float distance = glm::length(world_pos - m_last_position);
    m_accumulated_distance += distance;
    
    // Check if we should apply brush based on spacing
    float spacing_distance = m_brush_settings.size * m_brush_settings.spacing;
    
    if (m_accumulated_distance >= spacing_distance) {
        // Apply brush dab
        ApplyBrushDab(world_pos, m_current_terrain, 1.0f);
        
        // Record position
        m_current_stroke.positions.push_back(world_pos);
        
        // Reset accumulated distance
        m_accumulated_distance = 0.0f;
    }
    
    m_last_position = world_pos;
}

void TerrainBrush::EndStroke() {
    if (!m_stroke_active) return;
    
    m_stroke_active = false;
    
    // Notify callback if set
    if (m_stroke_callback) {
        m_stroke_callback(m_current_stroke);
    }
    
    m_current_terrain = nullptr;
}

void TerrainBrush::ApplyBrushDab(const glm::vec3& world_pos, TerrainData* terrain_data, float strength_multiplier) {
    if (!terrain_data) return;
    
    // Get affected chunks
    std::vector<TerrainChunk*> affected_chunks = GetAffectedChunks(world_pos, terrain_data);
    
    for (TerrainChunk* chunk : affected_chunks) {
        if (!chunk) continue;
        
        auto [chunk_min, chunk_max] = chunk->GetWorldBounds();
        glm::ivec2 heightmap_size = chunk->GetHeightMapSize();
        
        // Iterate through chunk heightmap
        for (int z = 0; z < heightmap_size.y; ++z) {
            for (int x = 0; x < heightmap_size.x; ++x) {
                // Convert local coordinates to world position
                float world_x = chunk_min.x + (static_cast<float>(x) / chunk->GetResolution());
                float world_z = chunk_min.z + (static_cast<float>(z) / chunk->GetResolution());
                glm::vec3 sample_pos(world_x, 0.0f, world_z);
                
                // Calculate distance from brush center
                glm::vec2 offset(sample_pos.x - world_pos.x, sample_pos.z - world_pos.z);
                float distance = glm::length(offset);
                
                // Check if point is within brush radius
                if (distance <= m_brush_settings.size) {
                    // Calculate brush weight
                    float brush_weight = CalculateBrushWeight(offset, m_brush_settings.size);
                    brush_weight *= strength_multiplier;
                    
                    if (brush_weight > 0.0f) {
                        // Check slope constraints if enabled
                        if (m_operation_params.respect_max_slope && 
                            !ShouldRespectSlope(terrain_data, sample_pos)) {
                            continue;
                        }
                        
                        // Get original height
                        float original_height = chunk->GetHeight(x, z);
                        
                        // Record original height for undo if this is a new stroke
                        if (m_stroke_active) {
                            RecordOriginalHeight(chunk->GetChunkCoords(), x, z, original_height);
                        }
                        
                        // Apply height operation
                        ApplyHeightOperation(chunk, x, z, original_height, brush_weight, 0.016f); // Assume 60 FPS
                    }
                }
            }
        }
        
        // Mark chunk as dirty
        chunk->SetDirty(true);
    }
}

void TerrainBrush::GenerateBrushPreview(const glm::vec3& world_pos, const TerrainData* terrain_data,
                                       std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices) const {
    vertices.clear();
    indices.clear();
    
    if (!terrain_data) return;
    
    // Generate circle preview for now (can be enhanced for other shapes)
    const int segments = 32;
    const float radius = m_brush_settings.size;
    
    // Center vertex
    float center_height = terrain_data->GetHeightAtWorldPos(world_pos);
    vertices.push_back(glm::vec3(world_pos.x, center_height + 0.1f, world_pos.z));
    
    // Ring vertices
    for (int i = 0; i < segments; ++i) {
        float angle = (static_cast<float>(i) / segments) * 2.0f * M_PI;
        float x = world_pos.x + std::cos(angle) * radius;
        float z = world_pos.z + std::sin(angle) * radius;
        float height = terrain_data->GetHeightAtWorldPos(glm::vec3(x, 0.0f, z));
        
        vertices.push_back(glm::vec3(x, height + 0.1f, z));
    }
    
    // Generate indices for triangle fan
    for (int i = 0; i < segments; ++i) {
        indices.push_back(0); // Center
        indices.push_back(i + 1);
        indices.push_back((i + 1) % segments + 1);
    }
}

std::pair<glm::vec3, glm::vec3> TerrainBrush::GetBrushBounds(const glm::vec3& world_pos) const {
    float radius = m_brush_settings.size;
    glm::vec3 min_bounds(world_pos.x - radius, world_pos.y - radius, world_pos.z - radius);
    glm::vec3 max_bounds(world_pos.x + radius, world_pos.y + radius, world_pos.z + radius);
    return {min_bounds, max_bounds};
}

void TerrainBrush::SetStrokeCallback(std::function<void(const BrushStroke&)> callback) {
    m_stroke_callback = callback;
}

float TerrainBrush::CalculateBrushWeight(const glm::vec2& offset, float brush_radius) const {
    float distance = glm::length(offset);
    
    // Check if point is within brush shape
    if (!IsPointInBrushShape(offset, brush_radius, m_brush_settings.shape)) {
        return 0.0f;
    }
    
    // Calculate falloff weight
    float falloff_weight = CalculateFalloffWeight(distance, brush_radius, 
                                                 m_brush_settings.falloff, 
                                                 m_brush_settings.falloff_curve);
    
    return falloff_weight * m_brush_settings.strength;
}

float TerrainBrush::CalculateFalloffWeight(float distance, float radius, BrushFalloff falloff, float curve) const {
    if (distance >= radius) return 0.0f;
    
    float normalized_distance = distance / radius;
    
    switch (falloff) {
        case BrushFalloff::Linear:
            return 1.0f - normalized_distance;
            
        case BrushFalloff::Smooth:
            return glm::pow(1.0f - normalized_distance, 1.0f + curve * 3.0f);
        
        case BrushFalloff::Sharp:
            return glm::pow(1.0f - normalized_distance, 2.0f + curve * 8.0f);
            
        case BrushFalloff::Constant:
            return 1.0f;
            
        case BrushFalloff::Custom:
            // Custom falloff could be implemented with a curve texture
            return 1.0f - normalized_distance;
            
        default:
            return 1.0f - normalized_distance;
    }
}

bool TerrainBrush::IsPointInBrushShape(const glm::vec2& offset, float radius, TerrainBrushShape shape) const {
    switch (shape) {
        case TerrainBrushShape::Circle:
            return glm::length(offset) <= radius;
            
        case TerrainBrushShape::Square:
            return std::abs(offset.x) <= radius && std::abs(offset.y) <= radius;

        case TerrainBrushShape::Diamond: {
            float diamond_distance = std::abs(offset.x) + std::abs(offset.y);
            return diamond_distance <= radius;
        }

        case TerrainBrushShape::Custom:
            // Custom shape implementation would go here
            return glm::length(offset) <= radius;
            
        default:
            return glm::length(offset) <= radius;
    }
}

void TerrainBrush::ApplyHeightOperation(TerrainChunk* chunk, int local_x, int local_z,
                                       float original_height, float brush_weight, float delta_time) {
    if (!chunk || brush_weight <= 0.0f) return;

    float target_height = CalculateTargetHeight(m_operation_params.operation, original_height,
                                               m_operation_params.target_height, brush_weight, delta_time);

    chunk->SetHeight(local_x, local_z, target_height);
}

float TerrainBrush::CalculateTargetHeight(HeightOperation operation, float current_height, 
                                         float target_height, float brush_weight, float delta_time) {
    float modification_speed = 10.0f; // Units per second
    float max_change = modification_speed * brush_weight * delta_time;
    
    switch (operation) {
        case HeightOperation::Raise:
            return current_height + max_change;
            
        case HeightOperation::Lower:
            return current_height - max_change;
            
        case HeightOperation::Flatten: {
            float difference = target_height - current_height;
            float change = glm::sign(difference) * std::min(std::abs(difference), max_change);
            return current_height + change;
        }
        
        case HeightOperation::Smooth: {
            // Smoothing would require neighboring height samples
            // For now, just return current height
            return current_height;
        }
        
        case HeightOperation::Noise: {
            glm::vec2 noise_position(0.0f, 0.0f); // Use world position for noise
            float noise = GenerateNoise(noise_position, m_operation_params.noise_scale,
                                       m_operation_params.noise_frequency);
            return current_height + noise * max_change;
        }
        
        case HeightOperation::Set:
            return target_height;
            
        default:
            return current_height;
    }
}

float TerrainBrush::GenerateNoise(const glm::vec2& position, float scale, float frequency) const {
    // Simple Perlin-like noise implementation
    // In a full implementation, you'd use a proper noise library
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    
    // For now, just return random noise
    return dis(gen) * scale;
}

float TerrainBrush::CalculateSlope(const TerrainData* terrain_data, const glm::vec3& world_pos) const {
    if (!terrain_data) return 0.0f;
    
    // Sample neighboring heights to calculate slope
    float sample_distance = 0.5f;
    float center_height = terrain_data->GetHeightAtWorldPos(world_pos);
    float right_height = terrain_data->GetHeightAtWorldPos(world_pos + glm::vec3(sample_distance, 0.0f, 0.0f));
    float forward_height = terrain_data->GetHeightAtWorldPos(world_pos + glm::vec3(0.0f, 0.0f, sample_distance));
    
    glm::vec3 tangent_x(sample_distance, right_height - center_height, 0.0f);
    glm::vec3 tangent_z(0.0f, forward_height - center_height, sample_distance);
    glm::vec3 normal = glm::normalize(glm::cross(tangent_x, tangent_z));
    
    // Calculate angle from vertical
    float angle = std::acos(glm::dot(normal, glm::vec3(0.0f, 1.0f, 0.0f)));
    return glm::degrees(angle);
}

bool TerrainBrush::ShouldRespectSlope(const TerrainData* terrain_data, const glm::vec3& world_pos) const {
    if (!m_operation_params.respect_max_slope) return true;
    
    float slope = CalculateSlope(terrain_data, world_pos);
    return slope <= m_operation_params.max_slope_angle;
}

bool TerrainBrush::LoadCustomBrush(const std::string& brush_path) {
    // Custom brush loading implementation would go here
    // For now, just return false
    return false;
}

std::vector<TerrainChunk*> TerrainBrush::GetAffectedChunks(const glm::vec3& world_pos, TerrainData* terrain_data) const {
    std::vector<TerrainChunk*> chunks;
    
    if (!terrain_data) return chunks;
    
    // Calculate brush bounds
    auto [min_bounds, max_bounds] = GetBrushBounds(world_pos);
    
    // Find all chunks that intersect with brush bounds
    glm::ivec2 min_chunk = terrain_data->WorldToChunkCoords(min_bounds);
    glm::ivec2 max_chunk = terrain_data->WorldToChunkCoords(max_bounds);
    
    for (int chunk_z = min_chunk.y; chunk_z <= max_chunk.y; ++chunk_z) {
        for (int chunk_x = min_chunk.x; chunk_x <= max_chunk.x; ++chunk_x) {
            TerrainChunk* chunk = terrain_data->CreateChunk(chunk_x, chunk_z);
            if (chunk) {
                chunks.push_back(chunk);
            }
        }
    }
    
    return chunks;
}

void TerrainBrush::RecordOriginalHeight(const glm::ivec2& chunk_coords, int local_x, int local_z, float height) {
    // Check if this position is already recorded in current stroke
    for (const auto& [coords, original_height] : m_current_stroke.original_heights) {
        if (coords.x == chunk_coords.x && coords.y == chunk_coords.y) {
            // Already recorded for this chunk position
            return;
        }
    }
    
    // Record original height
    m_current_stroke.original_heights.emplace_back(
        glm::ivec2(chunk_coords.x * 10000 + local_x, chunk_coords.y * 10000 + local_z), 
        height
    );
}

// HeightPaintingHistory Implementation

HeightPaintingHistory::HeightPaintingHistory(size_t max_history_size)
    : m_current_index(0)
    , m_max_history_size(max_history_size)
{
}

void HeightPaintingHistory::RecordStroke(const BrushStroke& stroke) {
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

bool HeightPaintingHistory::Undo(TerrainData* terrain_data) {
    if (!CanUndo() || !terrain_data) return false;
    
    --m_current_index;
    const BrushStroke& stroke = m_history[m_current_index];
    ApplyStroke(stroke, terrain_data, true); // Reverse = true for undo
    
    return true;
}

bool HeightPaintingHistory::Redo(TerrainData* terrain_data) {
    if (!CanRedo() || !terrain_data) return false;
    
    const BrushStroke& stroke = m_history[m_current_index];
    ApplyStroke(stroke, terrain_data, false); // Reverse = false for redo
    ++m_current_index;
    
    return true;
}

void HeightPaintingHistory::Clear() {
    m_history.clear();
    m_current_index = 0;
}

void HeightPaintingHistory::TrimHistory() {
    if (m_history.size() > m_max_history_size) {
        size_t excess = m_history.size() - m_max_history_size;
        m_history.erase(m_history.begin(), m_history.begin() + excess);
        m_current_index = std::min(m_current_index, m_history.size());
    }
}

void HeightPaintingHistory::ApplyStroke(const BrushStroke& stroke, TerrainData* terrain_data, bool reverse) {
    // Apply or reverse the stroke based on recorded original heights
    for (const auto& [encoded_coords, original_height] : stroke.original_heights) {
        // Decode coordinates
        int chunk_x = encoded_coords.x / 10000;
        int local_x = encoded_coords.x % 10000;
        int chunk_z = encoded_coords.y / 10000;
        int local_z = encoded_coords.y % 10000;
        
        TerrainChunk* chunk = terrain_data->GetChunk(chunk_x, chunk_z);
        if (chunk) {
            if (reverse) {
                // Restore original height
                chunk->SetHeight(local_x, local_z, original_height);
            } else {
                // Re-apply the modification (would need to store final heights too)
                // For now, just restore original height
                chunk->SetHeight(local_x, local_z, original_height);
            }
            chunk->SetDirty(true);
        }
    }
}

} // namespace Lupine
