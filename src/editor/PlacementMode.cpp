// Include glad before any OpenGL usage
#include <glad/glad.h>

#include "PlacementMode.h"
#include "panels/SceneViewPanel.h"
#include "lupine/resources/MeshLoader.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/Camera.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/physics/PhysicsManager.h"
#include <QFileInfo>
#include <iostream>
#include <cmath>

PlacementMode::PlacementMode(QObject* parent)
    : QObject(parent)
{
}

PlacementMode::~PlacementMode()
{
    clearGhostModel();
}

void PlacementMode::setConfig(const PlacementConfig& config)
{
    m_config = config;
    emit configChanged();
}

void PlacementMode::setEnabled(bool enabled)
{
    if (m_config.enabled != enabled) {
        m_config.enabled = enabled;
        if (!enabled) {
            cancelPlacement();
        }
        emit configChanged();
    }
}

void PlacementMode::setPlacementModeType(PlacementModeType mode)
{
    m_config.placementMode = mode;
    emit configChanged();
}

void PlacementMode::setSurfaceDetectionType(SurfaceDetectionType type)
{
    m_config.surfaceDetection = type;
    emit configChanged();
}

void PlacementMode::setGridSnapEnabled(bool enabled)
{
    m_config.gridSnapEnabled = enabled;
    emit configChanged();
}

void PlacementMode::setGridSnapYEnabled(bool enabled)
{
    m_config.gridSnapYEnabled = enabled;
    emit configChanged();
}

void PlacementMode::setGridSize(float size)
{
    m_config.gridSize = std::max(0.1f, size);
    emit configChanged();
}

void PlacementMode::setPlacementGridY(float y)
{
    m_config.placementGridY = y;
    emit configChanged();
}

void PlacementMode::setSurfaceSnapEnabled(bool enabled)
{
    m_config.surfaceSnapEnabled = enabled;
    emit configChanged();
}

void PlacementMode::setSurfaceSnapTolerance(float tolerance)
{
    m_config.surfaceSnapTolerance = std::max(1.0f, tolerance);
    emit configChanged();
}

void PlacementMode::setAlignToSurfaceNormal(bool align)
{
    m_config.alignToSurfaceNormal = align;
    emit configChanged();
}

void PlacementMode::setSnapToSurfaceNode(Lupine::Node* node)
{
    m_config.snapToSurfaceNode = node;
    emit configChanged();
}

void PlacementMode::setGhostOpacity(float opacity)
{
    m_config.ghostOpacity = std::clamp(opacity, 0.0f, 1.0f);
    emit configChanged();
}

void PlacementMode::setDefault2DSpriteType(const QString& type)
{
    m_config.default2DSpriteType = type;
    emit configChanged();
}

void PlacementMode::setDefault3DSpriteType(const QString& type)
{
    m_config.default3DSpriteType = type;
    emit configChanged();
}

void PlacementMode::setDefault3DModelType(const QString& type)
{
    m_config.default3DModelType = type;
    emit configChanged();
}

void PlacementMode::startPlacement(const QString& filePath, const QPoint& screenPos, SceneViewPanel* sceneView)
{
    if (!m_config.enabled || !sceneView) {
        return;
    }
    
    m_isPlacing = true;
    m_currentFilePath = filePath;
    m_sceneView = sceneView;
    m_showGhost = false; // Will be shown when Ctrl is pressed
    
    // Load ghost model/texture for preview
    loadGhostModel(filePath);
    
    emit placementStarted(filePath);
}

void PlacementMode::updatePlacement(const QPoint& screenPos, bool ctrlPressed)
{
    if (!m_isPlacing || !m_sceneView) {
        return;
    }
    
    // Show/hide ghost based on Ctrl key
    m_showGhost = ctrlPressed;
    
    if (m_showGhost) {
        // Convert screen position to world position
        glm::vec3 worldPos = m_sceneView->screenToWorldPosition(screenPos);
        
        // Apply snapping
        worldPos = snapPosition(worldPos, m_sceneView->getViewMode() == SceneViewPanel::ViewMode::Mode2D);
        
        updateGhostPosition(worldPos);
    }
}

