#include "lupine/terrain/TextureBrush.h"
#include "lupine/terrain/TerrainData.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace Lupine {

// TextureBrush Implementation

TextureBrush::TextureBrush()
    : m_stroke_active(false)
    , m_current_terrain(nullptr)
    , m_last_position(0.0f)
    , m_accumulated_distance(0.0f)
    , m_accumulated_flow(0.0f)
{
    // Initialize with default brush settings
    m_brush_settings.size = 5.0f;
    m_brush_settings.strength = 0.5f;
    m_brush_settings.falloff = BrushFalloff::Smooth;
    m_brush_settings.falloff_curve = 0.5f;
    m_brush_settings.spacing = 0.25f;
}

void TextureBrush::SetBrushSettings(const BrushSettings& settings) {
    m_brush_settings = settings;
}

void TextureBrush::SetPaintParams(const TexturePaintParams& params) {
    m_paint_params = params;
}

int TextureBrush::AddTextureLayer(const TextureLayerInfo& layer_info) {
    m_texture_layers.push_back(layer_info);
    return static_cast<int>(m_texture_layers.size() - 1);
}

void TextureBrush::RemoveTextureLayer(int layer_index) {
    if (layer_index >= 0 && layer_index < static_cast<int>(m_texture_layers.size())) {
        m_texture_layers.erase(m_texture_layers.begin() + layer_index);
    }
}

const TextureLayerInfo& TextureBrush::GetTextureLayer(int layer_index) const {
    static TextureLayerInfo empty_layer;
    if (layer_index >= 0 && layer_index < static_cast<int>(m_texture_layers.size())) {
        return m_texture_layers[layer_index];
    }
    return empty_layer;
}

void TextureBrush::UpdateTextureLayer(int layer_index, const TextureLayerInfo& layer_info) {
    if (layer_index >= 0 && layer_index < static_cast<int>(m_texture_layers.size())) {
        m_texture_layers[layer_index] = layer_info;
    }
}

void TextureBrush::StartStroke(const glm::vec3& world_pos, TerrainData* terrain_data) {
    if (!terrain_data) return;
    
    m_stroke_active = true;
    m_current_terrain = terrain_data;
    m_last_position = world_pos;
    m_accumulated_distance = 0.0f;
    m_accumulated_flow = 0.0f;
    
    // Initialize stroke data
    m_current_stroke = TexturePaintStroke();
    m_current_stroke.brush_settings = m_brush_settings;
    m_current_stroke.paint_params = m_paint_params;
    m_current_stroke.positions.push_back(world_pos);
    
    // Apply initial texture dab
    ApplyTextureDab(world_pos, terrain_data, 1.0f);
}

void TextureBrush::ContinueStroke(const glm::vec3& world_pos, float delta_time) {
    if (!m_stroke_active || !m_current_terrain) return;
    
    // Calculate distance from last position
    float distance = glm::length(world_pos - m_last_position);
    m_accumulated_distance += distance;
    m_accumulated_flow += delta_time * m_paint_params.flow_rate;
    
    // Check if we should apply brush based on spacing
    float spacing_distance = m_brush_settings.size * m_brush_settings.spacing;
    
    if (m_accumulated_distance >= spacing_distance) {
        // Apply texture dab with flow rate
        float flow_strength = std::min(m_accumulated_flow, 1.0f);
        ApplyTextureDab(world_pos, m_current_terrain, flow_strength);
        
        // Record position
        m_current_stroke.positions.push_back(world_pos);
        
        // Reset accumulated distance and flow
        m_accumulated_distance = 0.0f;
        m_accumulated_flow = 0.0f;
    }
    
    m_last_position = world_pos;
}

void TextureBrush::EndStroke() {
    if (!m_stroke_active) return;
    
    m_stroke_active = false;
    
    // Notify callback if set
    if (m_stroke_callback) {
        m_stroke_callback(m_current_stroke);
    }
    
    m_current_terrain = nullptr;
}

