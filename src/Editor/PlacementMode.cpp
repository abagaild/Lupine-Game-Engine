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
#include "lupine/components/TerrainRenderer.h"
#include "lupine/components/TerrainCollider.h"
#include "lupine/components/StaticMesh.h"
#include "lupine/components/CollisionMesh3D.h"
#include "lupine/core/Scene.h"
#include "lupine/rendering/DebugRenderer.h"
#include "lupine/rendering/GridRenderer.h"
#include <QFileInfo>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
        // Store the surface normal for alignment calculations
        m_lastSurfaceNormal = glm::normalize(hitNormal);

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

    // Reset surface normal to default if no surface found
    m_lastSurfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    return worldPos; // No surface found
}

glm::vec3 PlacementMode::detectSurfaceWithTerrain(const glm::vec3& worldPos)
{
    if (!m_sceneView || !m_sceneView->getCurrentScene()) {
        return worldPos;
    }

    // Search for terrain components in the scene
    Lupine::Scene* scene = m_sceneView->getCurrentScene();
    if (!scene || !scene->GetRootNode()) {
        return worldPos;
    }

    // Recursively search for terrain components
    std::function<Lupine::TerrainRenderer*(Lupine::Node*)> findTerrain = [&](Lupine::Node* node) -> Lupine::TerrainRenderer* {
        if (!node) return nullptr;

        // Check if this node has a TerrainRenderer component
        auto terrainRenderer = node->GetComponent<Lupine::TerrainRenderer>();
        if (terrainRenderer) {
            return terrainRenderer;
        }

        // Check children
        for (const auto& child : node->GetChildren()) {
            auto result = findTerrain(child.get());
            if (result) return result;
        }

        return nullptr;
    };

    Lupine::TerrainRenderer* terrainRenderer = findTerrain(scene->GetRootNode());
    if (!terrainRenderer) {
        return worldPos; // No terrain found, return original position
    }

    // Get height at the world position from terrain
    float terrainHeight = terrainRenderer->GetHeightAtPosition(worldPos);
    if (std::isnan(terrainHeight)) {
        // Reset surface normal to default if outside terrain bounds
        m_lastSurfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        return worldPos; // Position is outside terrain bounds
    }

    // Get surface normal at the world position from terrain
    glm::vec3 terrainNormal = terrainRenderer->GetNormalAtPosition(worldPos);
    m_lastSurfaceNormal = glm::normalize(terrainNormal);

    // Calculate the bottom offset of the object being placed
    float bottomOffset = calculateObjectBottomOffset();

    // Create snapped position with terrain height
    glm::vec3 snappedPos = worldPos;
    snappedPos.y = terrainHeight + bottomOffset;

    return snappedPos;
}

