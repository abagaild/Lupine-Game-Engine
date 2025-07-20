#pragma once

// Include glad before any OpenGL usage
#include <glad/glad.h>

#include <QObject>
#include <QString>
#include <QPoint>
#include <glm/glm.hpp>
#include <memory>

namespace Lupine {
    class Node;
    class Scene;
    class Camera;
    class Model;
}

class SceneViewPanel;

/**
 * @brief Placement mode types
 */
enum class PlacementModeType {
    FreePlace,      // Free placement without snapping
    GridSnap,       // Snap to grid
    SurfaceSnap,    // Snap to surfaces
    Combined        // Grid + Surface snapping combined
};

/**
 * @brief Surface detection types
 */
enum class SurfaceDetectionType {
    Physics,        // Use physics raycasting (default)
    Terrain,        // Terrain-specific detection
    Mesh,           // Mesh surface detection
    All             // All surface types
};

/**
 * @brief Placement mode configuration and state
 */
struct PlacementConfig {
    bool enabled = false;
    PlacementModeType placementMode = PlacementModeType::FreePlace;
    SurfaceDetectionType surfaceDetection = SurfaceDetectionType::Physics;

    // Grid snapping settings
    bool gridSnapEnabled = false;
    bool gridSnapYEnabled = false;
    float gridSize = 1.0f;
    float placementGridY = 0.0f;

    // Surface snapping settings
    bool surfaceSnapEnabled = false;
    float surfaceSnapTolerance = 100.0f; // Maximum distance to search for surfaces
    bool alignToSurfaceNormal = false;   // Align object to surface normal
    Lupine::Node* snapToSurfaceNode = nullptr;

    // Visual settings
    float ghostOpacity = 0.3f;

    // Default placement types
    QString default2DSpriteType = "Sprite2D Component on 2D Node";
    QString default3DSpriteType = "Sprite3D Component on 3D Node";
    QString default3DModelType = "Static Mesh Component on 3D Node";
};

/**
 * @brief Manages placement mode for drag and drop operations
 */
class PlacementMode : public QObject
{
    Q_OBJECT

public:
    explicit PlacementMode(QObject* parent = nullptr);
    ~PlacementMode();

    // Configuration
    void setConfig(const PlacementConfig& config);
    const PlacementConfig& getConfig() const { return m_config; }
    
    // Placement mode state
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_config.enabled; }

    // Placement mode type
    void setPlacementModeType(PlacementModeType mode);
    PlacementModeType getPlacementModeType() const { return m_config.placementMode; }

    void setSurfaceDetectionType(SurfaceDetectionType type);
    SurfaceDetectionType getSurfaceDetectionType() const { return m_config.surfaceDetection; }

    // Grid snapping
    void setGridSnapEnabled(bool enabled);
    bool isGridSnapEnabled() const { return m_config.gridSnapEnabled; }

    void setGridSnapYEnabled(bool enabled);
    bool isGridSnapYEnabled() const { return m_config.gridSnapYEnabled; }

    void setGridSize(float size);
    float getGridSize() const { return m_config.gridSize; }

    void setPlacementGridY(float y);
    float getPlacementGridY() const { return m_config.placementGridY; }

    // Surface snapping
    void setSurfaceSnapEnabled(bool enabled);
    bool isSurfaceSnapEnabled() const { return m_config.surfaceSnapEnabled; }

    void setSurfaceSnapTolerance(float tolerance);
    float getSurfaceSnapTolerance() const { return m_config.surfaceSnapTolerance; }

    void setAlignToSurfaceNormal(bool align);
    bool getAlignToSurfaceNormal() const { return m_config.alignToSurfaceNormal; }

    void setSnapToSurfaceNode(Lupine::Node* node);
    Lupine::Node* getSnapToSurfaceNode() const { return m_config.snapToSurfaceNode; }
    
    // Ghost rendering
    void setGhostOpacity(float opacity);
    float getGhostOpacity() const { return m_config.ghostOpacity; }
    
    // Default placement types
    void setDefault2DSpriteType(const QString& type);
    const QString& getDefault2DSpriteType() const { return m_config.default2DSpriteType; }
    
    void setDefault3DSpriteType(const QString& type);
    const QString& getDefault3DSpriteType() const { return m_config.default3DSpriteType; }
    
    void setDefault3DModelType(const QString& type);
    const QString& getDefault3DModelType() const { return m_config.default3DModelType; }
    
    // Placement operations
    void startPlacement(const QString& filePath, const QPoint& screenPos, SceneViewPanel* sceneView);
    void updatePlacement(const QPoint& screenPos, bool ctrlPressed);
    void endPlacement();
    void cancelPlacement();
    
    // Position snapping
    glm::vec3 snapPosition(const glm::vec3& worldPos, bool is2D = false);
    
    // Ghost rendering
    void renderGhost(Lupine::Camera* camera);
    
    // State queries
    bool isPlacing() const { return m_isPlacing; }
    bool isGhostVisible() const { return m_isPlacing && m_showGhost; }

signals:
    void configChanged();
    void placementStarted(const QString& filePath);
    void placementEnded();
    void placementCancelled();

private:
    void loadGhostModel(const QString& filePath);
    void clearGhostModel();
    void updateGhostPosition(const glm::vec3& worldPos);
    glm::vec3 snapToGrid(const glm::vec3& worldPos, bool is2D = false);
    glm::vec3 snapToSurface(const glm::vec3& worldPos);
    float calculateObjectBottomOffset();

    // Enhanced surface detection methods
    glm::vec3 detectSurfaceWithPhysics(const glm::vec3& worldPos);
    glm::vec3 detectSurfaceWithTerrain(const glm::vec3& worldPos);
    glm::vec3 detectSurfaceWithMesh(const glm::vec3& worldPos);
    glm::vec3 applyPlacementMode(const glm::vec3& worldPos, bool is2D);

    // Enhanced ghost rendering methods
    glm::mat4 calculateGhostTransform();
    glm::vec4 calculateGhostColor();
    void renderPlacementIndicators(Lupine::Camera* camera);
    
    PlacementConfig m_config;
    
    // Placement state
    bool m_isPlacing = false;
    bool m_showGhost = false;
    QString m_currentFilePath;
    glm::vec3 m_ghostPosition{0.0f};
    SceneViewPanel* m_sceneView = nullptr;
    
    // Ghost rendering
    std::unique_ptr<Lupine::Model> m_ghostModel;
    bool m_ghostModelLoaded = false;
    unsigned int m_ghostTexture = 0;
    bool m_ghostTextureLoaded = false;
    glm::vec2 m_ghostSpriteSize{1.0f};
};