void PlacementMode::endPlacement()
{
    if (m_isPlacing) {
        m_isPlacing = false;
        m_showGhost = false;
        m_sceneView = nullptr;
        clearGhostModel();
        emit placementEnded();
    }
}

void PlacementMode::cancelPlacement()
{
    if (m_isPlacing) {
        m_isPlacing = false;
        m_showGhost = false;
        m_sceneView = nullptr;
        clearGhostModel();
        emit placementCancelled();
    }
}

glm::vec3 PlacementMode::snapPosition(const glm::vec3& worldPos, bool is2D)
{
    return applyPlacementMode(worldPos, is2D);
}

glm::vec3 PlacementMode::applyPlacementMode(const glm::vec3& worldPos, bool is2D)
{
    glm::vec3 snappedPos = worldPos;

    switch (m_config.placementMode) {
        case PlacementModeType::FreePlace:
            // No snapping, return original position
            break;

        case PlacementModeType::GridSnap:
            if (m_config.gridSnapEnabled) {
                snappedPos = snapToGrid(snappedPos, is2D);
            }
            break;

        case PlacementModeType::SurfaceSnap:
            if (m_config.surfaceSnapEnabled) {
                snappedPos = snapToSurface(snappedPos);
            }
            break;

        case PlacementModeType::Combined:
            // Apply surface snapping first to get proper ground position
            if (m_config.surfaceSnapEnabled) {
                snappedPos = snapToSurface(snappedPos);
            }
            // Then apply grid snapping (respects surface snapping)
            if (m_config.gridSnapEnabled) {
                snappedPos = snapToGrid(snappedPos, is2D);
            }
            break;
    }

    // Apply Y-axis grid snapping (overrides other Y positioning if enabled)
    if (m_config.gridSnapYEnabled && !is2D) {
        float bottomOffset = calculateObjectBottomOffset();
        snappedPos.y = m_config.placementGridY + bottomOffset;
    }

    return snappedPos;
}

glm::vec3 PlacementMode::snapToGrid(const glm::vec3& worldPos, bool is2D)
{
    glm::vec3 snapped = worldPos;
    float gridSize = m_config.gridSize;

    if (gridSize <= 0.0f) {
        return worldPos; // Invalid grid size, return original position
    }

    // Snap X and Z (or X and Y for 2D)
    snapped.x = std::round(worldPos.x / gridSize) * gridSize;

    if (is2D) {
        // For 2D mode, snap Y coordinate
        snapped.y = std::round(worldPos.y / gridSize) * gridSize;
    } else {
        // For 3D mode, snap Z coordinate
        snapped.z = std::round(worldPos.z / gridSize) * gridSize;

        // Only snap Y if Y-axis grid snapping is NOT enabled (to avoid conflicts)
        if (!m_config.gridSnapYEnabled) {
            // If surface snapping is enabled, preserve the Y from surface snapping
            // Otherwise, snap Y to grid as well
            if (!m_config.surfaceSnapEnabled || !m_config.snapToSurfaceNode) {
                snapped.y = std::round(worldPos.y / gridSize) * gridSize;
            }
        }
    }

    return snapped;
}

