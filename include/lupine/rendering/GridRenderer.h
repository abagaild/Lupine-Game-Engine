#pragma once

#ifndef GRIDRENDERER_H
#define GRIDRENDERER_H

#include <glm/glm.hpp>
#include <memory>

namespace Lupine {

class GraphicsShader;
class Camera;

/**
 * @brief Grid configuration structure
 */
struct GridConfig {
    // Grid appearance
    glm::vec4 majorLineColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);  // Major grid lines
    glm::vec4 minorLineColor = glm::vec4(0.3f, 0.3f, 0.3f, 0.6f);  // Minor grid lines
    glm::vec4 axisLineColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);   // Axis lines (X/Y/Z)
    glm::vec4 gameBoundsColor = glm::vec4(0.2f, 0.8f, 0.2f, 1.0f); // Game bounds border

    // Grid spacing
    float majorSpacing = 10.0f;     // Distance between major grid lines
    float minorSpacing = 1.0f;      // Distance between minor grid lines

    // Grid size
    float gridSize = 100.0f;        // Total size of the grid

    // Grid offset (for infinite grids)
    glm::vec2 gridOffset = glm::vec2(0.0f, 0.0f);           // Offset from origin

    // Game bounds (default rendering area)
    glm::vec2 gameBoundsSize = glm::vec2(1920.0f, 1080.0f);  // Default game resolution
    glm::vec2 gameBoundsOffset = glm::vec2(0.0f, 0.0f);      // Offset from center

    // Visibility flags
    bool showMajorLines = true;
    bool showMinorLines = true;
    bool showAxisLines = true;
    bool showGameBounds = true;     // Show game bounds border
    bool fadeWithDistance = true;   // Fade grid lines based on distance from camera

    // 3D specific settings
    bool showXZPlane = true;        // Show horizontal grid plane
    bool showXYPlane = false;       // Show front grid plane
    bool showYZPlane = false;       // Show side grid plane
};

/**
 * @brief Grid renderer for 2D and 3D scene views
 */
class GridRenderer {
public:
    /**
     * @brief Initialize the grid renderer
     * @return True if successful
     */
    static bool Initialize();

    /**
     * @brief Shutdown the grid renderer
     */
    static void Shutdown();

    /**
     * @brief Render 2D grid
     * @param camera Camera to render from
     * @param config Grid configuration
     */
    static void Render2DGrid(Camera* camera, const GridConfig& config = GridConfig{});

    /**
     * @brief Render 3D perspective grid
     * @param camera Camera to render from
     * @param config Grid configuration
     */
    static void Render3DGrid(Camera* camera, const GridConfig& config = GridConfig{});

    /**
     * @brief Get default grid configuration
     * @return Default grid config
     */
    static GridConfig GetDefaultConfig();

    /**
     * @brief Get default 2D grid configuration with larger spacing
     * @return Default 2D grid config
     */
    static GridConfig GetDefault2DConfig();

    /**
     * @brief Get default 3D grid configuration with original spacing
     * @return Default 3D grid config
     */
    static GridConfig GetDefault3DConfig();

    /**
     * @brief Check if grid renderer is initialized
     * @return True if initialized
     */
    static bool IsInitialized() { return s_initialized; }

    /**
     * @brief Check if OpenGL resources are valid
     * @return True if resources are valid
     */
    static bool AreResourcesValid();

private:
    static bool s_initialized;
    static std::shared_ptr<GraphicsShader> s_grid_shader;
    
    // 2D grid geometry
    static unsigned int s_2d_grid_VAO;
    static unsigned int s_2d_grid_VBO;
    static unsigned int s_2d_grid_vertex_count;
    
    // 3D grid geometry
    static unsigned int s_3d_grid_VAO;
    static unsigned int s_3d_grid_VBO;
    static unsigned int s_3d_grid_vertex_count;

    /**
     * @brief Create grid shader
     */
    static void CreateGridShader();

    /**
     * @brief Generate 2D grid geometry
     * @param config Grid configuration
     */
    static void Generate2DGridGeometry(const GridConfig& config);

    /**
     * @brief Generate 3D grid geometry
     * @param config Grid configuration
     */
    static void Generate3DGridGeometry(const GridConfig& config);

    /**
     * @brief Update dynamic grid bounds based on camera
     * @param camera Camera to calculate bounds from
     * @param config Grid configuration to update
     */
    static void UpdateDynamicGridBounds(Camera* camera, GridConfig& config);

    /**
     * @brief Update grid geometry if needed
     * @param config Grid configuration
     */
    static void UpdateGridGeometry(const GridConfig& config);
};

} // namespace Lupine

#endif // GRIDRENDERER_H
