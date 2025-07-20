#include "MenuSceneView.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include "lupine/nodes/Control.h"
#include "lupine/rendering/Camera.h"
#include "lupine/rendering/GridRenderer.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/DebugRenderer.h"
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <iostream>

MenuSceneView::MenuSceneView(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_scene(nullptr)
    , m_camera(nullptr)
    , m_gridConfig(nullptr)
    , m_cameraPosition(0.0f, 0.0f)
    , m_zoom(1.0f)
    , m_viewMatrix(1.0f)
    , m_projectionMatrix(1.0f)
    , m_gridVisible(true)
    , m_gridSize(20.0f)
    , m_snapToGrid(true)
    , m_gridColor(0.3f, 0.3f, 0.3f, 0.5f)
    , m_majorGridColor(0.4f, 0.4f, 0.4f, 0.7f)
    , m_canvasSize(1920.0f, 1080.0f)
    , m_canvasBorderColor(0.8f, 0.4f, 0.8f, 1.0f) // Purple border
    , m_backgroundColorInside(0.15f, 0.15f, 0.15f, 1.0f)
    , m_backgroundColorOutside(0.1f, 0.1f, 0.1f, 1.0f)
    , m_selectedNode(nullptr)
    , m_isDragging(false)
    , m_lastMousePos(0, 0)
    , m_dragStartPos(0, 0)
    , m_dragStartNodePos(0.0f, 0.0f)
    , m_leftMousePressed(false)
    , m_rightMousePressed(false)
    , m_middleMousePressed(false)
    , m_keyModifiers(Qt::NoModifier)
    , m_updateTimer(nullptr)
    , m_initialized(false)
    , m_viewportWidth(800)
    , m_viewportHeight(600)
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    setupUpdateTimer();
}

MenuSceneView::~MenuSceneView()
{
    if (m_updateTimer) {
        m_updateTimer->stop();
        delete m_updateTimer;
    }
}

void MenuSceneView::setupUpdateTimer()
{
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MenuSceneView::onUpdateTimer);
    m_updateTimer->start(16); // ~60 FPS
}

void MenuSceneView::initializeGL()
{
    initializeOpenGLFunctions();
    
    // Initialize OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    // Initialize camera
    initializeCamera();
    
    // Initialize grid
    initializeGrid();
    
    // Initialize renderers
    if (!Lupine::Renderer::IsInitialized()) {
        Lupine::Renderer::Initialize();
    }
    
    if (!Lupine::GridRenderer::IsInitialized()) {
        Lupine::GridRenderer::Initialize();
    }
    
    if (!Lupine::DebugRenderer::IsInitialized()) {
        Lupine::DebugRenderer::Initialize();
    }
    
    m_initialized = true;
    
    std::cout << "MenuSceneView: OpenGL initialized" << std::endl;
}

void MenuSceneView::initializeCamera()
{
    m_camera = std::make_unique<Lupine::Camera>(Lupine::ProjectionType::Orthographic);
    m_camera->SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));
    m_camera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));

    // Set initial orthographic bounds
    float orthoSize = 500.0f;
    m_camera->SetOrthographic(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 1000.0f);

    updateCameraMatrices();

    std::cout << "MenuSceneView: Camera initialized with orthographic projection" << std::endl;
}

void MenuSceneView::initializeGrid()
{
    m_gridConfig = std::make_unique<Lupine::GridConfig>();
    m_gridConfig->showMinorLines = true;
    m_gridConfig->showMajorLines = true;
    m_gridConfig->showAxisLines = true;
    m_gridConfig->minorSpacing = m_gridSize;
    m_gridConfig->majorSpacing = m_gridSize * 5.0f;
    m_gridConfig->minorLineColor = m_gridColor;
    m_gridConfig->majorLineColor = m_majorGridColor;
    m_gridConfig->axisLineColor = glm::vec4(0.6f, 0.6f, 0.6f, 0.8f);
    m_gridConfig->gridSize = 100.0f; // Large grid area
}

void MenuSceneView::resizeGL(int w, int h)
{
    m_viewportWidth = w;
    m_viewportHeight = h;
    glViewport(0, 0, w, h);
    updateProjectionMatrix();
}

