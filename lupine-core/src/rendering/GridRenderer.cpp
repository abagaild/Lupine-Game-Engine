#include "lupine/rendering/GridRenderer.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/Camera.h"
#include "lupine/rendering/GraphicsDevice.h"
#include "lupine/rendering/GraphicsShader.h"
#include <glad/glad.h>
#include <vector>
#include <iostream>
#include <cmath>

namespace Lupine {

// Static member definitions
bool GridRenderer::s_initialized = false;
std::shared_ptr<GraphicsShader> GridRenderer::s_grid_shader = nullptr;
unsigned int GridRenderer::s_2d_grid_VAO = 0;
unsigned int GridRenderer::s_2d_grid_VBO = 0;
unsigned int GridRenderer::s_2d_grid_vertex_count = 0;
unsigned int GridRenderer::s_3d_grid_VAO = 0;
unsigned int GridRenderer::s_3d_grid_VBO = 0;
unsigned int GridRenderer::s_3d_grid_vertex_count = 0;

bool GridRenderer::Initialize() {
    if (s_initialized) {
        return true;
    }

    std::cout << "Initializing GridRenderer..." << std::endl;

    // Create grid shader
    CreateGridShader();

    // Generate default grid geometry
    GridConfig defaultConfig = GetDefaultConfig();
    Generate2DGridGeometry(defaultConfig);
    Generate3DGridGeometry(defaultConfig);

    s_initialized = true;
    std::cout << "GridRenderer initialized successfully!" << std::endl;
    return true;
}

void GridRenderer::Shutdown() {
    if (!s_initialized) {
        return;
    }

    std::cout << "Shutting down GridRenderer..." << std::endl;

    s_grid_shader.reset();

    // Clean up OpenGL resources
    if (s_2d_grid_VAO != 0) {
        glDeleteVertexArrays(1, &s_2d_grid_VAO);
        glDeleteBuffers(1, &s_2d_grid_VBO);
        s_2d_grid_VAO = 0;
        s_2d_grid_VBO = 0;
    }

    if (s_3d_grid_VAO != 0) {
        glDeleteVertexArrays(1, &s_3d_grid_VAO);
        glDeleteBuffers(1, &s_3d_grid_VBO);
        s_3d_grid_VAO = 0;
        s_3d_grid_VBO = 0;
    }

    s_initialized = false;
    std::cout << "GridRenderer shutdown complete." << std::endl;
}

void GridRenderer::Render2DGrid(Camera* camera, const GridConfig& config) {
    if (!camera) {
        return;
    }

    // Get graphics device from renderer
    auto graphics_device = Renderer::GetGraphicsDevice();
    if (!graphics_device) {
        std::cerr << "GridRenderer: No graphics device available" << std::endl;
        return;
    }

    // Check if we need to reinitialize (handles OpenGL context loss)
    if (!s_initialized || !s_grid_shader || s_2d_grid_VAO == 0) {
        std::cout << "GridRenderer: Reinitializing due to lost resources..." << std::endl;
        s_initialized = false; // Force full reinitialization
        Initialize();
    }

    if (!s_initialized || !s_grid_shader || !s_grid_shader->IsValid()) {
        std::cout << "GridRenderer: Failed to initialize, skipping render" << std::endl;
        return;
    }

    // Create dynamic grid config based on camera
    GridConfig dynamicConfig = config;
    UpdateDynamicGridBounds(camera, dynamicConfig);

    // Update geometry with dynamic bounds
    UpdateGridGeometry(dynamicConfig);

    // Set graphics device state for grid rendering
    graphics_device->SetDepthWrite(false);  // Disable depth writing but keep depth testing for proper layering
    graphics_device->SetBlending(true);     // Enable blending for transparency

    // Use grid shader
    s_grid_shader->Use();

    // Set uniforms
    glm::mat4 model = glm::mat4(1.0f);
    s_grid_shader->SetMat4("model", model);
    s_grid_shader->SetMat4("view", camera->GetViewMatrix());
    s_grid_shader->SetMat4("projection", camera->GetProjectionMatrix());

    // Set consistent line width (still need OpenGL call for this)
    glLineWidth(1.5f);

    // Render 2D grid
    if (s_2d_grid_VAO != 0 && s_2d_grid_vertex_count > 0) {
        glBindVertexArray(s_2d_grid_VAO);
        glDrawArrays(GL_LINES, 0, s_2d_grid_vertex_count);
        glBindVertexArray(0);
    }

    // Reset line width and restore state
    glLineWidth(1.0f);
    graphics_device->SetDepthWrite(true);   // Restore depth writing
    graphics_device->SetBlending(false);    // Disable blending
}

void GridRenderer::Render3DGrid(Camera* camera, const GridConfig& config) {
    if (!camera) {
        return;
    }

    // Get graphics device from renderer
    auto graphics_device = Renderer::GetGraphicsDevice();
    if (!graphics_device) {
        std::cerr << "GridRenderer: No graphics device available" << std::endl;
        return;
    }

    // Check if we need to reinitialize (handles OpenGL context loss)
    if (!s_initialized || !s_grid_shader || s_3d_grid_VAO == 0) {
        std::cout << "GridRenderer: Reinitializing 3D grid due to lost resources..." << std::endl;
        s_initialized = false; // Force full reinitialization
        Initialize();
    }

    if (!s_initialized || !s_grid_shader || !s_grid_shader->IsValid()) {
        std::cout << "GridRenderer: Failed to initialize 3D grid, skipping render" << std::endl;
        return;
    }

    // Create dynamic grid config based on camera
    GridConfig dynamicConfig = config;
    UpdateDynamicGridBounds(camera, dynamicConfig);

    // Update geometry with dynamic bounds
    UpdateGridGeometry(dynamicConfig);

    // Set graphics device state for grid rendering
    graphics_device->SetDepthWrite(false);  // Disable depth writing but keep depth testing for proper layering
    graphics_device->SetBlending(true);     // Enable blending for transparency

    // Use grid shader
    s_grid_shader->Use();

    // Set uniforms
    glm::mat4 model = glm::mat4(1.0f);
    s_grid_shader->SetMat4("model", model);
    s_grid_shader->SetMat4("view", camera->GetViewMatrix());
    s_grid_shader->SetMat4("projection", camera->GetProjectionMatrix());

    // No fading - just set basic uniforms

    // Set consistent line width
    glLineWidth(1.5f);

    // Render 3D grid
    if (s_3d_grid_VAO != 0 && s_3d_grid_vertex_count > 0) {
        glBindVertexArray(s_3d_grid_VAO);
        glDrawArrays(GL_LINES, 0, s_3d_grid_vertex_count);
        glBindVertexArray(0);
    }

    // Reset line width and restore state
    glLineWidth(1.0f);
    graphics_device->SetDepthWrite(true);   // Restore depth writing
    graphics_device->SetBlending(false);    // Disable blending
}

GridConfig GridRenderer::GetDefaultConfig() {
    GridConfig config;
    // Use subtle pastel pink colors
    config.majorLineColor = glm::vec4(0.9f, 0.7f, 0.8f, 0.8f);  // Pastel pink major lines
    config.minorLineColor = glm::vec4(0.8f, 0.6f, 0.7f, 0.5f);  // Lighter pastel pink minor lines
    config.axisLineColor = glm::vec4(1.0f, 0.8f, 0.9f, 1.0f);   // Bright pastel pink axis lines
    config.gameBoundsColor = glm::vec4(0.2f, 0.8f, 0.2f, 1.0f); // Green game bounds

    // Ensure all line types are visible
    config.showMajorLines = true;
    config.showMinorLines = true;
    config.showAxisLines = true;
    config.showGameBounds = true;
    config.fadeWithDistance = false;  // Disable fading

    return config;
}

GridConfig GridRenderer::GetDefault2DConfig() {
    GridConfig config = GetDefaultConfig();

    // Much larger spacing for 2D view
    config.majorSpacing = 200.0f;    // Even larger spacing for major lines
    config.minorSpacing = 50.0f;     // Larger spacing for minor lines

    return config;
}

GridConfig GridRenderer::GetDefault3DConfig() {
    GridConfig config = GetDefaultConfig();

    // Original smaller spacing for 3D view
    config.majorSpacing = 10.0f;     // Original major spacing
    config.minorSpacing = 1.0f;      // Original minor spacing

    return config;
}

bool GridRenderer::AreResourcesValid() {
    if (!s_initialized) {
        return false;
    }

    // Check if OpenGL resources are still valid
    if (s_2d_grid_VAO == 0 || s_3d_grid_VAO == 0) {
        return false;
    }

    // Check if VAOs are still valid OpenGL objects
    if (!glIsVertexArray(s_2d_grid_VAO) || !glIsVertexArray(s_3d_grid_VAO)) {
        return false;
    }

    // Check if shader is still valid
    if (!s_grid_shader || !s_grid_shader->IsValid()) {
        return false;
    }

    return true;
}

void GridRenderer::CreateGridShader() {
    std::string vertex_shader = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec4 aColor;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec4 vertexColor;

        void main() {
            vec4 worldPos = model * vec4(aPos, 1.0);
            gl_Position = projection * view * worldPos;

            // Pass through color unchanged - no lighting calculations
            vertexColor = aColor;
        }
    )";