glm::vec3 PlacementMode::detectSurfaceWithMesh(const glm::vec3& worldPos)
{
    if (!m_sceneView || !m_sceneView->getCurrentScene()) {
        return worldPos;
    }

    // Search for mesh components in the scene
    Lupine::Scene* scene = m_sceneView->getCurrentScene();
    if (!scene || !scene->GetRootNode()) {
        return worldPos;
    }

    // Recursively search for mesh components with collision
    std::function<glm::vec3(Lupine::Node*)> findMeshSurface = [&](Lupine::Node* node) -> glm::vec3 {
        if (!node) return worldPos;

        // Check if this node has both StaticMesh and CollisionMesh3D components
        auto staticMesh = node->GetComponent<Lupine::StaticMesh>();
        auto collisionMesh = node->GetComponent<Lupine::CollisionMesh3D>();

        if (staticMesh && collisionMesh && collisionMesh->IsMeshLoaded()) {
            // Get the node's transform
            auto* node3d = dynamic_cast<Lupine::Node3D*>(node);
            if (node3d) {
                glm::vec3 nodePos = node3d->GetGlobalPosition();
                glm::vec3 nodeScale = node3d->GetGlobalScale();

                // Simple bounding box check for mesh intersection
                // This is a simplified implementation - a full implementation would
                // use actual mesh vertex data and triangle intersection

                // Calculate approximate mesh bounds based on scale
                float meshRadius = std::max({nodeScale.x, nodeScale.y, nodeScale.z}) * 2.0f;
                float distance = glm::distance(worldPos, nodePos);

                if (distance <= meshRadius) {
                    // Found a mesh surface - snap to the top of the mesh
                    float bottomOffset = calculateObjectBottomOffset();
                    glm::vec3 snappedPos = worldPos;
                    snappedPos.y = nodePos.y + (nodeScale.y * 0.5f) + bottomOffset;

                    // Calculate approximate surface normal based on mesh orientation
                    // In a full implementation, this would use actual mesh vertex data
                    glm::quat nodeRotationQuat = node3d->GetGlobalRotation();
                    // For now, use a simple approximation - just use the up vector from the rotation
                    glm::vec3 nodeRotation = glm::vec3(0.0f, 0.0f, 0.0f); // Simplified for now
                    glm::mat4 rotationMatrix = glm::mat4(1.0f);
                    rotationMatrix = glm::rotate(rotationMatrix, nodeRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
                    rotationMatrix = glm::rotate(rotationMatrix, nodeRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
                    rotationMatrix = glm::rotate(rotationMatrix, nodeRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

                    // Transform the up vector by the mesh rotation to get surface normal
                    glm::vec4 localUp = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
                    glm::vec4 worldNormal = rotationMatrix * localUp;
                    m_lastSurfaceNormal = glm::normalize(glm::vec3(worldNormal));

                    return snappedPos;
                }
            }
        }

        // Check children
        for (const auto& child : node->GetChildren()) {
            glm::vec3 result = findMeshSurface(child.get());
            if (result != worldPos) {
                return result; // Found a surface in children
            }
        }

        return worldPos;
    };

    glm::vec3 result = findMeshSurface(scene->GetRootNode());

    // If no mesh surface found, try using physics raycast as fallback
    if (result == worldPos) {
        return detectSurfaceWithPhysics(worldPos);
    }

    return result;
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
                m_ghostTexture = texture.graphics_texture;
                m_ghostTextureLoaded = true;
                m_ghostSpriteSize = glm::vec2(texture.width / 100.0f, texture.height / 100.0f); // Scale down
                std::cout << "PlacementMode: Loaded ghost sprite texture: " << filePath.toStdString()
                         << " (" << texture.width << "x" << texture.height << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "PlacementMode: Failed to load ghost texture: " << e.what() << std::endl;
            m_ghostTextureLoaded = false;
            m_ghostTexture = nullptr;
        }
    }
}

void PlacementMode::clearGhostModel()
{
    m_ghostModel.reset();
    m_ghostModelLoaded = false;
    m_ghostTexture = nullptr;
    m_ghostTextureLoaded = false;
    m_lastSurfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
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
    if (m_ghostTextureLoaded && m_ghostTexture && m_ghostTexture->IsValid()) {
        // Calculate sprite color with transparency
        glm::vec4 spriteColor = ghostColor;
        spriteColor.a *= 0.8f; // Make sprites slightly more transparent

        // Calculate sprite size based on placement mode and original texture size
        glm::vec2 spriteSize = m_ghostSpriteSize;

        // Adjust size based on placement mode
        switch (m_config.placementMode) {
            case PlacementModeType::GridSnap:
                // Scale to fit grid size while maintaining aspect ratio
                {
                    float maxDimension = std::max(spriteSize.x, spriteSize.y);
                    float scaleFactor = m_config.gridSize / maxDimension;
                    spriteSize *= scaleFactor;
                }
                break;
            case PlacementModeType::SurfaceSnap:
                // Keep original size but ensure it's not too small
                if (std::max(spriteSize.x, spriteSize.y) < 1.0f) {
                    spriteSize *= 2.0f;
                }
                break;
            default:
                // Use original size or default minimum
                if (std::max(spriteSize.x, spriteSize.y) < 0.5f) {
                    spriteSize *= 4.0f;
                }
                break;
        }

        // Create transform for sprite quad
        glm::mat4 spriteTransform = glm::mat4(1.0f);
        spriteTransform = glm::translate(spriteTransform, m_ghostPosition);

        // Implement proper billboarding to face the camera
        if (camera) {
            glm::vec3 cameraPos = camera->GetPosition();
            glm::vec3 cameraForward = camera->GetForward();
            glm::vec3 cameraUp = camera->GetUp();
            glm::vec3 cameraRight = glm::normalize(glm::cross(cameraForward, cameraUp));

            // Create billboard matrix that always faces the camera
            glm::mat4 billboardMatrix = glm::mat4(1.0f);
            billboardMatrix[0] = glm::vec4(cameraRight, 0.0f);
            billboardMatrix[1] = glm::vec4(cameraUp, 0.0f);
            billboardMatrix[2] = glm::vec4(-cameraForward, 0.0f);

            spriteTransform = spriteTransform * billboardMatrix;
        }

        // Apply surface normal alignment if enabled
        if (m_config.alignToSurfaceNormal && m_config.surfaceSnapEnabled) {
            // Calculate rotation to align with surface normal
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 surfaceNormal = glm::normalize(m_lastSurfaceNormal);

            // Only apply rotation if surface normal is significantly different from up
            if (glm::dot(surfaceNormal, up) < 0.99f) {
                glm::vec3 rotationAxis = glm::cross(up, surfaceNormal);
                if (glm::length(rotationAxis) > 0.001f) {
                    rotationAxis = glm::normalize(rotationAxis);
                    float rotationAngle = acos(glm::clamp(glm::dot(up, surfaceNormal), -1.0f, 1.0f));
                    glm::mat4 surfaceRotation = glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis);
                    spriteTransform = spriteTransform * surfaceRotation;
                }
            }
        }

        // Scale the sprite to final size
        spriteTransform = glm::scale(spriteTransform, glm::vec3(spriteSize.x, spriteSize.y, 1.0f));

        // Render the sprite quad using the engine's renderer with the loaded texture
        Lupine::Renderer::RenderQuad(spriteTransform, spriteColor, m_ghostTexture);
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
        // Calculate rotation to align with surface normal
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 surfaceNormal = glm::normalize(m_lastSurfaceNormal);

        // Only apply rotation if surface normal is significantly different from up
        float dotProduct = glm::dot(surfaceNormal, up);
        if (dotProduct < 0.999f) { // Allow for small floating point errors
            // Calculate rotation axis and angle
            glm::vec3 rotationAxis = glm::cross(up, surfaceNormal);

            if (glm::length(rotationAxis) > 0.001f) {
                rotationAxis = glm::normalize(rotationAxis);
                float rotationAngle = acos(glm::clamp(dotProduct, -1.0f, 1.0f));

                // Create rotation matrix to align with surface normal
                glm::mat4 surfaceRotation = glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis);
                transform = transform * surfaceRotation;
            }
        }
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
    if (!camera) return;

    glm::mat4 viewProjection = camera->GetProjectionMatrix() * camera->GetViewMatrix();

    // Render grid lines for grid snapping mode
    if (m_config.placementMode == PlacementModeType::GridSnap && m_config.gridSnapEnabled) {
        renderGridIndicators(camera);
    }

    // Render surface normal arrows for surface snapping mode
    if (m_config.surfaceSnapEnabled && m_config.placementMode == PlacementModeType::SurfaceSnap) {
        renderSurfaceNormalIndicators(camera, viewProjection);
    }

    // Render snap guides and placement hints
    renderSnapGuides(camera, viewProjection);

    // Render placement bounds/area indicators
    renderPlacementBounds(camera, viewProjection);
}

