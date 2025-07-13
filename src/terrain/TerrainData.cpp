#include "lupine/terrain/TerrainData.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <limits>

namespace Lupine {

// TerrainChunk Implementation

TerrainChunk::TerrainChunk(int chunk_x, int chunk_z, float chunk_size, float resolution)
    : m_chunk_x(chunk_x)
    , m_chunk_z(chunk_z)
    , m_chunk_size(chunk_size)
    , m_resolution(resolution)
    , m_next_asset_id(1)
    , m_is_dirty(true)
    , m_is_loaded(false)
{
    // Calculate heightmap size based on chunk size and resolution
    int samples_per_side = static_cast<int>(chunk_size * resolution) + 1;
    m_heightmap_size = glm::ivec2(samples_per_side, samples_per_side);
    
    // Initialize height data
    size_t total_samples = m_heightmap_size.x * m_heightmap_size.y;
    m_height_data.resize(total_samples, 0.0f);
    
    // Initialize texture blend data
    m_texture_blend_data.resize(total_samples);
}

std::pair<glm::vec3, glm::vec3> TerrainChunk::GetWorldBounds() const {
    float world_x = m_chunk_x * m_chunk_size;
    float world_z = m_chunk_z * m_chunk_size;
    
    glm::vec3 min_bounds(world_x, 0.0f, world_z);
    glm::vec3 max_bounds(world_x + m_chunk_size, 0.0f, world_z + m_chunk_size);
    
    // Calculate actual height bounds
    if (!m_height_data.empty()) {
        auto [min_height, max_height] = std::minmax_element(m_height_data.begin(), m_height_data.end());
        min_bounds.y = *min_height;
        max_bounds.y = *max_height;
    }
    
    return {min_bounds, max_bounds};
}

void TerrainChunk::SetHeight(int x, int z, float height) {
    if (IsValidCoordinate(x, z)) {
        int index = GetHeightIndex(x, z);
        m_height_data[index] = height;
        SetDirty(true);
    }
}

float TerrainChunk::GetHeight(int x, int z) const {
    if (IsValidCoordinate(x, z)) {
        int index = GetHeightIndex(x, z);
        return m_height_data[index];
    }
    return 0.0f;
}

float TerrainChunk::GetHeightInterpolated(float x, float z) const {
    // Clamp to valid range
    x = glm::clamp(x, 0.0f, static_cast<float>(m_heightmap_size.x - 1));
    z = glm::clamp(z, 0.0f, static_cast<float>(m_heightmap_size.y - 1));
    
    // Get integer coordinates
    int x0 = static_cast<int>(std::floor(x));
    int z0 = static_cast<int>(std::floor(z));
    int x1 = std::min(x0 + 1, m_heightmap_size.x - 1);
    int z1 = std::min(z0 + 1, m_heightmap_size.y - 1);
    
    // Get fractional parts
    float fx = x - x0;
    float fz = z - z0;
    
    // Bilinear interpolation
    float h00 = GetHeight(x0, z0);
    float h10 = GetHeight(x1, z0);
    float h01 = GetHeight(x0, z1);
    float h11 = GetHeight(x1, z1);
    
    float h0 = h00 * (1.0f - fx) + h10 * fx;
    float h1 = h01 * (1.0f - fx) + h11 * fx;
    
    return h0 * (1.0f - fz) + h1 * fz;
}

void TerrainChunk::SetHeightData(const std::vector<float>& height_data) {
    if (height_data.size() == m_height_data.size()) {
        m_height_data = height_data;
        SetDirty(true);
    }
}

void TerrainChunk::SetTextureBlend(int x, int z, const TerrainTextureBlend& blend_data) {
    if (IsValidCoordinate(x, z)) {
        int index = GetHeightIndex(x, z);
        m_texture_blend_data[index] = blend_data;
        SetDirty(true);
    }
}

const TerrainTextureBlend& TerrainChunk::GetTextureBlend(int x, int z) const {
    static TerrainTextureBlend empty_blend;
    if (IsValidCoordinate(x, z)) {
        int index = GetHeightIndex(x, z);
        return m_texture_blend_data[index];
    }
    return empty_blend;
}

TerrainTextureBlend TerrainChunk::GetTextureBlendInterpolated(float x, float z) const {
    // Clamp to valid range
    x = glm::clamp(x, 0.0f, static_cast<float>(m_heightmap_size.x - 1));
    z = glm::clamp(z, 0.0f, static_cast<float>(m_heightmap_size.y - 1));
    
    // Get integer coordinates
    int x0 = static_cast<int>(std::floor(x));
    int z0 = static_cast<int>(std::floor(z));
    int x1 = std::min(x0 + 1, m_heightmap_size.x - 1);
    int z1 = std::min(z0 + 1, m_heightmap_size.y - 1);
    
    // Get fractional parts
    float fx = x - x0;
    float fz = z - z0;
    
    // Get blend data at corners
    const auto& blend00 = GetTextureBlend(x0, z0);
    const auto& blend10 = GetTextureBlend(x1, z0);
    const auto& blend01 = GetTextureBlend(x0, z1);
    const auto& blend11 = GetTextureBlend(x1, z1);
    
    // Interpolate blend weights
    TerrainTextureBlend result;
    if (!blend00.layer_weights.empty()) {
        result.layer_weights.resize(blend00.layer_weights.size());
        
        for (size_t i = 0; i < result.layer_weights.size(); ++i) {
            float w00 = blend00.GetLayerWeight(i);
            float w10 = blend10.GetLayerWeight(i);
            float w01 = blend01.GetLayerWeight(i);
            float w11 = blend11.GetLayerWeight(i);
            
            float w0 = w00 * (1.0f - fx) + w10 * fx;
            float w1 = w01 * (1.0f - fx) + w11 * fx;
            
            result.layer_weights[i] = w0 * (1.0f - fz) + w1 * fz;
        }
    }
    
    return result;
}

void TerrainChunk::AddAsset(const TerrainAsset& asset) {
    TerrainAsset new_asset = asset;
    new_asset.asset_id = m_next_asset_id++;
    m_assets.push_back(new_asset);
    SetDirty(true);
}

void TerrainChunk::RemoveAsset(uint32_t asset_id) {
    auto it = std::remove_if(m_assets.begin(), m_assets.end(),
        [asset_id](const TerrainAsset& asset) {
            return asset.asset_id == asset_id;
        });
    
    if (it != m_assets.end()) {
        m_assets.erase(it, m_assets.end());
        SetDirty(true);
    }
}

void TerrainChunk::GenerateMesh(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices,
                               bool include_normals, bool include_uvs) const {
    vertices.clear();
    indices.clear();
    
    // Generate vertices
    for (int z = 0; z < m_heightmap_size.y; ++z) {
        for (int x = 0; x < m_heightmap_size.x; ++x) {
            float height = GetHeight(x, z);
            glm::vec3 world_pos = LocalToWorldPosition(x, z, height);
            vertices.push_back(world_pos);
            
            if (include_normals) {
                glm::vec3 normal = CalculateNormal(x, z);
                vertices.push_back(normal);
            }
            
            if (include_uvs) {
                float u = static_cast<float>(x) / (m_heightmap_size.x - 1);
                float v = static_cast<float>(z) / (m_heightmap_size.y - 1);
                vertices.push_back(glm::vec3(u, v, 0.0f));
            }
        }
    }
    
    // Generate indices for triangles
    for (int z = 0; z < m_heightmap_size.y - 1; ++z) {
        for (int x = 0; x < m_heightmap_size.x - 1; ++x) {
            uint32_t i0 = z * m_heightmap_size.x + x;
            uint32_t i1 = z * m_heightmap_size.x + (x + 1);
            uint32_t i2 = (z + 1) * m_heightmap_size.x + x;
            uint32_t i3 = (z + 1) * m_heightmap_size.x + (x + 1);
            
            // First triangle
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            
            // Second triangle
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
}

glm::vec3 TerrainChunk::CalculateNormal(int x, int z) const {
    // Calculate normal using neighboring heights
    float left = GetHeight(std::max(0, x - 1), z);
    float right = GetHeight(std::min(m_heightmap_size.x - 1, x + 1), z);
    float down = GetHeight(x, std::max(0, z - 1));
    float up = GetHeight(x, std::min(m_heightmap_size.y - 1, z + 1));
    
    float scale = 1.0f / m_resolution;
    glm::vec3 normal(left - right, 2.0f * scale, down - up);
    return glm::normalize(normal);
}

int TerrainChunk::GetHeightIndex(int x, int z) const {
    return z * m_heightmap_size.x + x;
}

bool TerrainChunk::IsValidCoordinate(int x, int z) const {
    return x >= 0 && x < m_heightmap_size.x && z >= 0 && z < m_heightmap_size.y;
}

glm::vec3 TerrainChunk::LocalToWorldPosition(int x, int z, float height) const {
    float world_x = m_chunk_x * m_chunk_size + (static_cast<float>(x) / m_resolution);
    float world_z = m_chunk_z * m_chunk_size + (static_cast<float>(z) / m_resolution);
    return glm::vec3(world_x, height, world_z);
}

// TerrainData Implementation

TerrainData::TerrainData(float width, float height, float resolution, float chunk_size)
    : m_width(width)
    , m_height(height)
    , m_resolution(resolution)
    , m_chunk_size(chunk_size)
{
    // Calculate chunk grid size
    m_chunk_grid_size.x = static_cast<int>(std::ceil(width / chunk_size));
    m_chunk_grid_size.y = static_cast<int>(std::ceil(height / chunk_size));
}

std::pair<glm::vec3, glm::vec3> TerrainData::GetBounds() const {
    glm::vec3 min_bounds(0.0f, 0.0f, 0.0f);
    glm::vec3 max_bounds(m_width, 0.0f, m_height);
    
    // Calculate height bounds from all chunks
    float min_height = std::numeric_limits<float>::max();
    float max_height = std::numeric_limits<float>::lowest();
    
    for (const auto& [key, chunk] : m_chunks) {
        auto [chunk_min, chunk_max] = chunk->GetWorldBounds();
        min_height = std::min(min_height, chunk_min.y);
        max_height = std::max(max_height, chunk_max.y);
    }
    
    if (min_height != std::numeric_limits<float>::max()) {
        min_bounds.y = min_height;
        max_bounds.y = max_height;
    }
    
    return {min_bounds, max_bounds};
}

TerrainChunk* TerrainData::GetChunk(int chunk_x, int chunk_z) {
    uint64_t key = GetChunkKey(chunk_x, chunk_z);
    auto it = m_chunks.find(key);
    return (it != m_chunks.end()) ? it->second.get() : nullptr;
}

TerrainChunk* TerrainData::GetChunkAtWorldPos(const glm::vec3& world_pos) {
    if (!IsWithinBounds(world_pos)) return nullptr;
    
    glm::ivec2 chunk_coords = WorldToChunkCoords(world_pos);
    return GetChunk(chunk_coords.x, chunk_coords.y);
}

std::vector<TerrainChunk*> TerrainData::GetAllChunks() {
    std::vector<TerrainChunk*> chunks;
    chunks.reserve(m_chunks.size());
    
    for (const auto& [key, chunk] : m_chunks) {
        chunks.push_back(chunk.get());
    }
    
    return chunks;
}

TerrainChunk* TerrainData::CreateChunk(int chunk_x, int chunk_z) {
    uint64_t key = GetChunkKey(chunk_x, chunk_z);
    auto it = m_chunks.find(key);
    
    if (it == m_chunks.end()) {
        auto chunk = std::make_unique<TerrainChunk>(chunk_x, chunk_z, m_chunk_size, m_resolution);
        TerrainChunk* chunk_ptr = chunk.get();
        m_chunks[key] = std::move(chunk);
        return chunk_ptr;
    }
    
    return it->second.get();
}

void TerrainData::InitializeFlatTerrain(float height, int chunk_count_x, int chunk_count_z) {
    // Clear existing chunks
    m_chunks.clear();

    // Create chunks in a grid pattern centered around origin
    int start_x = -chunk_count_x / 2;
    int start_z = -chunk_count_z / 2;

    for (int z = start_z; z < start_z + chunk_count_z; ++z) {
        for (int x = start_x; x < start_x + chunk_count_x; ++x) {
            // Create chunk
            TerrainChunk* chunk = CreateChunk(x, z);
            if (chunk) {
                // Initialize all height points to the specified height
                glm::ivec2 heightmap_size = chunk->GetHeightMapSize();
                for (int local_z = 0; local_z < heightmap_size.y; ++local_z) {
                    for (int local_x = 0; local_x < heightmap_size.x; ++local_x) {
                        chunk->SetHeight(local_x, local_z, height);
                    }
                }

                // Mark chunk as loaded and clean (not dirty)
                chunk->SetLoaded(true);
                chunk->SetDirty(false);
            }
        }
    }

    std::cout << "TerrainData: Initialized flat terrain with " << chunk_count_x << "x" << chunk_count_z
              << " chunks at height " << height << std::endl;
}

float TerrainData::GetHeightAtWorldPos(const glm::vec3& world_pos) const {
    if (!IsWithinBounds(world_pos)) {
        return std::numeric_limits<float>::quiet_NaN();
    }
    
    glm::ivec2 chunk_coords = WorldToChunkCoords(world_pos);
    uint64_t key = GetChunkKey(chunk_coords.x, chunk_coords.y);
    auto it = m_chunks.find(key);
    
    if (it != m_chunks.end()) {
        glm::vec2 local_coords = WorldToLocalChunkCoords(world_pos);
        return it->second->GetHeightInterpolated(local_coords.x, local_coords.y);
    }
    
    return 0.0f;
}

void TerrainData::SetHeightAtWorldPos(const glm::vec3& world_pos, float height) {
    if (!IsWithinBounds(world_pos)) return;
    
    glm::ivec2 chunk_coords = WorldToChunkCoords(world_pos);
    TerrainChunk* chunk = CreateChunk(chunk_coords.x, chunk_coords.y);
    
    if (chunk) {
        glm::vec2 local_coords = WorldToLocalChunkCoords(world_pos);
        int local_x = static_cast<int>(std::round(local_coords.x));
        int local_z = static_cast<int>(std::round(local_coords.y));
        chunk->SetHeight(local_x, local_z, height);
    }
}

glm::vec3 TerrainData::GetNormalAtWorldPos(const glm::vec3& world_pos) const {
    if (!IsWithinBounds(world_pos)) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }
    
    glm::ivec2 chunk_coords = WorldToChunkCoords(world_pos);
    uint64_t key = GetChunkKey(chunk_coords.x, chunk_coords.y);
    auto it = m_chunks.find(key);
    
    if (it != m_chunks.end()) {
        glm::vec2 local_coords = WorldToLocalChunkCoords(world_pos);
        int local_x = static_cast<int>(std::round(local_coords.x));
        int local_z = static_cast<int>(std::round(local_coords.y));
        return it->second->CalculateNormal(local_x, local_z);
    }
    
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::ivec2 TerrainData::WorldToChunkCoords(const glm::vec3& world_pos) const {
    int chunk_x = static_cast<int>(std::floor(world_pos.x / m_chunk_size));
    int chunk_z = static_cast<int>(std::floor(world_pos.z / m_chunk_size));
    return glm::ivec2(chunk_x, chunk_z);
}

glm::vec2 TerrainData::WorldToLocalChunkCoords(const glm::vec3& world_pos) const {
    glm::ivec2 chunk_coords = WorldToChunkCoords(world_pos);
    float local_x = (world_pos.x - chunk_coords.x * m_chunk_size) * m_resolution;
    float local_z = (world_pos.z - chunk_coords.y * m_chunk_size) * m_resolution;
    return glm::vec2(local_x, local_z);
}

bool TerrainData::IsWithinBounds(const glm::vec3& world_pos) const {
    return world_pos.x >= 0.0f && world_pos.x <= m_width &&
           world_pos.z >= 0.0f && world_pos.z <= m_height;
}

uint64_t TerrainData::GetChunkKey(int chunk_x, int chunk_z) const {
    return (static_cast<uint64_t>(chunk_x) << 32) | static_cast<uint64_t>(chunk_z);
}

glm::ivec2 TerrainData::KeyToChunkCoords(uint64_t key) const {
    int chunk_x = static_cast<int>(key >> 32);
    int chunk_z = static_cast<int>(key & 0xFFFFFFFF);
    return glm::ivec2(chunk_x, chunk_z);
}

} // namespace Lupine