    std::string fragment_shader = R"(
        #version 330 core
        in vec4 vertexColor;

        out vec4 FragColor;

        void main() {
            // Use vertex color directly - no fading, no lighting, no shadow effects
            FragColor = vertexColor;
        }
    )";

    auto graphics_device = Renderer::GetGraphicsDevice();
    if (graphics_device) {
        s_grid_shader = graphics_device->CreateShader(vertex_shader, fragment_shader);
    }
}

void GridRenderer::Generate2DGridGeometry(const GridConfig& config) {
    std::vector<float> vertices;

    float halfSize = config.gridSize * 0.5f;

    // Apply grid offset for infinite grid positioning
    float centerX = config.gridOffset.x;
    float centerY = config.gridOffset.y;

    // Use fixed spacing for now to ensure visibility
    float adaptiveMinorSpacing = config.minorSpacing;
    float adaptiveMajorSpacing = config.majorSpacing;

    // Generate vertical lines
    for (float x = centerX - halfSize; x <= centerX + halfSize; x += adaptiveMinorSpacing) {
        bool isMajor = (fmod(abs(x - centerX), adaptiveMajorSpacing) < 0.001f);
        bool isAxis = (abs(x) < 0.001f);

        glm::vec4 color = config.minorLineColor;
        if (isAxis && config.showAxisLines) {
            color = config.axisLineColor;
        } else if (isMajor && config.showMajorLines) {
            color = config.majorLineColor;
        } else if (!config.showMinorLines) {
            continue;
        }

        // Line from bottom to top
        vertices.insert(vertices.end(), {x, centerY - halfSize, 0.0f, color.r, color.g, color.b, color.a});
        vertices.insert(vertices.end(), {x, centerY + halfSize, 0.0f, color.r, color.g, color.b, color.a});
    }

    // Generate horizontal lines
    for (float y = centerY - halfSize; y <= centerY + halfSize; y += adaptiveMinorSpacing) {
        bool isMajor = (fmod(abs(y - centerY), adaptiveMajorSpacing) < 0.001f);
        bool isAxis = (abs(y) < 0.001f);

        glm::vec4 color = config.minorLineColor;
        if (isAxis && config.showAxisLines) {
            color = config.axisLineColor;
        } else if (isMajor && config.showMajorLines) {
            color = config.majorLineColor;
        } else if (!config.showMinorLines) {
            continue;
        }

        // Line from left to right
        vertices.insert(vertices.end(), {centerX - halfSize, y, 0.0f, color.r, color.g, color.b, color.a});
        vertices.insert(vertices.end(), {centerX + halfSize, y, 0.0f, color.r, color.g, color.b, color.a});
    }

    // Add game bounds border if enabled
    if (config.showGameBounds) {
        glm::vec4 boundsColor = config.gameBoundsColor;
        float halfWidth = config.gameBoundsSize.x * 0.5f;
        float halfHeight = config.gameBoundsSize.y * 0.5f;
        float offsetX = config.gameBoundsOffset.x;
        float offsetY = config.gameBoundsOffset.y;

        // Bottom edge
        vertices.insert(vertices.end(), {offsetX - halfWidth, offsetY - halfHeight, 0.0f, boundsColor.r, boundsColor.g, boundsColor.b, boundsColor.a});
        vertices.insert(vertices.end(), {offsetX + halfWidth, offsetY - halfHeight, 0.0f, boundsColor.r, boundsColor.g, boundsColor.b, boundsColor.a});

        // Top edge
        vertices.insert(vertices.end(), {offsetX - halfWidth, offsetY + halfHeight, 0.0f, boundsColor.r, boundsColor.g, boundsColor.b, boundsColor.a});
        vertices.insert(vertices.end(), {offsetX + halfWidth, offsetY + halfHeight, 0.0f, boundsColor.r, boundsColor.g, boundsColor.b, boundsColor.a});

        // Left edge
        vertices.insert(vertices.end(), {offsetX - halfWidth, offsetY - halfHeight, 0.0f, boundsColor.r, boundsColor.g, boundsColor.b, boundsColor.a});
        vertices.insert(vertices.end(), {offsetX - halfWidth, offsetY + halfHeight, 0.0f, boundsColor.r, boundsColor.g, boundsColor.b, boundsColor.a});

        // Right edge
        vertices.insert(vertices.end(), {offsetX + halfWidth, offsetY - halfHeight, 0.0f, boundsColor.r, boundsColor.g, boundsColor.b, boundsColor.a});
        vertices.insert(vertices.end(), {offsetX + halfWidth, offsetY + halfHeight, 0.0f, boundsColor.r, boundsColor.g, boundsColor.b, boundsColor.a});
    }

    s_2d_grid_vertex_count = static_cast<unsigned int>(vertices.size() / 7); // 3 pos + 4 color = 7 floats per vertex

    // Create VAO and VBO
    if (s_2d_grid_VAO == 0) {
        glGenVertexArrays(1, &s_2d_grid_VAO);
        glGenBuffers(1, &s_2d_grid_VBO);
    }

    glBindVertexArray(s_2d_grid_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_2d_grid_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void GridRenderer::Generate3DGridGeometry(const GridConfig& config) {
    std::vector<float> vertices;

    float halfSize = config.gridSize * 0.5f;

    // Apply grid offset for infinite grid positioning
    float centerX = config.gridOffset.x;
    float centerZ = config.gridOffset.y; // Use Y component for Z in 3D

    // Generate XZ plane grid (horizontal)
    if (config.showXZPlane) {
        // Lines parallel to X axis (varying Z)
        for (float z = centerZ - halfSize; z <= centerZ + halfSize; z += config.minorSpacing) {
            bool isMajor = (fmod(abs(z - centerZ), config.majorSpacing) < 0.001f);
            bool isAxis = (abs(z) < 0.001f);

            glm::vec4 color = config.minorLineColor;
            if (isAxis && config.showAxisLines) {
                color = config.axisLineColor;
            } else if (isMajor && config.showMajorLines) {
                color = config.majorLineColor;
            } else if (!config.showMinorLines) {
                continue;
            }

            vertices.insert(vertices.end(), {centerX - halfSize, 0.0f, z, color.r, color.g, color.b, color.a});
            vertices.insert(vertices.end(), {centerX + halfSize, 0.0f, z, color.r, color.g, color.b, color.a});
        }

        // Lines parallel to Z axis (varying X)
        for (float x = centerX - halfSize; x <= centerX + halfSize; x += config.minorSpacing) {
            bool isMajor = (fmod(abs(x - centerX), config.majorSpacing) < 0.001f);
            bool isAxis = (abs(x) < 0.001f);

            glm::vec4 color = config.minorLineColor;
            if (isAxis && config.showAxisLines) {
                color = config.axisLineColor;
            } else if (isMajor && config.showMajorLines) {
                color = config.majorLineColor;
            } else if (!config.showMinorLines) {
                continue;
            }

            vertices.insert(vertices.end(), {x, 0.0f, centerZ - halfSize, color.r, color.g, color.b, color.a});
            vertices.insert(vertices.end(), {x, 0.0f, centerZ + halfSize, color.r, color.g, color.b, color.a});
        }
    }

    s_3d_grid_vertex_count = static_cast<unsigned int>(vertices.size() / 7); // 3 pos + 4 color = 7 floats per vertex

    // Create VAO and VBO
    if (s_3d_grid_VAO == 0) {
        glGenVertexArrays(1, &s_3d_grid_VAO);
        glGenBuffers(1, &s_3d_grid_VBO);
    }

    glBindVertexArray(s_3d_grid_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_3d_grid_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void GridRenderer::UpdateDynamicGridBounds(Camera* camera, GridConfig& config) {
    if (!camera) return;

    // Calculate visible area based on camera type and position
    if (camera->GetProjectionType() == ProjectionType::Orthographic) {
        // For orthographic (2D) camera, calculate bounds from projection matrix
        float left, right, bottom, top, near_plane, far_plane;
        camera->GetOrthographicBounds(left, right, bottom, top, near_plane, far_plane);

        float width = right - left;
        float height = top - bottom;

        // Expand grid to cover visible area plus some padding
        float padding = std::max(width, height) * 0.5f;
        config.gridSize = std::max(width, height) + padding * 2.0f;

        // Center grid around camera view
        glm::vec3 cameraPos = camera->GetPosition();
        config.gridOffset = glm::vec2(left + width * 0.5f, bottom + height * 0.5f);
    } else {
        // For perspective (3D) camera, calculate bounds based on distance and FOV
        glm::vec3 cameraPos = camera->GetPosition();
        float distance = glm::length(cameraPos);

        // Calculate grid size based on camera distance
        config.gridSize = std::max(100.0f, distance * 2.0f);

        // Center grid around origin for 3D
        config.gridOffset = glm::vec2(0.0f, 0.0f);
    }
}

void GridRenderer::UpdateGridGeometry(const GridConfig& config) {
    // For now, we'll regenerate geometry each time
    // In the future, we could cache based on config hash
    static GridConfig lastConfig;

    // Simple comparison - in practice you'd want a proper equality operator
    bool configChanged = (lastConfig.gridSize != config.gridSize ||
                         lastConfig.majorSpacing != config.majorSpacing ||
                         lastConfig.minorSpacing != config.minorSpacing ||
                         lastConfig.showGameBounds != config.showGameBounds ||
                         lastConfig.gameBoundsSize.x != config.gameBoundsSize.x ||
                         lastConfig.gameBoundsSize.y != config.gameBoundsSize.y ||
                         lastConfig.gameBoundsOffset.x != config.gameBoundsOffset.x ||
                         lastConfig.gameBoundsOffset.y != config.gameBoundsOffset.y ||
                         lastConfig.gridOffset.x != config.gridOffset.x ||
                         lastConfig.gridOffset.y != config.gridOffset.y);

    if (configChanged) {
        Generate2DGridGeometry(config);
        Generate3DGridGeometry(config);
        lastConfig = config;
    }
}

} // namespace Lupine