glm::vec3 PlacementMode::snapToSurface(const glm::vec3& worldPos)
{
    glm::vec3 surfacePos = worldPos;

    // Try different surface detection methods based on configuration
    switch (m_config.surfaceDetection) {
        case SurfaceDetectionType::Physics:
            surfacePos = detectSurfaceWithPhysics(worldPos);
            break;
        case SurfaceDetectionType::Terrain:
            surfacePos = detectSurfaceWithTerrain(worldPos);
            break;
        case SurfaceDetectionType::Mesh:
            surfacePos = detectSurfaceWithMesh(worldPos);
            break;
        case SurfaceDetectionType::All:
            // Try physics first, then terrain, then mesh
            surfacePos = detectSurfaceWithPhysics(worldPos);
            if (surfacePos == worldPos) {
                surfacePos = detectSurfaceWithTerrain(worldPos);
            }
            if (surfacePos == worldPos) {
                surfacePos = detectSurfaceWithMesh(worldPos);
            }
            break;
    }

    return surfacePos;
}

glm::vec3 PlacementMode::detectSurfaceWithPhysics(const glm::vec3& worldPos)
{
    // Cast ray downward from the world position to find surface
    glm::vec3 rayStart = worldPos + glm::vec3(0.0f, m_config.surfaceSnapTolerance, 0.0f);
    glm::vec3 rayEnd = worldPos - glm::vec3(0.0f, m_config.surfaceSnapTolerance, 0.0f);

    Lupine::Node3D* hitNode = nullptr;
    glm::vec3 hitPoint, hitNormal;

    // Perform raycast to find surface
    if (Lupine::PhysicsManager::Raycast3D(rayStart, rayEnd, hitNode, hitPoint, hitNormal)) {
        // Calculate the bottom offset of the object being placed
        float bottomOffset = calculateObjectBottomOffset();

        // Snap to surface with bottom of object touching the surface
        glm::vec3 snappedPos = hitPoint;
        snappedPos.y += bottomOffset;

        // Keep X and Z from original position, only adjust Y
        snappedPos.x = worldPos.x;
        snappedPos.z = worldPos.z;

        return snappedPos;
    }

    return worldPos; // No surface found
}

glm::vec3 PlacementMode::detectSurfaceWithTerrain(const glm::vec3& worldPos)
{
    // TODO: Implement terrain-specific surface detection
    // This would use terrain height maps and normal calculations
    return worldPos;
}

glm::vec3 PlacementMode::detectSurfaceWithMesh(const glm::vec3& worldPos)
{
    // TODO: Implement mesh-specific surface detection
    // This would use mesh vertex data for more precise surface detection
    return worldPos;
}

float PlacementMode::calculateObjectBottomOffset()
{
    // Calculate the bottom offset based on the ghost model or default size
    if (m_ghostModelLoaded && m_ghostModel) {
        // Calculate bounds of the ghost model
        glm::vec3 minBounds(FLT_MAX);
        glm::vec3 maxBounds(-FLT_MAX);

        const auto& meshes = m_ghostModel->GetMeshes();
        for (const auto& mesh : meshes) {
            for (const auto& vertex : mesh.vertices) {
                minBounds = glm::min(minBounds, vertex.position);
                maxBounds = glm::max(maxBounds, vertex.position);
            }
        }

        // Return the distance from center to bottom
        return -minBounds.y; // Negative because we want the offset from center to bottom
    }

    // Default fallback - assume object center is at its bottom
    return 0.0f;
}

void PlacementMode::loadGhostModel(const QString& filePath)
{
    clearGhostModel();
    
    QFileInfo fileInfo(filePath);
    QString extension = fileInfo.suffix().toLower();
    
    // Check if it's a 3D model
    QStringList modelExtensions = {"obj", "fbx", "dae", "gltf", "glb", "3ds", "blend", "ply"};
    if (modelExtensions.contains(extension)) {
        try {
            m_ghostModel = Lupine::MeshLoader::LoadModel(filePath.toStdString());
            m_ghostModelLoaded = (m_ghostModel && m_ghostModel->IsLoaded());
        } catch (const std::exception& e) {
            std::cerr << "PlacementMode: Failed to load ghost model: " << e.what() << std::endl;
            m_ghostModelLoaded = false;
        }
    }
    
    // Check if it's an image for sprite
    QStringList imageExtensions = {"png", "jpg", "jpeg", "bmp", "tga", "tiff", "gif", "webp"};
    if (imageExtensions.contains(extension)) {
        try {
            auto texture = Lupine::ResourceManager::LoadTexture(filePath.toStdString(), true);
            if (texture.IsValid()) {
                m_ghostTexture = texture.id;
                m_ghostTextureLoaded = true;
                m_ghostSpriteSize = glm::vec2(texture.width / 100.0f, texture.height / 100.0f); // Scale down
            }
        } catch (const std::exception& e) {
            std::cerr << "PlacementMode: Failed to load ghost texture: " << e.what() << std::endl;
            m_ghostTextureLoaded = false;
        }
    }
}