void MenuSceneView::paintGL()
{
    if (!m_initialized) return;
    
    // Clear the screen
    glClearColor(m_backgroundColorOutside.r, m_backgroundColorOutside.g, 
                 m_backgroundColorOutside.b, m_backgroundColorOutside.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (!m_camera) return;
    
    // Update camera matrices
    updateCameraMatrices();
    
    // Render background
    renderBackground();
    
    // Render grid if visible
    if (m_gridVisible) {
        renderGrid();
    }
    
    // Render canvas bounds
    renderCanvasBounds();
    
    // Render scene
    if (m_scene) {
        renderScene();
    }
    
    // Render selection
    if (m_selectedNode) {
        renderSelection();
    }
    
    // Render gizmos
    renderGizmos();
}

void MenuSceneView::renderBackground()
{
    // Render canvas background
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(0.0f, 0.0f, -1.0f));
    transform = glm::scale(transform, glm::vec3(m_canvasSize.width(), m_canvasSize.height(), 1.0f));
    
    Lupine::Renderer::RenderQuad(transform, m_backgroundColorInside);
}

void MenuSceneView::renderGrid()
{
    if (!m_gridConfig || !Lupine::GridRenderer::IsInitialized()) return;
    
    // Update grid config based on current zoom
    Lupine::GridConfig dynamicConfig = *m_gridConfig;
    dynamicConfig.minorSpacing = m_gridSize;
    dynamicConfig.majorSpacing = m_gridSize * 5.0f;
    
    // Adjust grid opacity based on zoom
    float opacity = glm::clamp(m_zoom * 0.5f, 0.1f, 1.0f);
    dynamicConfig.minorLineColor.a = m_gridColor.a * opacity;
    dynamicConfig.majorLineColor.a = m_majorGridColor.a * opacity;
    
    Lupine::GridRenderer::Render2DGrid(m_camera.get(), dynamicConfig);
}

void MenuSceneView::renderCanvasBounds()
{
    // Render canvas border
    float halfWidth = m_canvasSize.width() * 0.5f;
    float halfHeight = m_canvasSize.height() * 0.5f;
    
    // Top border
    glm::mat4 topTransform = glm::mat4(1.0f);
    topTransform = glm::translate(topTransform, glm::vec3(0.0f, halfHeight, 0.0f));
    topTransform = glm::scale(topTransform, glm::vec3(m_canvasSize.width() + 4.0f, 2.0f, 1.0f));
    Lupine::Renderer::RenderQuad(topTransform, m_canvasBorderColor);
    
    // Bottom border
    glm::mat4 bottomTransform = glm::mat4(1.0f);
    bottomTransform = glm::translate(bottomTransform, glm::vec3(0.0f, -halfHeight, 0.0f));
    bottomTransform = glm::scale(bottomTransform, glm::vec3(m_canvasSize.width() + 4.0f, 2.0f, 1.0f));
    Lupine::Renderer::RenderQuad(bottomTransform, m_canvasBorderColor);
    
    // Left border
    glm::mat4 leftTransform = glm::mat4(1.0f);
    leftTransform = glm::translate(leftTransform, glm::vec3(-halfWidth, 0.0f, 0.0f));
    leftTransform = glm::scale(leftTransform, glm::vec3(2.0f, m_canvasSize.height(), 1.0f));
    Lupine::Renderer::RenderQuad(leftTransform, m_canvasBorderColor);
    
    // Right border
    glm::mat4 rightTransform = glm::mat4(1.0f);
    rightTransform = glm::translate(rightTransform, glm::vec3(halfWidth, 0.0f, 0.0f));
    rightTransform = glm::scale(rightTransform, glm::vec3(2.0f, m_canvasSize.height(), 1.0f));
    Lupine::Renderer::RenderQuad(rightTransform, m_canvasBorderColor);
}

void MenuSceneView::renderScene()
{
    if (!m_scene) return;
    
    // Set 2D rendering context
    Lupine::Renderer::SetRenderingContext(Lupine::RenderingContext::Editor2D);
    
    // Render the scene
    Lupine::Renderer::RenderScene(m_scene, m_camera.get(), false);
}