void PlacementMode::renderGridIndicators(Lupine::Camera* camera)
{
    if (!camera) return;

    // Create custom grid configuration for placement mode
    Lupine::GridConfig gridConfig = Lupine::GridRenderer::GetDefaultConfig();

    // Customize grid appearance for placement mode
    gridConfig.majorLineColor = glm::vec4(0.4f, 0.8f, 1.0f, 0.8f);  // Bright blue major lines
    gridConfig.minorLineColor = glm::vec4(0.3f, 0.6f, 0.9f, 0.5f);  // Lighter blue minor lines
    gridConfig.axisLineColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);   // White axis lines

    // Set grid spacing based on placement configuration
    gridConfig.majorSpacing = m_config.gridSize * 5.0f;  // Major lines every 5 grid units
    gridConfig.minorSpacing = m_config.gridSize;         // Minor lines at grid size

    // Adjust grid size based on camera distance for better visibility
    float cameraDistance = glm::length(camera->GetPosition() - m_ghostPosition);
    gridConfig.gridSize = std::max(50.0f, cameraDistance * 2.0f);

    // Center grid around ghost position
    gridConfig.gridOffset = glm::vec2(m_ghostPosition.x, m_ghostPosition.z);

    // Enable all grid features for placement mode
    gridConfig.showMajorLines = true;
    gridConfig.showMinorLines = true;
    gridConfig.showAxisLines = true;
    gridConfig.fadeWithDistance = false;  // Keep grid visible at all distances

    // Render 3D grid for placement
    Lupine::GridRenderer::Render3DGrid(camera, gridConfig);

    // Highlight the current grid cell
    glm::vec3 gridCellCenter = m_ghostPosition;
    gridCellCenter.x = floor(gridCellCenter.x / m_config.gridSize) * m_config.gridSize + m_config.gridSize * 0.5f;
    gridCellCenter.z = floor(gridCellCenter.z / m_config.gridSize) * m_config.gridSize + m_config.gridSize * 0.5f;

    // Draw highlighted grid cell outline
    glm::mat4 viewProjection = camera->GetProjectionMatrix() * camera->GetViewMatrix();
    float halfGrid = m_config.gridSize * 0.5f;

    glm::vec3 cellMin = gridCellCenter - glm::vec3(halfGrid, 0.0f, halfGrid);
    glm::vec3 cellMax = gridCellCenter + glm::vec3(halfGrid, 0.0f, halfGrid);

    // Draw cell outline at ground level
    glm::vec3 highlightColor = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow highlight
    Lupine::DebugRenderer::DrawLine(glm::vec3(cellMin.x, gridCellCenter.y, cellMin.z),
                                   glm::vec3(cellMax.x, gridCellCenter.y, cellMin.z),
                                   highlightColor, viewProjection);
    Lupine::DebugRenderer::DrawLine(glm::vec3(cellMax.x, gridCellCenter.y, cellMin.z),
                                   glm::vec3(cellMax.x, gridCellCenter.y, cellMax.z),
                                   highlightColor, viewProjection);
    Lupine::DebugRenderer::DrawLine(glm::vec3(cellMax.x, gridCellCenter.y, cellMax.z),
                                   glm::vec3(cellMin.x, gridCellCenter.y, cellMax.z),
                                   highlightColor, viewProjection);
    Lupine::DebugRenderer::DrawLine(glm::vec3(cellMin.x, gridCellCenter.y, cellMax.z),
                                   glm::vec3(cellMin.x, gridCellCenter.y, cellMin.z),
                                   highlightColor, viewProjection);
}