void PlacementMode::clearGhostModel()
{
    m_ghostModel.reset();
    m_ghostModelLoaded = false;
    m_ghostTexture = 0;
    m_ghostTextureLoaded = false;
}

void PlacementMode::updateGhostPosition(const glm::vec3& worldPos)
{
    m_ghostPosition = worldPos;
}

void PlacementMode::renderGhost(Lupine::Camera* camera)
{
    if (!m_showGhost || !camera) {
        return;
    }

    // Calculate transform with potential surface alignment
    glm::mat4 transform = calculateGhostTransform();

    // Choose ghost color based on placement mode and validity
    glm::vec4 ghostColor = calculateGhostColor();

    // Render 3D model ghost using engine's renderer
    if (m_ghostModelLoaded && m_ghostModel) {
        const auto& meshes = m_ghostModel->GetMeshes();
        for (const auto& mesh : meshes) {
            // Ensure mesh is set up for rendering
            if (mesh.VAO == 0) {
                const_cast<Lupine::Mesh&>(mesh).SetupMesh();
            }

            // Use engine's renderer which handles OpenGL state properly
            Lupine::Renderer::RenderMesh(mesh, transform, ghostColor);
        }
    }

    // Render sprite ghost
    if (m_ghostTextureLoaded && m_ghostTexture != 0) {
        // TODO: Implement sprite quad rendering for ghost preview using engine's renderer
        // This would use Lupine::Renderer for proper state management
    }

    // Render placement indicators (grid lines, surface normal, etc.)
    renderPlacementIndicators(camera);
}

glm::mat4 PlacementMode::calculateGhostTransform()
{
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, m_ghostPosition);

    // Apply surface alignment if enabled
    if (m_config.alignToSurfaceNormal && m_config.surfaceSnapEnabled) {
        // TODO: Calculate rotation based on surface normal
        // This would require storing the surface normal from the last raycast
    }

    return transform;
}

glm::vec4 PlacementMode::calculateGhostColor()
{
    glm::vec4 baseColor(1.0f, 1.0f, 1.0f, m_config.ghostOpacity);

    // Color-code based on placement mode
    switch (m_config.placementMode) {
        case PlacementModeType::FreePlace:
            baseColor = glm::vec4(1.0f, 1.0f, 1.0f, m_config.ghostOpacity); // White
            break;
        case PlacementModeType::GridSnap:
            baseColor = glm::vec4(0.0f, 1.0f, 1.0f, m_config.ghostOpacity); // Cyan
            break;
        case PlacementModeType::SurfaceSnap:
            baseColor = glm::vec4(0.0f, 1.0f, 0.0f, m_config.ghostOpacity); // Green
            break;
        case PlacementModeType::Combined:
            baseColor = glm::vec4(1.0f, 1.0f, 0.0f, m_config.ghostOpacity); // Yellow
            break;
    }

    return baseColor;
}

void PlacementMode::renderPlacementIndicators(Lupine::Camera* camera)
{
    // TODO: Render visual indicators for different placement modes
    // - Grid lines for grid snapping
    // - Surface normal arrows for surface snapping
    // - Snap points or guides
    // This would use Lupine::DebugRenderer for drawing lines and shapes
}
