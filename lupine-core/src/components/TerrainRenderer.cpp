#include "lupine/components/TerrainRenderer.h"
#include "lupine/terrain/TerrainData.h"
#include "lupine/terrain/TerrainBrush.h"
#include "lupine/terrain/TextureBrush.h"
#include "lupine/terrain/AssetScatterBrush.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/resources/ResourceManager.h"
#include <glad/glad.h>
#include <iostream>
#include <algorithm>
#include <random>

namespace Lupine {

// TerrainRenderChunk destructor implementation
TerrainRenderChunk::~TerrainRenderChunk() {
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (EBO != 0) glDeleteBuffers(1, &EBO);
}

TerrainRenderer::TerrainRenderer()
    : Component("TerrainRenderer")
    , m_terrain_data(nullptr)
    , m_chunk_size(64.0f)
    , m_quality_level(QualityLevel::Medium)
    , m_wireframe(false)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_casts_shadows(true)
    , m_receives_shadows(true)
    , m_needs_regeneration(false)
    , m_chunks_dirty(false)
{
    InitializeExportVariables();
}

void TerrainRenderer::SetTerrainData(std::shared_ptr<TerrainData> data) {
    m_terrain_data = data;
    m_needs_regeneration = true;
    UpdateExportVariables();
}

void TerrainRenderer::CreateTerrain(float width, float height, float resolution) {
    // This will be implemented when we have TerrainData structure
    std::cout << "TerrainRenderer: Creating terrain " << width << "x" << height << " with resolution " << resolution << std::endl;
    m_needs_regeneration = true;
}

void TerrainRenderer::SetChunkSize(float size) {
    if (m_chunk_size != size) {
        m_chunk_size = size;
        m_chunks_dirty = true;
        UpdateExportVariables();
    }
}

size_t TerrainRenderer::GetChunkCount() const {
    return m_chunks.size();
}

void TerrainRenderer::RegenerateChunks() {
    m_chunks_dirty = true;
    m_needs_regeneration = true;
}

void TerrainRenderer::ModifyHeight(const glm::vec3& world_pos, float delta, float radius, float falloff) {
    if (!m_terrain_data) return;

    // Create a temporary brush for height modification
    TerrainBrush brush;
    BrushSettings brush_settings;
    brush_settings.size = radius;
    brush_settings.strength = std::abs(delta);
    brush_settings.falloff_curve = falloff;
    brush.SetBrushSettings(brush_settings);

    HeightOperationParams operation_params;
    operation_params.operation = (delta > 0) ? HeightOperation::Raise : HeightOperation::Lower;
    brush.SetOperationParams(operation_params);

    // Apply brush dab
    brush.ApplyBrushDab(world_pos, m_terrain_data.get(), 1.0f);

    m_chunks_dirty = true;
}

void TerrainRenderer::FlattenHeight(const glm::vec3& world_pos, float target_height, float radius, float strength) {
    if (!m_terrain_data) return;

    // Create a temporary brush for flattening
    TerrainBrush brush;
    BrushSettings brush_settings;
    brush_settings.size = radius;
    brush_settings.strength = strength;
    brush.SetBrushSettings(brush_settings);

    HeightOperationParams operation_params;
    operation_params.operation = HeightOperation::Flatten;
    operation_params.target_height = target_height;
    brush.SetOperationParams(operation_params);

    // Apply brush dab
    brush.ApplyBrushDab(world_pos, m_terrain_data.get(), 1.0f);

    m_chunks_dirty = true;
}

void TerrainRenderer::SmoothHeight(const glm::vec3& world_pos, float radius, float strength) {
    if (!m_terrain_data) return;

    // Create a temporary brush for smoothing
    TerrainBrush brush;
    BrushSettings brush_settings;
    brush_settings.size = radius;
    brush_settings.strength = strength;
    brush.SetBrushSettings(brush_settings);

    HeightOperationParams operation_params;
    operation_params.operation = HeightOperation::Smooth;
    brush.SetOperationParams(operation_params);

    // Apply brush dab
    brush.ApplyBrushDab(world_pos, m_terrain_data.get(), 1.0f);

    m_chunks_dirty = true;
}

float TerrainRenderer::GetHeightAtPosition(const glm::vec3& world_pos) const {
    if (!m_terrain_data) return 0.0f;

    return m_terrain_data->GetHeightAtWorldPos(world_pos);
}

glm::vec3 TerrainRenderer::GetNormalAtPosition(const glm::vec3& world_pos) const {
    if (!m_terrain_data) return glm::vec3(0.0f, 1.0f, 0.0f);

    return m_terrain_data->GetNormalAtWorldPos(world_pos);
}

int TerrainRenderer::AddTextureLayer(const std::string& texture_path, float scale, float opacity) {
    TerrainTextureLayer layer(texture_path, scale, opacity);
    m_texture_layers.push_back(layer);
    
    // Load texture
    LoadTextures();
    
    UpdateExportVariables();
    return static_cast<int>(m_texture_layers.size() - 1);
}

void TerrainRenderer::RemoveTextureLayer(int layer_index) {
    if (layer_index >= 0 && layer_index < static_cast<int>(m_texture_layers.size())) {
        m_texture_layers.erase(m_texture_layers.begin() + layer_index);
        UpdateExportVariables();
    }
}

const TerrainTextureLayer& TerrainRenderer::GetTextureLayer(int layer_index) const {
    static TerrainTextureLayer empty_layer;
    if (layer_index >= 0 && layer_index < static_cast<int>(m_texture_layers.size())) {
        return m_texture_layers[layer_index];
    }
    return empty_layer;
}

void TerrainRenderer::PaintTexture(const glm::vec3& world_pos, int layer_index, float opacity, float radius, float falloff) {
    if (!m_terrain_data || layer_index < 0 || layer_index >= static_cast<int>(m_texture_layers.size())) return;

    // Create a temporary texture brush for painting
    TextureBrush texture_brush;

    // Set up brush settings
    BrushSettings brush_settings;
    brush_settings.size = radius;
    brush_settings.strength = opacity;
    brush_settings.falloff_curve = falloff;
    texture_brush.SetBrushSettings(brush_settings);

    // Set up paint parameters
    TexturePaintParams paint_params;
    paint_params.target_layer = layer_index;
    paint_params.opacity = opacity;
    paint_params.blend_mode = TextureBlendMode::Replace;
    texture_brush.SetPaintParams(paint_params);

    // Apply texture dab
    texture_brush.ApplyTextureDab(world_pos, m_terrain_data.get(), 1.0f);

    m_chunks_dirty = true;
}

void TerrainRenderer::ScatterAssets(const glm::vec3& world_pos, const std::vector<std::string>& asset_paths,
                                   float density, float radius, float scale_variance,
                                   float rotation_variance, const glm::vec2& height_offset_range) {
    if (asset_paths.empty() || !m_terrain_data) return;

    // Create a temporary asset scatter brush
    AssetScatterBrush scatter_brush;

    // Set up brush settings
    BrushSettings brush_settings;
    brush_settings.size = radius;
    brush_settings.strength = 1.0f;
    scatter_brush.SetBrushSettings(brush_settings);

    // Set up scatter parameters
    AssetScatterParams scatter_params;
    scatter_params.density = density;
    scatter_params.scale_range = glm::vec2(1.0f - scale_variance, 1.0f + scale_variance);
    scatter_params.rotation_range = glm::vec3(0.0f, rotation_variance * 360.0f, 0.0f);
    scatter_params.height_offset_range = height_offset_range;
    scatter_params.snapping_mode = SurfaceSnappingMode::TerrainNormals;
    scatter_brush.SetScatterParams(scatter_params);

    // Add assets to palette
    for (const std::string& asset_path : asset_paths) {
        ScatterAssetInfo asset_info(asset_path);
        scatter_brush.AddAsset(asset_info);
    }

    // Get the owner node to add scattered assets to
    Node3D* owner_node = dynamic_cast<Node3D*>(GetOwner());
    if (!owner_node) {
        std::cout << "TerrainRenderer: No valid owner node for asset scattering" << std::endl;
        return;
    }

    // Apply scatter dab
    scatter_brush.ApplyScatterDab(world_pos, m_terrain_data.get(), owner_node, 1.0f);

    std::cout << "TerrainRenderer: Scattered assets at (" << world_pos.x << ", " << world_pos.z
              << ") density=" << density << " radius=" << radius << std::endl;

    UpdateExportVariables();
}

void TerrainRenderer::RemoveAssets(const glm::vec3& world_pos, float radius) {
    if (!m_terrain_data) return;

    // Create a temporary asset scatter brush for erasing
    AssetScatterBrush scatter_brush;

    // Get the owner node containing scattered assets
    Node3D* owner_node = dynamic_cast<Node3D*>(GetOwner());
    if (!owner_node) {
        std::cout << "TerrainRenderer: No valid owner node for asset removal" << std::endl;
        return;
    }

    // Erase assets in the specified radius
    scatter_brush.EraseAssets(world_pos, m_terrain_data.get(), owner_node, radius);

    // Also remove from internal asset instances list
    auto it = std::remove_if(m_asset_instances.begin(), m_asset_instances.end(),
        [&](const TerrainAssetInstance& instance) {
            float distance = glm::length(glm::vec2(instance.position.x - world_pos.x, instance.position.z - world_pos.z));
            return distance <= radius;
        });

    size_t removed_count = std::distance(it, m_asset_instances.end());
    m_asset_instances.erase(it, m_asset_instances.end());

    std::cout << "TerrainRenderer: Removed " << removed_count << " assets at (" << world_pos.x << ", " << world_pos.z
              << ") radius=" << radius << std::endl;

    UpdateExportVariables();
}

void TerrainRenderer::SetQualityLevel(QualityLevel quality) {
    if (m_quality_level != quality) {
        m_quality_level = quality;
        m_chunks_dirty = true;
        UpdateExportVariables();
    }
}

void TerrainRenderer::SetWireframe(bool wireframe) {
    m_wireframe = wireframe;
    UpdateExportVariables();
}

void TerrainRenderer::SetColor(const glm::vec4& color) {
    m_color = color;
    UpdateExportVariables();
}

void TerrainRenderer::OnReady() {
    UpdateFromExportVariables();
    if (m_needs_regeneration) {
        GenerateChunks();
    }
}

void TerrainRenderer::OnUpdate(float delta_time) {
    (void)delta_time;
    
    UpdateFromExportVariables();
    
    if (m_chunks_dirty || m_needs_regeneration) {
        GenerateChunks();
        m_chunks_dirty = false;
        m_needs_regeneration = false;
    }
}

void TerrainRenderer::Render() {
    if (!m_terrain_data) return;
    
    // Render terrain chunks
    for (const auto& chunk : m_chunks) {
        if (chunk && chunk->terrain_chunk) {
            UpdateChunk(chunk->terrain_chunk);
        }
    }
    
    // Render scattered assets
    // This will be implemented when we have proper asset rendering
}

void TerrainRenderer::InitializeExportVariables() {
    // Terrain properties
    AddExportVariable("chunk_size", m_chunk_size, "Size of terrain chunks", ExportVariableType::Float);
    
    // Quality settings
    std::vector<std::string> qualityOptions = {"Low", "Medium", "High", "Ultra"};
    AddEnumExportVariable("quality_level", static_cast<int>(m_quality_level), "Rendering quality level", qualityOptions);
    
    // Rendering properties
    AddExportVariable("wireframe", m_wireframe, "Enable wireframe rendering", ExportVariableType::Bool);
    AddExportVariable("color", m_color, "Terrain color modulation", ExportVariableType::Color);
    AddExportVariable("casts_shadows", m_casts_shadows, "Enable shadow casting", ExportVariableType::Bool);
    AddExportVariable("receives_shadows", m_receives_shadows, "Enable shadow receiving", ExportVariableType::Bool);
}

void TerrainRenderer::UpdateFromExportVariables() {
    m_chunk_size = GetExportVariableValue<float>("chunk_size", m_chunk_size);
    m_quality_level = static_cast<QualityLevel>(GetExportVariableValue<int>("quality_level", static_cast<int>(m_quality_level)));
    m_wireframe = GetExportVariableValue<bool>("wireframe", m_wireframe);
    m_color = GetExportVariableValue<glm::vec4>("color", m_color);
    m_casts_shadows = GetExportVariableValue<bool>("casts_shadows", m_casts_shadows);
    m_receives_shadows = GetExportVariableValue<bool>("receives_shadows", m_receives_shadows);
}

void TerrainRenderer::UpdateExportVariables() {
    SetExportVariable("chunk_size", m_chunk_size);
    SetExportVariable("quality_level", static_cast<int>(m_quality_level));
    SetExportVariable("wireframe", m_wireframe);
    SetExportVariable("color", m_color);
    SetExportVariable("casts_shadows", m_casts_shadows);
    SetExportVariable("receives_shadows", m_receives_shadows);
}

void TerrainRenderer::LoadTextures() {
    for (auto& layer : m_texture_layers) {
        if (!layer.texture_path.empty() && layer.texture_id == 0) {
            // Load texture using ResourceManager
            auto texture = ResourceManager::LoadTexture(layer.texture_path);
            layer.texture_id = texture.id;
        }
    }
}

void TerrainRenderer::GenerateChunks() {
    // Clear existing chunks
    m_chunks.clear();

    if (!m_terrain_data) return;

    // Get all terrain chunks from terrain data
    auto terrain_chunks = m_terrain_data->GetAllChunks();

    for (auto* terrain_chunk : terrain_chunks) {
        if (terrain_chunk) {
            // Create a render chunk for each terrain chunk
            auto render_chunk = std::make_unique<TerrainRenderChunk>();
            render_chunk->terrain_chunk = terrain_chunk;
            render_chunk->needs_update = true;
            render_chunk->VAO = 0;
            render_chunk->VBO = 0;
            render_chunk->EBO = 0;
            render_chunk->vertex_count = 0;
            render_chunk->index_count = 0;

            m_chunks.push_back(std::move(render_chunk));
        }
    }

    std::cout << "TerrainRenderer: Generated " << m_chunks.size() << " render chunks" << std::endl;
}

void TerrainRenderer::UpdateChunk(TerrainChunk* chunk) {
    if (!chunk) return;

    // Find the render chunk for this terrain chunk
    TerrainRenderChunk* render_chunk = nullptr;
    for (auto& rc : m_chunks) {
        if (rc->terrain_chunk == chunk) {
            render_chunk = rc.get();
            break;
        }
    }

    if (!render_chunk) return;

    // Generate mesh data from terrain chunk
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    chunk->GenerateMesh(vertices, indices, true, true);

    if (vertices.empty() || indices.empty()) return;

    // Convert to OpenGL format (position, normal, uv)
    std::vector<float> gl_vertices;
    gl_vertices.reserve(vertices.size() * 8); // 3 pos + 3 normal + 2 uv

    for (size_t i = 0; i < vertices.size(); i += 3) {
        // Position
        gl_vertices.push_back(vertices[i].x);
        gl_vertices.push_back(vertices[i].y);
        gl_vertices.push_back(vertices[i].z);

        // Normal (every 3rd vertex in the array)
        if (i + 1 < vertices.size()) {
            gl_vertices.push_back(vertices[i + 1].x);
            gl_vertices.push_back(vertices[i + 1].y);
            gl_vertices.push_back(vertices[i + 1].z);
        } else {
            gl_vertices.push_back(0.0f);
            gl_vertices.push_back(1.0f);
            gl_vertices.push_back(0.0f);
        }

        // UV coordinates (every 3rd vertex + 2 in the array)
        if (i + 2 < vertices.size()) {
            gl_vertices.push_back(vertices[i + 2].x);
            gl_vertices.push_back(vertices[i + 2].y);
        } else {
            gl_vertices.push_back(0.0f);
            gl_vertices.push_back(0.0f);
        }
    }

    // Create OpenGL buffers if they don't exist
    if (render_chunk->VAO == 0) {
        glGenVertexArrays(1, &render_chunk->VAO);
        glGenBuffers(1, &render_chunk->VBO);
        glGenBuffers(1, &render_chunk->EBO);
    }

    // Upload mesh data to GPU
    glBindVertexArray(render_chunk->VAO);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, render_chunk->VBO);
    glBufferData(GL_ARRAY_BUFFER, gl_vertices.size() * sizeof(float), gl_vertices.data(), GL_STATIC_DRAW);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render_chunk->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Set vertex attributes
    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // UV coordinates (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Update render chunk state
    render_chunk->vertex_count = static_cast<int>(gl_vertices.size() / 8);
    render_chunk->index_count = static_cast<int>(indices.size());
    render_chunk->needs_update = false;