void MenuSceneView::renderSelection()
{
    if (!m_selectedNode) return;
    
    // Render selection outline
    if (auto* control = dynamic_cast<Lupine::Control*>(m_selectedNode)) {
        glm::vec2 pos = control->GetPosition();
        glm::vec2 size = control->GetSize();
        
        // Selection outline
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(pos.x, pos.y, 0.1f));
        transform = glm::scale(transform, glm::vec3(size.x + 4.0f, size.y + 4.0f, 1.0f));
        
        glm::vec4 selectionColor(1.0f, 0.5f, 0.0f, 0.8f); // Orange selection
        Lupine::Renderer::RenderQuad(transform, selectionColor);
    }
}

void MenuSceneView::renderGizmos()
{
    // TODO: Implement gizmo rendering for move/resize handles
}

void MenuSceneView::updateCameraMatrices()
{
    updateViewMatrix();
    updateProjectionMatrix();

    if (m_camera) {
        // Ensure camera matrices are updated
        m_camera->UpdateMatrices();

        // Debug output to verify orthographic projection
        if (m_camera->GetProjectionType() != Lupine::ProjectionType::Orthographic) {
            qWarning() << "MenuSceneView: Camera is not using orthographic projection!";
        }
    }
}

void MenuSceneView::updateViewMatrix()
{
    glm::vec3 cameraPos = glm::vec3(m_cameraPosition.x, m_cameraPosition.y, 10.0f);
    glm::vec3 target = glm::vec3(m_cameraPosition.x, m_cameraPosition.y, 0.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    
    m_viewMatrix = glm::lookAt(cameraPos, target, up);
}

void MenuSceneView::updateProjectionMatrix()
{
    if (m_viewportWidth <= 0 || m_viewportHeight <= 0) return;

    float aspect = static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight);
    float orthoSize = 500.0f / m_zoom; // Base orthographic size

    float left = -orthoSize * aspect;
    float right = orthoSize * aspect;
    float bottom = -orthoSize;
    float top = orthoSize;

    m_projectionMatrix = glm::ortho(left, right, bottom, top, 0.1f, 1000.0f);

    // Update camera with orthographic projection
    if (m_camera) {
        m_camera->SetOrthographic(left, right, bottom, top, 0.1f, 1000.0f);
        m_camera->SetPosition(glm::vec3(m_cameraPosition.x, m_cameraPosition.y, 10.0f));
        m_camera->SetTarget(glm::vec3(m_cameraPosition.x, m_cameraPosition.y, 0.0f));
    }
}

void MenuSceneView::setScene(Lupine::Scene* scene)
{
    m_scene = scene;
    update();
}

void MenuSceneView::resetCamera()
{
    m_cameraPosition = glm::vec2(0.0f, 0.0f);
    m_zoom = 1.0f;
    updateCameraMatrices();
    update();
}

void MenuSceneView::fitToView()
{
    // Fit canvas to view
    if (m_viewportWidth <= 0 || m_viewportHeight <= 0) return;
    
    float aspect = static_cast<float>(m_viewportWidth) / static_cast<float>(m_viewportHeight);
    float canvasAspect = m_canvasSize.width() / m_canvasSize.height();
    
    if (canvasAspect > aspect) {
        // Canvas is wider, fit to width
        m_zoom = (m_viewportWidth * 0.8f) / m_canvasSize.width();
    } else {
        // Canvas is taller, fit to height
        m_zoom = (m_viewportHeight * 0.8f) / m_canvasSize.height();
    }
    
    m_zoom = glm::clamp(m_zoom, MIN_ZOOM, MAX_ZOOM);
    m_cameraPosition = glm::vec2(0.0f, 0.0f);
    updateCameraMatrices();
    update();
}

void MenuSceneView::zoomIn()
{
    setZoom(m_zoom + ZOOM_STEP);
}

void MenuSceneView::zoomOut()
{
    setZoom(m_zoom - ZOOM_STEP);
}

void MenuSceneView::setZoom(float zoom)
{
    m_zoom = glm::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
    updateCameraMatrices();
    update();
}