void TextureBrush::ApplyTextureDab(const glm::vec3& world_pos, TerrainData* terrain_data, float strength_multiplier) {
    if (!terrain_data || m_paint_params.target_layer < 0 || 
        m_paint_params.target_layer >= static_cast<int>(m_texture_layers.size())) {
        return;
    }
    
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
                        // Get original texture blend
                        const TerrainTextureBlend& original_blend = chunk->GetTextureBlend(x, z);
                        
                        // Convert to weight vector
                        std::vector<float> original_weights(m_texture_layers.size(), 0.0f);
                        for (size_t i = 0; i < std::min(original_weights.size(), original_blend.layer_weights.size()); ++i) {
                            original_weights[i] = original_blend.layer_weights[i];
                        }
                        
                        // Record original weights for undo if this is a new stroke
                        if (m_stroke_active) {
                            RecordOriginalWeights(chunk->GetChunkCoords(), x, z, original_weights);
                        }
                        
                        // Apply texture painting
                        ApplyTexturePaint(chunk, x, z, original_weights, brush_weight, 0.016f); // Assume 60 FPS
                    }
                }
            }
        }
        
        // Mark chunk as dirty
        chunk->SetDirty(true);
    }
}

std::vector<float> TextureBrush::GetTextureWeightsAt(const glm::vec3& world_pos, const TerrainData* terrain_data) const {
    std::vector<float> weights(m_texture_layers.size(), 0.0f);
    
    if (!terrain_data) return weights;
    
    // Find the chunk containing this position
    glm::ivec2 chunk_coords = terrain_data->WorldToChunkCoords(world_pos);
    const TerrainChunk* chunk = const_cast<TerrainData*>(terrain_data)->GetChunk(chunk_coords.x, chunk_coords.y);
    
    if (chunk) {
        glm::vec2 local_coords = terrain_data->WorldToLocalChunkCoords(world_pos);
        TerrainTextureBlend blend = chunk->GetTextureBlendInterpolated(local_coords.x, local_coords.y);
        
        for (size_t i = 0; i < std::min(weights.size(), blend.layer_weights.size()); ++i) {
            weights[i] = blend.layer_weights[i];
        }
    }
    
    return weights;
}

int TextureBrush::GetDominantTextureAt(const glm::vec3& world_pos, const TerrainData* terrain_data) const {
    std::vector<float> weights = GetTextureWeightsAt(world_pos, terrain_data);
    
    int dominant_layer = 0;
    float max_weight = 0.0f;
    
    for (size_t i = 0; i < weights.size(); ++i) {
        if (weights[i] > max_weight) {
            max_weight = weights[i];
            dominant_layer = static_cast<int>(i);
        }
    }
    
    return dominant_layer;
}

void TextureBrush::SetStrokeCallback(std::function<void(const TexturePaintStroke&)> callback) {
    m_stroke_callback = callback;
}

void TextureBrush::ApplyTexturePaint(TerrainChunk* chunk, int local_x, int local_z, 
                                    const std::vector<float>& original_weights, float brush_weight, float delta_time) {
    if (!chunk || brush_weight <= 0.0f) return;
    
    // Calculate new weights
    std::vector<float> new_weights = CalculateNewWeights(original_weights, brush_weight, delta_time);
    
    // Normalize weights if requested
    if (m_paint_params.normalize_weights) {
        NormalizeWeights(new_weights);
    }
    
    // Create texture blend data
    TerrainTextureBlend blend_data(new_weights.size());
    for (size_t i = 0; i < new_weights.size(); ++i) {
        blend_data.SetLayerWeight(i, new_weights[i]);
    }
    
    // Set texture blend
    chunk->SetTextureBlend(local_x, local_z, blend_data);
}

