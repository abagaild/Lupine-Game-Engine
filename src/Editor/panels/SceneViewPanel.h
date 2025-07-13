#pragma once

#ifndef SCENEVIEWPANEL_H
#define SCENEVIEWPANEL_H

// Include glad before any Qt OpenGL headers to avoid conflicts
#include <glad/glad.h>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "../rendering/GizmoRenderer.h"
#include "../AssetDropHandler.h"

namespace Lupine {
    class Scene;
    class Camera;
    class Engine;
    class Node;
    struct GridConfig;
}

class AssetDropHandler;
class PlacementMode;

class SceneViewPanel : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    enum class ViewMode {
        Mode2D,
        Mode3D
    };

    explicit SceneViewPanel(QWidget *parent = nullptr);
    ~SceneViewPanel();

    void setScene(Lupine::Scene* scene);
    void setEngine(Lupine::Engine* engine);

    // Scene access
    Lupine::Scene* getCurrentScene() const { return m_scene; }

    // View mode controls
    void setViewMode(ViewMode mode);
    ViewMode getViewMode() const { return m_viewMode; }

    // Camera controls
    void resetCamera();
    void focusOnScene();

    // Grid controls
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }
    void setGridConfig(const Lupine::GridConfig& config);
    const Lupine::GridConfig& getGridConfig() const;

    // Selection controls
    void setSelectedNode(Lupine::Node* node);
    void setSelectedNodeSilently(Lupine::Node* node); // Set selection without emitting signals
    Lupine::Node* getSelectedNode() const { return m_selectedNode; }

    // Placement mode
    void setPlacementMode(PlacementMode* placementMode);
    PlacementMode* getPlacementMode() const { return m_placementMode; }

    // Gizmo controls
    void setActiveGizmo(GizmoType gizmo);
    GizmoType getActiveGizmo() const { return m_activeGizmo; }

    // Force a complete scene refresh (useful after adding/removing nodes)
    void forceSceneRefresh();

    // Coordinate conversion (public for AssetDropHandler)
    glm::vec3 screenToWorldRay(const QPoint& screenPos, glm::vec3& rayOrigin, glm::vec3& rayDirection);
    glm::vec3 screenToWorldPosition(const QPoint& screenPos);
    glm::vec3 screenToWorldPositionWithSurfaceDetection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection);

signals:
    void nodeSelected(Lupine::Node* node);
    void nodeCreated(Lupine::Node* node);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    // Drag and drop events
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void updateScene();

private:
    Lupine::Scene* m_scene;
    Lupine::Engine* m_engine;
    QTimer* m_updateTimer;

    // Camera system
    std::unique_ptr<Lupine::Camera> m_editorCamera;
    ViewMode m_viewMode;

    // Mouse interaction
    QPoint m_lastMousePos;
    bool m_isRotating;
    bool m_isPanning;
    bool m_isZooming;

    // Camera movement parameters
    float m_cameraSpeed;
    float m_mouseSensitivity;
    float m_zoomSpeed;
    float m_panSpeed;

    // 2D camera parameters
    float m_zoom2D;
    glm::vec2 m_pan2D;

    // 3D camera parameters
    float m_cameraDistance;
    float m_cameraYaw;
    float m_cameraPitch;
    glm::vec3 m_cameraTarget;

    // Grid settings
    bool m_gridVisible;
    std::unique_ptr<Lupine::GridConfig> m_gridConfig;

    // Selection system
    Lupine::Node* m_selectedNode;
    bool m_isDraggingGizmo;

    // Gizmo system
    std::unique_ptr<GizmoRenderer> m_gizmoRenderer;
    GizmoType m_activeGizmo;
    GizmoAxis m_hoveredGizmoAxis;
    GizmoAxis m_draggedGizmoAxis;

    // Gizmo drag state
    glm::vec3 m_dragStartPosition;
    glm::quat m_dragStartRotation;
    glm::vec3 m_dragStartScale;
    glm::vec3 m_dragStartMouseWorld;
    float m_dragStartAngle;

    // Helper methods
    void handleMouseSelection(const QPoint& mousePos);
    Lupine::Node* performRaycast(const QPoint& mousePos);
    bool rayIntersectsNode(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, Lupine::Node* node, float& distance);
    glm::vec2 worldToScreen(const glm::vec3& worldPos);
    void renderSelectionHighlight(Lupine::Node* node);
    void renderBoundingBoxOutline(const glm::vec3& center, const glm::vec3& size, const glm::vec3& color);
    void renderOutlineLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color);
    void updateTextureFiltering();

    // Gizmo helper methods
    GizmoHitResult testGizmoHit(const QPoint& mousePos);
    void updateGizmoHighlight(const QPoint& mousePos);
    void startGizmoDrag(const QPoint& mousePos, GizmoAxis axis);
    void updateGizmoDrag(const QPoint& mousePos);
    void endGizmoDrag();
    void updateMoveGizmoDrag(const glm::vec3& rayOrigin, const glm::vec3& rayDirection);
    void updateRotateGizmoDrag(const glm::vec3& rayOrigin, const glm::vec3& rayDirection);
    void updateScaleGizmoDrag(const glm::vec3& rayOrigin, const glm::vec3& rayDirection);

    // Helper methods
    void updateCameraMatrices();
    void update2DCamera();
    void update3DCamera();
    void handleMouseRotation(const QPoint& delta);
    void handleMousePanning(const QPoint& delta);
    void handleMouseZoom(float delta);
    void renderGrid();
    bool isNodeValid(Lupine::Node* node) const;
    bool isNodeInHierarchy(Lupine::Node* root, Lupine::Node* target) const;

    // Drag and drop support
    AssetDropHandler* m_assetDropHandler;
    bool m_isDragging;
    PlacementMode* m_placementMode;

    // Cached drag validation data to prevent jittering
    QString m_currentDragFilePath;

    // Initialization state
    bool m_isFullyInitialized;

    // Signal recursion guard
    bool m_blockSelectionSignals;
};

#endif // SCENEVIEWPANEL_H