void MenuSceneView::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    update();
}

void MenuSceneView::setGridSize(float size)
{
    m_gridSize = size;
    if (m_gridConfig) {
        m_gridConfig->minorSpacing = size;
        m_gridConfig->majorSpacing = size * 5.0f;
    }
    update();
}

void MenuSceneView::setSnapToGrid(bool snap)
{
    m_snapToGrid = snap;
}

void MenuSceneView::setCanvasSize(const QSizeF& size)
{
    m_canvasSize = size;
    update();
}

void MenuSceneView::setSelectedNode(Lupine::Node* node)
{
    m_selectedNode = node;
    update();
}

void MenuSceneView::onUpdateTimer()
{
    // Update the view if needed
    if (m_initialized) {
        update();
    }
}

// Mouse and event handling
void MenuSceneView::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();
    m_keyModifiers = event->modifiers();

    if (event->button() == Qt::LeftButton) {
        m_leftMousePressed = true;
        handleSelection(event->pos());
    } else if (event->button() == Qt::RightButton) {
        m_rightMousePressed = true;
    } else if (event->button() == Qt::MiddleButton) {
        m_middleMousePressed = true;
    }

    setFocus();
    update();
}

void MenuSceneView::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - m_lastMousePos;
    m_lastMousePos = event->pos();

    if (m_middleMousePressed || (m_rightMousePressed && (m_keyModifiers & Qt::AltModifier))) {
        // Pan camera
        float panScale = 1.0f / m_zoom;
        m_cameraPosition.x -= delta.x() * panScale * PAN_SPEED;
        m_cameraPosition.y += delta.y() * panScale * PAN_SPEED; // Invert Y
        updateCameraMatrices();
        update();
    } else if (m_leftMousePressed && m_isDragging && m_selectedNode) {
        // Drag selected node
        handleDragging(event->pos());
    }
}

void MenuSceneView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_leftMousePressed = false;
        if (m_isDragging) {
            stopDragging();
        }
    } else if (event->button() == Qt::RightButton) {
        m_rightMousePressed = false;
    } else if (event->button() == Qt::MiddleButton) {
        m_middleMousePressed = false;
    }

    update();
}

void MenuSceneView::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().y() / 120.0f; // Standard wheel step
    float zoomFactor = 1.0f + (delta * 0.1f);

    // Zoom towards mouse position
    QPointF mouseWorld = screenToWorld(event->position().toPoint());

    setZoom(m_zoom * zoomFactor);

    // Adjust camera position to zoom towards mouse
    QPointF newMouseWorld = screenToWorld(event->position().toPoint());
    QPointF offset = mouseWorld - newMouseWorld;
    m_cameraPosition.x += offset.x();
    m_cameraPosition.y += offset.y();

    updateCameraMatrices();
    update();
}

void MenuSceneView::keyPressEvent(QKeyEvent* event)
{
    m_keyModifiers = event->modifiers();

    switch (event->key()) {
        case Qt::Key_Delete:
            if (m_selectedNode) {
                // TODO: Delete selected node
                emit nodeSelected(nullptr);
            }
            break;
        case Qt::Key_Escape:
            if (m_selectedNode) {
                setSelectedNode(nullptr);
                emit nodeDeselected();
            }
            break;
        default:
            QOpenGLWidget::keyPressEvent(event);
            break;
    }
}

void MenuSceneView::keyReleaseEvent(QKeyEvent* event)
{
    m_keyModifiers = event->modifiers();
    QOpenGLWidget::keyReleaseEvent(event);
}

// Drag and drop handling
void MenuSceneView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void MenuSceneView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
    }
}

void MenuSceneView::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasText()) {
        QString componentType = event->mimeData()->text();
        QPointF worldPos = screenToWorld(event->position().toPoint());

        if (m_snapToGrid) {
            worldPos = snapToGrid(worldPos);
        }

        emit nodeAdded(componentType, worldPos);
        event->acceptProposedAction();
    }
}