std::vector<float> TextureBrush::CalculateNewWeights(const std::vector<float>& original_weights, 
                                                    float brush_weight, float delta_time) const {
    std::vector<float> new_weights = original_weights;
    
    // Ensure we have enough weights for all layers
    if (new_weights.size() < m_texture_layers.size()) {
        new_weights.resize(m_texture_layers.size(), 0.0f);
    }
    
    // Calculate paint strength
    float paint_strength = m_paint_params.opacity * brush_weight * delta_time * 60.0f; // Normalize for 60 FPS
    paint_strength = glm::clamp(paint_strength, 0.0f, 1.0f);
    
    // Apply blend mode to target layer
    if (m_paint_params.target_layer < static_cast<int>(new_weights.size())) {
        float original_weight = new_weights[m_paint_params.target_layer];
        float new_weight = ApplyBlendMode(original_weight, paint_strength, m_paint_params.blend_mode);
        new_weights[m_paint_params.target_layer] = glm::clamp(new_weight, 0.0f, 1.0f);
    }
    
    return new_weights;
}

void TextureBrush::NormalizeWeights(std::vector<float>& weights) const {
    float total_weight = 0.0f;
    for (float weight : weights) {
        total_weight += weight;
    }
    
    if (total_weight > 0.0f) {
        for (float& weight : weights) {
            weight /= total_weight;
        }
    } else {
        // If no weights, set first layer to full weight
        if (!weights.empty()) {
            weights[0] = 1.0f;
        }
    }
}

float TextureBrush::ApplyBlendMode(float original_weight, float paint_weight, TextureBlendMode blend_mode) const {
    switch (blend_mode) {
        case TextureBlendMode::Replace:
            return glm::mix(original_weight, 1.0f, paint_weight);
            
        case TextureBlendMode::Add:
            return original_weight + paint_weight;
            
        case TextureBlendMode::Subtract:
            return original_weight - paint_weight;
            
        case TextureBlendMode::Multiply:
            return original_weight * (1.0f - paint_weight) + (original_weight * paint_weight);
            
        case TextureBlendMode::Overlay:
            if (original_weight < 0.5f) {
                return 2.0f * original_weight * paint_weight;
            } else {
                return 1.0f - 2.0f * (1.0f - original_weight) * (1.0f - paint_weight);
            }
            
        case TextureBlendMode::SoftLight:
            return original_weight * (1.0f - paint_weight) + 
                   glm::sqrt(original_weight) * paint_weight;
            
        case TextureBlendMode::HardLight:
            if (paint_weight < 0.5f) {
                return 2.0f * original_weight * paint_weight;
            } else {
                return 1.0f - 2.0f * (1.0f - original_weight) * (1.0f - paint_weight);
            }
            
        default:
            return glm::mix(original_weight, 1.0f, paint_weight);
    }
}

std::vector<TerrainChunk*> TextureBrush::GetAffectedChunks(const glm::vec3& world_pos, TerrainData* terrain_data) const {
    std::vector<TerrainChunk*> chunks;
    
    if (!terrain_data) return chunks;
    
    // Calculate brush bounds
    float radius = m_brush_settings.size;
    glm::vec3 min_bounds(world_pos.x - radius, world_pos.y - radius, world_pos.z - radius);
    glm::vec3 max_bounds(world_pos.x + radius, world_pos.y + radius, world_pos.z + radius);
    
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

void TextureBrush::RecordOriginalWeights(const glm::ivec2& chunk_coords, int local_x, int local_z, 
                                        const std::vector<float>& weights) {
    // Check if this position is already recorded in current stroke
    for (const auto& [coords, original_weights] : m_current_stroke.original_weights) {
        if (coords.x == chunk_coords.x && coords.y == chunk_coords.y) {
            // Already recorded for this chunk position
            return;
        }
    }
    
    // Record original weights
    m_current_stroke.original_weights.emplace_back(
        glm::ivec2(chunk_coords.x * 10000 + local_x, chunk_coords.y * 10000 + local_z), 
        weights
    );
}

float TextureBrush::CalculateBrushWeight(const glm::vec2& offset, float brush_radius) const {
    float distance = glm::length(offset);
    
    // Check if point is within brush shape
    bool in_shape = false;
    switch (m_brush_settings.shape) {
        case TerrainBrushShape::Circle:
            in_shape = distance <= brush_radius;
            break;
        case TerrainBrushShape::Square:
            in_shape = std::abs(offset.x) <= brush_radius && std::abs(offset.y) <= brush_radius;
            break;
        case TerrainBrushShape::Diamond: {
            float diamond_distance = std::abs(offset.x) + std::abs(offset.y);
            in_shape = diamond_distance <= brush_radius;
            break;
        }
        case TerrainBrushShape::Custom:
            in_shape = distance <= brush_radius; // Default to circle for custom
            break;
    }
    
    if (!in_shape) return 0.0f;
    
    // Calculate falloff weight
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
            falloff_weight = 1.0f - normalized_distance; // Default to linear
            break;
    }
    
    return falloff_weight * m_brush_settings.strength;
}

