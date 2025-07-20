#pragma once

#include <glad/glad.h>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QTimer>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

// Forward declarations
namespace Lupine {
    class Scene;
    class Node;
    class Camera;
    class GridConfig;
    enum class ProjectionType;
}

/**
 * @brief Specialized 2D scene view for menu editing
 * 
 * This widget provides a 2D OpenGL view optimized for UI/menu editing with:
 * - Game bounds visualization
 * - Grid rendering with snap-to-grid functionality
 * - 2D camera controls (pan, zoom)
 * - Element selection and manipulation
 * - Drag-and-drop support for UI components
 */
class MenuSceneView : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit MenuSceneView(QWidget* parent = nullptr);
    ~MenuSceneView();

    // Scene management
    void setScene(Lupine::Scene* scene);
    Lupine::Scene* getScene() const { return m_scene; }

    // Camera controls
    void resetCamera();
    void fitToView();
    void zoomIn();
    void zoomOut();
    void setZoom(float zoom);
    float getZoom() const { return m_zoom; }

    // Grid controls
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }
    void setGridSize(float size);
    float getGridSize() const { return m_gridSize; }
    void setSnapToGrid(bool snap);
    bool isSnapToGrid() const { return m_snapToGrid; }

    // Canvas settings
    void setCanvasSize(const QSizeF& size);
    QSizeF getCanvasSize() const { return m_canvasSize; }

    // Selection
    void setSelectedNode(Lupine::Node* node);
    Lupine::Node* getSelectedNode() const { return m_selectedNode; }

    // Coordinate conversion
    QPointF screenToWorld(const QPoint& screenPos) const;
    QPoint worldToScreen(const QPointF& worldPos) const;
    QPointF snapToGrid(const QPointF& position) const;

signals:
    void nodeSelected(Lupine::Node* node);
    void nodeDeselected();
    void nodeAdded(const QString& componentType, const QPointF& position);
    void nodeMoved(Lupine::Node* node, const QPointF& newPosition);
    void nodeResized(Lupine::Node* node, const QSizeF& newSize);

protected:
    // OpenGL overrides
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    // Event handling
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    // Drag and drop
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private slots:
    void onUpdateTimer();

private:
    // Initialization
    void initializeCamera();
    void initializeGrid();
    void setupUpdateTimer();

    // Rendering
    void renderBackground();
    void renderGrid();
    void renderCanvasBounds();
    void renderScene();
    void renderSelection();
    void renderGizmos();

    // Camera management
    void updateCameraMatrices();
    void updateViewMatrix();
    void updateProjectionMatrix();

    // Interaction
    Lupine::Node* performRaycast(const QPoint& mousePos);
    void handleSelection(const QPoint& mousePos);
    void handleDragging(const QPoint& mousePos);
    void startDragging(Lupine::Node* node, const QPoint& startPos);
    void stopDragging();

    // Grid utilities
    void renderGridLines();
    QPointF getGridSpacing() const;

    // Scene data
    Lupine::Scene* m_scene;
    std::unique_ptr<Lupine::Camera> m_camera;
    std::unique_ptr<Lupine::GridConfig> m_gridConfig;

    // Camera state
    glm::vec2 m_cameraPosition;
    float m_zoom;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;

    // Grid settings
    bool m_gridVisible;
    float m_gridSize;
    bool m_snapToGrid;
    glm::vec4 m_gridColor;
    glm::vec4 m_majorGridColor;

    // Canvas settings
    QSizeF m_canvasSize;
    glm::vec4 m_canvasBorderColor;
    glm::vec4 m_backgroundColorInside;
    glm::vec4 m_backgroundColorOutside;

    // Selection and interaction
    Lupine::Node* m_selectedNode;
    bool m_isDragging;
    QPoint m_lastMousePos;
    QPoint m_dragStartPos;
    QPointF m_dragStartNodePos;

    // Mouse state
    bool m_leftMousePressed;
    bool m_rightMousePressed;
    bool m_middleMousePressed;
    Qt::KeyboardModifiers m_keyModifiers;

    // Update timer
    QTimer* m_updateTimer;

    // Rendering state
    bool m_initialized;
    int m_viewportWidth;
    int m_viewportHeight;

    // Constants
    static constexpr float MIN_ZOOM = 0.1f;
    static constexpr float MAX_ZOOM = 10.0f;
    static constexpr float ZOOM_STEP = 0.1f;
    static constexpr float PAN_SPEED = 1.0f;
    static constexpr float GRID_FADE_DISTANCE = 50.0f;
};