void PlacementMode::renderSurfaceNormalIndicators(Lupine::Camera* camera, const glm::mat4& viewProjection)
{
    if (!camera) return;

    // Only render surface normal indicators if we have a valid surface normal
    if (glm::length(m_lastSurfaceNormal) < 0.1f) return;

    // Draw surface normal arrow at ghost position
    glm::vec3 normalColor = glm::vec3(0.0f, 1.0f, 0.0f); // Green for surface normal
    float arrowLength = 2.0f;

    // Draw the main surface normal arrow
    Lupine::DebugRenderer::DrawDirectionalArrow(m_ghostPosition, m_lastSurfaceNormal,
                                               arrowLength, normalColor, viewProjection);

    // If surface alignment is enabled, show alignment indicators
    if (m_config.alignToSurfaceNormal) {
        // Draw object's up vector for comparison
        glm::vec3 objectUp = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 upColor = glm::vec3(1.0f, 0.0f, 0.0f); // Red for object up

        Lupine::DebugRenderer::DrawDirectionalArrow(m_ghostPosition + glm::vec3(0.5f, 0.0f, 0.0f),
                                                   objectUp, arrowLength * 0.8f, upColor, viewProjection);

        // Draw alignment arc or indicator
        float alignmentDot = glm::dot(m_lastSurfaceNormal, objectUp);
        if (alignmentDot < 0.99f) {
            // Draw a small circle around the ghost position to indicate rotation
            int circleSegments = 16;
            float circleRadius = 1.0f;
            glm::vec3 circleColor = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow for alignment indicator

            for (int i = 0; i < circleSegments; ++i) {
                float angle1 = (float)i / circleSegments * 2.0f * M_PI;
                float angle2 = (float)(i + 1) / circleSegments * 2.0f * M_PI;

                glm::vec3 point1 = m_ghostPosition + glm::vec3(cos(angle1) * circleRadius, 0.1f, sin(angle1) * circleRadius);
                glm::vec3 point2 = m_ghostPosition + glm::vec3(cos(angle2) * circleRadius, 0.1f, sin(angle2) * circleRadius);

                Lupine::DebugRenderer::DrawLine(point1, point2, circleColor, viewProjection);
            }
        }
    }

    // Draw surface contact point indicator
    glm::vec3 contactColor = glm::vec3(1.0f, 0.5f, 0.0f); // Orange for contact point
    float contactRadius = 0.3f;

    // Draw a small cross at the contact point
    glm::vec3 contactPoint = m_ghostPosition - glm::vec3(0.0f, calculateObjectBottomOffset(), 0.0f);
    Lupine::DebugRenderer::DrawLine(contactPoint - glm::vec3(contactRadius, 0.0f, 0.0f),
                                   contactPoint + glm::vec3(contactRadius, 0.0f, 0.0f),
                                   contactColor, viewProjection);
    Lupine::DebugRenderer::DrawLine(contactPoint - glm::vec3(0.0f, 0.0f, contactRadius),
                                   contactPoint + glm::vec3(0.0f, 0.0f, contactRadius),
                                   contactColor, viewProjection);
}