// TexturePaintingHistory Implementation

TexturePaintingHistory::TexturePaintingHistory(size_t max_history_size)
    : m_current_index(0)
    , m_max_history_size(max_history_size)
{
}

void TexturePaintingHistory::RecordStroke(const TexturePaintStroke& stroke) {
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

bool TexturePaintingHistory::Undo(TerrainData* terrain_data) {
    if (!CanUndo() || !terrain_data) return false;
    
    --m_current_index;
    const TexturePaintStroke& stroke = m_history[m_current_index];
    ApplyStroke(stroke, terrain_data, true); // Reverse = true for undo
    
    return true;
}

bool TexturePaintingHistory::Redo(TerrainData* terrain_data) {
    if (!CanRedo() || !terrain_data) return false;
    
    const TexturePaintStroke& stroke = m_history[m_current_index];
    ApplyStroke(stroke, terrain_data, false); // Reverse = false for redo
    ++m_current_index;
    
    return true;
}

void TexturePaintingHistory::Clear() {
    m_history.clear();
    m_current_index = 0;
}

void TexturePaintingHistory::TrimHistory() {
    if (m_history.size() > m_max_history_size) {
        size_t excess = m_history.size() - m_max_history_size;
        m_history.erase(m_history.begin(), m_history.begin() + excess);
        m_current_index = std::min(m_current_index, m_history.size());
    }
}

void TexturePaintingHistory::ApplyStroke(const TexturePaintStroke& stroke, TerrainData* terrain_data, bool reverse) {
    // Apply or reverse the stroke based on recorded original weights
    for (const auto& [encoded_coords, original_weights] : stroke.original_weights) {
        // Decode coordinates
        int chunk_x = encoded_coords.x / 10000;
        int local_x = encoded_coords.x % 10000;
        int chunk_z = encoded_coords.y / 10000;
        int local_z = encoded_coords.y % 10000;
        
        TerrainChunk* chunk = terrain_data->GetChunk(chunk_x, chunk_z);
        if (chunk) {
            if (reverse) {
                // Restore original weights
                TerrainTextureBlend blend_data(original_weights.size());
                for (size_t i = 0; i < original_weights.size(); ++i) {
                    blend_data.SetLayerWeight(i, original_weights[i]);
                }
                chunk->SetTextureBlend(local_x, local_z, blend_data);
            } else {
                // Re-apply the modification (would need to store final weights too)
                // For now, just restore original weights
                TerrainTextureBlend blend_data(original_weights.size());
                for (size_t i = 0; i < original_weights.size(); ++i) {
                    blend_data.SetLayerWeight(i, original_weights[i]);
                }
                chunk->SetTextureBlend(local_x, local_z, blend_data);
            }
            chunk->SetDirty(true);
        }
    }
}

} // namespace Lupine