    std::cout << "TerrainRenderer: Updated chunk with " << render_chunk->vertex_count
              << " vertices and " << render_chunk->index_count << " indices" << std::endl;
}

void TerrainRenderer::UpdateAllDirtyChunks() {
    if (!m_terrain_data) return;

    auto terrain_chunks = m_terrain_data->GetAllChunks();
    for (auto* chunk : terrain_chunks) {
        if (chunk && chunk->IsDirty()) {
            UpdateChunk(chunk);
        }
    }
}

glm::vec2 TerrainRenderer::WorldToTerrainCoords(const glm::vec3& world_pos) const {
    // Convert world coordinates to terrain texture coordinates
    // This will be properly implemented when we have terrain bounds
    return glm::vec2(world_pos.x, world_pos.z);
}

glm::vec3 TerrainRenderer::TerrainToWorldCoords(const glm::vec2& terrain_coords) const {
    // Convert terrain coordinates to world coordinates
    // This will be properly implemented when we have terrain bounds
    return glm::vec3(terrain_coords.x, 0.0f, terrain_coords.y);
}

float TerrainRenderer::CalculateBrushWeight(float distance, float radius, float falloff) const {
    if (distance >= radius) return 0.0f;
    
    float normalized_distance = distance / radius;
    
    // Apply falloff curve
    if (falloff <= 0.0f) {
        // Linear falloff
        return 1.0f - normalized_distance;
    } else {
        // Smooth falloff
        return glm::pow(1.0f - normalized_distance, 1.0f + falloff * 3.0f);
    }
}

} // namespace Lupine