void PlacementMode::renderSnapGuides(Lupine::Camera* camera, const glm::mat4& viewProjection)
{
    if (!camera) return;

    // Draw snap tolerance indicators
    glm::vec3 snapColor = glm::vec3(0.8f, 0.8f, 0.8f); // Light gray for snap guides

    // Draw vertical snap tolerance line
    if (m_config.surfaceSnapEnabled) {
        glm::vec3 snapStart = m_ghostPosition + glm::vec3(0.0f, m_config.surfaceSnapTolerance, 0.0f);
        glm::vec3 snapEnd = m_ghostPosition - glm::vec3(0.0f, m_config.surfaceSnapTolerance, 0.0f);
        Lupine::DebugRenderer::DrawLine(snapStart, snapEnd, snapColor, viewProjection);

        // Draw small horizontal lines at tolerance bounds
        float toleranceIndicatorSize = 0.2f;
        Lupine::DebugRenderer::DrawLine(snapStart - glm::vec3(toleranceIndicatorSize, 0.0f, 0.0f),
                                       snapStart + glm::vec3(toleranceIndicatorSize, 0.0f, 0.0f),
                                       snapColor, viewProjection);
        Lupine::DebugRenderer::DrawLine(snapEnd - glm::vec3(toleranceIndicatorSize, 0.0f, 0.0f),
                                       snapEnd + glm::vec3(toleranceIndicatorSize, 0.0f, 0.0f),
                                       snapColor, viewProjection);
    }

    // Draw placement crosshair at ghost position
    glm::vec3 crosshairColor = glm::vec3(1.0f, 1.0f, 1.0f); // White crosshair
    float crosshairSize = 0.5f;

    // Horizontal crosshair line
    Lupine::DebugRenderer::DrawLine(m_ghostPosition - glm::vec3(crosshairSize, 0.0f, 0.0f),
                                   m_ghostPosition + glm::vec3(crosshairSize, 0.0f, 0.0f),
                                   crosshairColor, viewProjection);
    // Vertical crosshair line (in Z direction)
    Lupine::DebugRenderer::DrawLine(m_ghostPosition - glm::vec3(0.0f, 0.0f, crosshairSize),
                                   m_ghostPosition + glm::vec3(0.0f, 0.0f, crosshairSize),
                                   crosshairColor, viewProjection);

    // Draw snap distance indicators for nearby objects
    if (m_sceneView && m_sceneView->getCurrentScene()) {
        renderNearbyObjectIndicators(camera, viewProjection);
    }
}

void PlacementMode::renderPlacementBounds(Lupine::Camera* camera, const glm::mat4& viewProjection)
{
    if (!camera) return;

    // Calculate object bounds for placement preview
    float objectSize = calculateObjectBottomOffset() * 2.0f; // Approximate object size
    if (objectSize < 0.5f) objectSize = 1.0f; // Minimum size for visibility

    glm::vec3 boundsColor = glm::vec3(0.5f, 0.5f, 1.0f); // Light blue for bounds

    // Draw bounding box around ghost position
    glm::vec3 boundsMin = m_ghostPosition - glm::vec3(objectSize * 0.5f, 0.0f, objectSize * 0.5f);
    glm::vec3 boundsMax = m_ghostPosition + glm::vec3(objectSize * 0.5f, objectSize, objectSize * 0.5f);

    // Draw wireframe bounding box
    Lupine::DebugRenderer::DrawWireframeBox(boundsMin, boundsMax, boundsColor, viewProjection);

    // Draw placement area indicator on ground
    if (m_config.placementMode == PlacementModeType::GridSnap) {
        // Show grid cell bounds
        float halfGrid = m_config.gridSize * 0.5f;
        glm::vec3 gridCenter = m_ghostPosition;
        gridCenter.x = floor(gridCenter.x / m_config.gridSize) * m_config.gridSize + halfGrid;
        gridCenter.z = floor(gridCenter.z / m_config.gridSize) * m_config.gridSize + halfGrid;

        glm::vec3 gridBoundsColor = glm::vec3(0.0f, 1.0f, 1.0f); // Cyan for grid bounds
        glm::vec3 gridMin = gridCenter - glm::vec3(halfGrid, 0.0f, halfGrid);
        glm::vec3 gridMax = gridCenter + glm::vec3(halfGrid, 0.0f, halfGrid);

        // Draw ground plane rectangle for grid cell
        Lupine::DebugRenderer::DrawLine(glm::vec3(gridMin.x, gridCenter.y, gridMin.z),
                                       glm::vec3(gridMax.x, gridCenter.y, gridMin.z),
                                       gridBoundsColor, viewProjection);
        Lupine::DebugRenderer::DrawLine(glm::vec3(gridMax.x, gridCenter.y, gridMin.z),
                                       glm::vec3(gridMax.x, gridCenter.y, gridMax.z),
                                       gridBoundsColor, viewProjection);
        Lupine::DebugRenderer::DrawLine(glm::vec3(gridMax.x, gridCenter.y, gridMax.z),
                                       glm::vec3(gridMin.x, gridCenter.y, gridMax.z),
                                       gridBoundsColor, viewProjection);
        Lupine::DebugRenderer::DrawLine(glm::vec3(gridMin.x, gridCenter.y, gridMax.z),
                                       glm::vec3(gridMin.x, gridCenter.y, gridMin.z),
                                       gridBoundsColor, viewProjection);
    }
}