// Coordinate conversion
QPointF MenuSceneView::screenToWorld(const QPoint& screenPos) const
{
    if (m_viewportWidth <= 0 || m_viewportHeight <= 0) return QPointF(0, 0);

    // Convert screen coordinates to normalized device coordinates
    float x = (2.0f * screenPos.x()) / m_viewportWidth - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / m_viewportHeight;

    // Convert to world coordinates
    glm::mat4 invProjection = glm::inverse(m_projectionMatrix);
    glm::mat4 invView = glm::inverse(m_viewMatrix);

    glm::vec4 worldPos = invView * invProjection * glm::vec4(x, y, 0.0f, 1.0f);

    return QPointF(worldPos.x, worldPos.y);
}

QPoint MenuSceneView::worldToScreen(const QPointF& worldPos) const
{
    if (m_viewportWidth <= 0 || m_viewportHeight <= 0) return QPoint(0, 0);

    glm::vec4 world = glm::vec4(worldPos.x(), worldPos.y(), 0.0f, 1.0f);
    glm::vec4 clip = m_projectionMatrix * m_viewMatrix * world;

    if (clip.w != 0.0f) {
        clip /= clip.w;
    }

    int x = static_cast<int>((clip.x + 1.0f) * 0.5f * m_viewportWidth);
    int y = static_cast<int>((1.0f - clip.y) * 0.5f * m_viewportHeight);

    return QPoint(x, y);
}

QPointF MenuSceneView::snapToGrid(const QPointF& position) const
{
    if (!m_snapToGrid || m_gridSize <= 0.0f) return position;

    float snappedX = std::round(position.x() / m_gridSize) * m_gridSize;
    float snappedY = std::round(position.y() / m_gridSize) * m_gridSize;

    return QPointF(snappedX, snappedY);
}

// Selection and interaction
void MenuSceneView::handleSelection(const QPoint& mousePos)
{
    Lupine::Node* clickedNode = performRaycast(mousePos);

    if (clickedNode != m_selectedNode) {
        setSelectedNode(clickedNode);
        if (clickedNode) {
            emit nodeSelected(clickedNode);
        } else {
            emit nodeDeselected();
        }
    }

    if (clickedNode) {
        startDragging(clickedNode, mousePos);
    }
}

Lupine::Node* MenuSceneView::performRaycast(const QPoint& mousePos)
{
    if (!m_scene) return nullptr;

    QPointF worldPos = screenToWorld(mousePos);

    // Simple 2D hit testing for Control nodes
    // TODO: Implement proper scene traversal and hit testing
    auto* root = m_scene->GetRootNode();
    if (!root) return nullptr;

    // For now, just check if we're clicking within canvas bounds
    float halfWidth = m_canvasSize.width() * 0.5f;
    float halfHeight = m_canvasSize.height() * 0.5f;

    if (worldPos.x() >= -halfWidth && worldPos.x() <= halfWidth &&
        worldPos.y() >= -halfHeight && worldPos.y() <= halfHeight) {
        return root; // Return root for now
    }

    return nullptr;
}

void MenuSceneView::handleDragging(const QPoint& mousePos)
{
    if (!m_isDragging || !m_selectedNode) return;

    QPointF worldPos = screenToWorld(mousePos);
    QPointF startWorldPos = screenToWorld(m_dragStartPos);
    QPointF delta = worldPos - startWorldPos;

    QPointF newPos = m_dragStartNodePos + delta;

    if (m_snapToGrid) {
        newPos = snapToGrid(newPos);
    }

    // Update node position
    if (auto* control = dynamic_cast<Lupine::Control*>(m_selectedNode)) {
        control->SetPosition(glm::vec2(newPos.x(), newPos.y()));
        emit nodeMoved(m_selectedNode, newPos);
    }

    update();
}

void MenuSceneView::startDragging(Lupine::Node* node, const QPoint& startPos)
{
    if (!node) return;

    m_isDragging = true;
    m_dragStartPos = startPos;

    if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        glm::vec2 pos = control->GetPosition();
        m_dragStartNodePos = QPointF(pos.x, pos.y);
    } else {
        m_dragStartNodePos = QPointF(0, 0);
    }
}

void MenuSceneView::stopDragging()
{
    m_isDragging = false;
}