void PlacementMode::renderNearbyObjectIndicators(Lupine::Camera* camera, const glm::mat4& viewProjection)
{
    if (!camera || !m_sceneView || !m_sceneView->getCurrentScene()) return;

    Lupine::Scene* scene = m_sceneView->getCurrentScene();
    if (!scene || !scene->GetRootNode()) return;

    float proximityDistance = 5.0f; // Distance to check for nearby objects
    glm::vec3 proximityColor = glm::vec3(1.0f, 0.8f, 0.0f); // Orange for proximity indicators

    // Recursively find nearby objects
    std::function<void(Lupine::Node*)> checkNearbyObjects = [&](Lupine::Node* node) {
        if (!node || !node->IsActive()) return;

        // Check if this node has a 3D transform
        auto* node3d = dynamic_cast<Lupine::Node3D*>(node);
        if (node3d) {
            glm::vec3 nodePos = node3d->GetGlobalPosition();
            float distance = glm::distance(nodePos, m_ghostPosition);

            // If object is nearby, draw proximity indicator
            if (distance > 0.1f && distance < proximityDistance) {
                // Draw line from ghost to nearby object
                Lupine::DebugRenderer::DrawLine(m_ghostPosition, nodePos, proximityColor, viewProjection);

                // Draw distance text indicator (simplified as a small marker)
                glm::vec3 midPoint = (m_ghostPosition + nodePos) * 0.5f;
                float markerSize = 0.1f;

                // Draw small cross at midpoint to indicate distance
                Lupine::DebugRenderer::DrawLine(midPoint - glm::vec3(markerSize, 0.0f, 0.0f),
                                               midPoint + glm::vec3(markerSize, 0.0f, 0.0f),
                                               proximityColor, viewProjection);
                Lupine::DebugRenderer::DrawLine(midPoint - glm::vec3(0.0f, markerSize, 0.0f),
                                               midPoint + glm::vec3(0.0f, markerSize, 0.0f),
                                               proximityColor, viewProjection);

                // Draw object bounds indicator
                if (node->GetComponent<Lupine::StaticMesh>() || node->GetComponent<Lupine::CollisionMesh3D>()) {
                    glm::vec3 objectScale = node3d->GetGlobalScale();
                    float objectRadius = std::max({objectScale.x, objectScale.y, objectScale.z}) * 0.5f;

                    // Draw simple circle around object (approximated with lines)
                    int segments = 8;
                    for (int i = 0; i < segments; ++i) {
                        float angle1 = (float)i / segments * 2.0f * M_PI;
                        float angle2 = (float)(i + 1) / segments * 2.0f * M_PI;

                        glm::vec3 point1 = nodePos + glm::vec3(cos(angle1) * objectRadius, 0.0f, sin(angle1) * objectRadius);
                        glm::vec3 point2 = nodePos + glm::vec3(cos(angle2) * objectRadius, 0.0f, sin(angle2) * objectRadius);

                        Lupine::DebugRenderer::DrawLine(point1, point2, proximityColor, viewProjection);
                    }
                }
            }
        }

        // Check children
        for (const auto& child : node->GetChildren()) {
            checkNearbyObjects(child.get());
        }
    };

    checkNearbyObjects(scene->GetRootNode());
}
