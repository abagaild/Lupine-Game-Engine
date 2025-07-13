#include <glad/glad.h>

// Prevent Windows.h min/max macros from interfering with std::max
#ifdef _WIN32
#define NOMINMAX
#endif

#include "SceneViewPanel.h"
#include "../AssetDropHandler.h"
#include "../PlacementMode.h"
#include "../../core/CrashHandler.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Engine.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/Camera.h"
#include "lupine/rendering/GridRenderer.h"
#include "lupine/rendering/DebugRenderer.h"
#include "lupine/rendering/ViewportManager.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/nodes/Control.h"
#include "lupine/components/Sprite2D.h"
#include "lupine/components/PrimitiveMesh.h"
#include "lupine/components/Label.h"
#include "lupine/components/Button.h"
#include "lupine/components/Panel.h"
#include "lupine/components/ColorRectangle.h"
#include "lupine/components/TextureRectangle.h"
#include "lupine/components/StaticMesh.h"
#include "lupine/components/SkinnedMesh.h"
#include "lupine/physics/PhysicsManager.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QApplication>
#include <QKeyEvent>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <iostream>
#include <functional>
#include <limits>
#include <algorithm>

SceneViewPanel::SceneViewPanel(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_scene(nullptr)
    , m_engine(nullptr)
    , m_viewMode(ViewMode::Mode3D)
    , m_isRotating(false)
    , m_isPanning(false)
    , m_isZooming(false)
    , m_cameraSpeed(5.0f)
    , m_mouseSensitivity(0.1f)
    , m_zoomSpeed(0.1f)
    , m_panSpeed(1.0f)
    , m_zoom2D(1.0f)
    , m_pan2D(0.0f, 0.0f)
    , m_cameraDistance(10.0f)
    , m_cameraYaw(0.0f)
    , m_cameraPitch(0.0f)
    , m_cameraTarget(0.0f, 0.0f, 0.0f)
    , m_gridVisible(true)
    , m_gridConfig(std::make_unique<Lupine::GridConfig>(Lupine::GridRenderer::GetDefaultConfig()))
    , m_selectedNode(nullptr)
    , m_isDraggingGizmo(false)
    , m_gizmoRenderer(std::make_unique<GizmoRenderer>())
    , m_activeGizmo(GizmoType::Move)
    , m_hoveredGizmoAxis(GizmoAxis::None)
    , m_draggedGizmoAxis(GizmoAxis::None)
    , m_dragStartPosition(0.0f)
    , m_dragStartRotation(1.0f, 0.0f, 0.0f, 0.0f)
    , m_dragStartScale(1.0f)
    , m_dragStartMouseWorld(0.0f)
    , m_dragStartAngle(0.0f)
    , m_assetDropHandler(new AssetDropHandler(this))
    , m_isDragging(false)
    , m_placementMode(nullptr)
    , m_isFullyInitialized(false)
    , m_blockSelectionSignals(false)
{
    // Set up asset drop handler
    m_assetDropHandler->setSceneViewPanel(this);

    // Connect asset drop handler signals
    connect(m_assetDropHandler, &AssetDropHandler::nodeCreatedFromAsset,
            [this](Lupine::Node* node, const QString& filePath) {
                Q_UNUSED(filePath)
                emit nodeCreated(node);
            });

    // Create editor camera
    m_editorCamera = std::make_unique<Lupine::Camera>(Lupine::ProjectionType::Perspective);

    // Enable mouse tracking and set focus policy
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // Enable drag and drop
    setAcceptDrops(true);

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &SceneViewPanel::updateScene);
    // Don't start timer immediately - wait for proper initialization
    // m_updateTimer->start(16); // ~60 FPS

    // Enable keyboard focus for key events
    setFocusPolicy(Qt::StrongFocus);

    // Initialize camera
    resetCamera();
}

SceneViewPanel::~SceneViewPanel()
{
}

void SceneViewPanel::setScene(Lupine::Scene* scene)
{
    try {
        qDebug() << "SceneViewPanel::setScene - Setting scene:" << (scene ? "valid" : "null");
        m_scene = scene;

    // Only initialize OpenGL-related components if the context is ready
    if (context() && context()->isValid()) {
        // Ensure grid renderer is initialized when setting a new scene
        // Don't shutdown/reinitialize as this can cause the grid to disappear
        if (!Lupine::GridRenderer::IsInitialized()) {
            makeCurrent(); // Ensure we have the correct OpenGL context
            Lupine::GridRenderer::Initialize();
            doneCurrent();
        }

        // Ensure grid is visible
        m_gridVisible = true;

        // Reset camera when loading a new scene to ensure proper view
        if (scene) {
            resetCamera();
        }

        // Force a full repaint to ensure grid is visible
        qDebug() << "SceneViewPanel::setScene - Triggering update";
        update();
        qDebug() << "SceneViewPanel::setScene - Update triggered successfully";
    } else {
        // OpenGL context not ready yet, just store the scene
        // The rendering will be set up when initializeGL is called
        qDebug() << "SceneViewPanel::setScene - OpenGL context not ready, deferring initialization";
    }

    qDebug() << "SceneViewPanel::setScene - Scene set successfully";

    } catch (const std::exception& e) {
        qCritical() << "SceneViewPanel::setScene - Exception:" << e.what();
    } catch (...) {
        qCritical() << "SceneViewPanel::setScene - Unknown exception occurred";
    }
}

void SceneViewPanel::setEngine(Lupine::Engine* engine)
{
    m_engine = engine;
    updateTextureFiltering();
}

void SceneViewPanel::updateTextureFiltering()
{
    if (!m_engine || !m_engine->GetCurrentProject()) {
        return;
    }

    // Get texture filtering setting from project
    std::string stretchStyleStr = m_engine->GetCurrentProject()->GetSettingValue<std::string>("rendering/stretch_style", "bilinear");

    Lupine::TextureFilter filter = Lupine::TextureFilter::Bilinear;
    if (stretchStyleStr == "nearest") {
        filter = Lupine::TextureFilter::Nearest;
    } else if (stretchStyleStr == "bicubic") {
        filter = Lupine::TextureFilter::Bicubic;
    } else {
        filter = Lupine::TextureFilter::Bilinear;
    }

    // Apply the texture filter setting
    Lupine::ResourceManager::SetTextureFilter(filter);
}

void SceneViewPanel::setViewMode(ViewMode mode)
{
    m_viewMode = mode;

    // Check if current selection is appropriate for new view mode
    if (m_selectedNode) {
        bool nodeIsAppropriate = false;
        if (mode == ViewMode::Mode2D) {
            // In 2D view, only 2D nodes are appropriate
            nodeIsAppropriate = (dynamic_cast<Lupine::Node2D*>(m_selectedNode) != nullptr) ||
                               (dynamic_cast<Lupine::Control*>(m_selectedNode) != nullptr);
        } else {
            // In 3D view, only 3D nodes are appropriate
            nodeIsAppropriate = (dynamic_cast<Lupine::Node3D*>(m_selectedNode) != nullptr);
        }

        if (!nodeIsAppropriate) {
            setSelectedNode(nullptr); // Clear selection if not appropriate
        }
    }

    if (mode == ViewMode::Mode2D) {
        m_editorCamera->SetProjectionType(Lupine::ProjectionType::Orthographic);
        update2DCamera();

        // Enable 2D debug flags
        if (Lupine::DebugRenderer::IsInitialized()) {
            Lupine::DebugRenderer::EnableDebugFlag(Lupine::DebugRenderer::DebugFlags::Camera2DBounds);
            Lupine::DebugRenderer::EnableDebugFlag(Lupine::DebugRenderer::DebugFlags::ScreenBounds);
            Lupine::DebugRenderer::EnableDebugFlag(Lupine::DebugRenderer::DebugFlags::GameBounds);
            Lupine::DebugRenderer::EnableDebugFlag(Lupine::DebugRenderer::DebugFlags::CollisionShapes2D);
            Lupine::DebugRenderer::DisableDebugFlag(Lupine::DebugRenderer::DebugFlags::Camera3DFrustum);
            Lupine::DebugRenderer::DisableDebugFlag(Lupine::DebugRenderer::DebugFlags::CollisionShapes3D);
        }
    } else {
        m_editorCamera->SetProjectionType(Lupine::ProjectionType::Perspective);
        update3DCamera();

        // Enable 3D debug flags
        if (Lupine::DebugRenderer::IsInitialized()) {
            Lupine::DebugRenderer::EnableDebugFlag(Lupine::DebugRenderer::DebugFlags::Camera3DFrustum);
            Lupine::DebugRenderer::EnableDebugFlag(Lupine::DebugRenderer::DebugFlags::Lights);
            Lupine::DebugRenderer::EnableDebugFlag(Lupine::DebugRenderer::DebugFlags::CollisionShapes3D);
            Lupine::DebugRenderer::DisableDebugFlag(Lupine::DebugRenderer::DebugFlags::Camera2DBounds);
            Lupine::DebugRenderer::DisableDebugFlag(Lupine::DebugRenderer::DebugFlags::ScreenBounds);
            Lupine::DebugRenderer::DisableDebugFlag(Lupine::DebugRenderer::DebugFlags::GameBounds);
            Lupine::DebugRenderer::DisableDebugFlag(Lupine::DebugRenderer::DebugFlags::CollisionShapes2D);
        }
    }

    updateCameraMatrices();
    update();
}

void SceneViewPanel::resetCamera()
{
    if (m_viewMode == ViewMode::Mode2D) {
        m_zoom2D = 1.0f;
        m_pan2D = glm::vec2(0.0f, 0.0f);
        update2DCamera();
    } else {
        m_cameraDistance = 10.0f;
        m_cameraYaw = 0.0f;
        m_cameraPitch = 0.0f;
        m_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        update3DCamera();
    }

    updateCameraMatrices();
    update();
}

void SceneViewPanel::focusOnScene()
{
    // TODO: Calculate scene bounds and focus camera on it
    resetCamera();
}

void SceneViewPanel::initializeGL()
{
    LUPINE_SAFE_EXECUTE({
        qDebug() << "SceneViewPanel::initializeGL - Starting OpenGL initialization";
        LUPINE_LOG_STARTUP("SceneViewPanel: Starting OpenGL initialization");

        // Validate OpenGL context
        if (!context() || !context()->isValid()) {
            LUPINE_LOG_CRITICAL("SceneViewPanel::initializeGL - Invalid OpenGL context");
            LUPINE_LOG_STARTUP("SceneViewPanel: ERROR - Invalid OpenGL context");
            return;
        }
        LUPINE_LOG_STARTUP("SceneViewPanel: OpenGL context validated");

        // Initialize OpenGL functions
        LUPINE_SAFE_EXECUTE(
            initializeOpenGLFunctions(),
            "Failed to initialize OpenGL functions"
        );

        // Initialize GLAD
        LUPINE_LOG_STARTUP("SceneViewPanel: Initializing GLAD");
        if (!gladLoadGL()) {
            LUPINE_LOG_CRITICAL("Failed to initialize GLAD in SceneViewPanel");
            LUPINE_LOG_STARTUP("SceneViewPanel: ERROR - Failed to initialize GLAD");
            return;
        }

        qDebug() << "SceneViewPanel::initializeGL - GLAD initialized successfully";
        LUPINE_LOG_STARTUP("SceneViewPanel: GLAD initialized successfully");

        // Check OpenGL version
        const char* version = (const char*)glGetString(GL_VERSION);
        const char* vendor = (const char*)glGetString(GL_VENDOR);
        const char* renderer = (const char*)glGetString(GL_RENDERER);

        qDebug() << "OpenGL Version:" << (version ? version : "Unknown");
        qDebug() << "OpenGL Vendor:" << (vendor ? vendor : "Unknown");
        qDebug() << "OpenGL Renderer:" << (renderer ? renderer : "Unknown");

        // Initialize renderer if not already done
        if (!Lupine::Renderer::GetDefaultShader()) {
            LUPINE_SAFE_EXECUTE(
                Lupine::Renderer::Initialize(),
                "Failed to initialize Lupine Renderer"
            );
            qDebug() << "Initializing Renderer...";

            // Now that renderer is initialized, initialize ResourceManager
            if (!Lupine::ResourceManager::Initialize()) {
                LUPINE_LOG_CRITICAL("Failed to initialize ResourceManager after renderer");
                qDebug() << "Failed to initialize ResourceManager";
            } else {
                qDebug() << "ResourceManager initialized successfully";
            }
        }

        // Initialize grid renderer
        if (!Lupine::GridRenderer::IsInitialized()) {
            LUPINE_SAFE_EXECUTE(
                Lupine::GridRenderer::Initialize(),
                "Failed to initialize GridRenderer"
            );
            qDebug() << "Initializing GridRenderer...";
        }

        // Initialize debug renderer
        if (!Lupine::DebugRenderer::IsInitialized()) {
            LUPINE_SAFE_EXECUTE({
                Lupine::DebugRenderer::Initialize();
                // Enable camera debug drawing by default in 2D view
                Lupine::DebugRenderer::EnableDebugFlag(Lupine::DebugRenderer::DebugFlags::Camera2DBounds);
                Lupine::DebugRenderer::EnableDebugFlag(Lupine::DebugRenderer::DebugFlags::ScreenBounds);
                Lupine::DebugRenderer::EnableDebugFlag(Lupine::DebugRenderer::DebugFlags::GameBounds);
            }, "Failed to initialize DebugRenderer");
            qDebug() << "DebugRenderer initialized";
        }

        // Initialize gizmo renderer
        if (m_gizmoRenderer) {
            LUPINE_SAFE_EXECUTE(
                m_gizmoRenderer->Initialize(),
                "Failed to initialize GizmoRenderer"
            );
        } else {
            LUPINE_LOG_CRITICAL("GizmoRenderer is null during initialization");
        }

        // Set OpenGL state
        LUPINE_SAFE_EXECUTE({
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.12f, 0.12f, 0.14f, 1.0f); // Dark background to match theme

            // Check for OpenGL errors
            GLenum error = glGetError();
            if (error != GL_NO_ERROR) {
                LUPINE_LOG_CRITICAL("OpenGL error during initialization: " + std::to_string(error));
            }
        }, "Failed to set OpenGL state");

        qDebug() << "SceneViewPanel::initializeGL - OpenGL initialization completed successfully";
        LUPINE_LOG_STARTUP("SceneViewPanel: OpenGL initialization completed successfully");

        // Mark as fully initialized
        m_isFullyInitialized = true;

        // Start the update timer with a slight delay to ensure everything is ready
        if (m_updateTimer && !m_updateTimer->isActive()) {
            qDebug() << "SceneViewPanel::initializeGL - Starting update timer with delay";
            LUPINE_LOG_STARTUP("SceneViewPanel: Starting update timer with delay");

            // Use a single-shot timer to delay the start of the update timer
            QTimer::singleShot(1000, this, [this]() {
                if (m_updateTimer && m_isFullyInitialized) {
                    qDebug() << "SceneViewPanel - Starting delayed update timer";
                    LUPINE_LOG_STARTUP("SceneViewPanel: Starting delayed update timer");
                    m_updateTimer->start(16); // ~60 FPS
                }
            });
        }

    }, "Critical error during OpenGL initialization");
}

void SceneViewPanel::paintGL()
{
    // Safety check: ensure we have a valid OpenGL context
    if (!context() || !context()->isValid()) {
        qWarning() << "SceneViewPanel::paintGL - Invalid OpenGL context";
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_editorCamera) {
        // Update camera matrices
        updateCameraMatrices();

        // Apply project texture filtering settings
        updateTextureFiltering();

        // Render grid if visible
        if (m_gridVisible && m_gridConfig) {
            // Ensure grid renderer is still initialized
            if (!Lupine::GridRenderer::IsInitialized()) {
                Lupine::GridRenderer::Initialize();
            }

            if (m_viewMode == ViewMode::Mode2D) {
                Lupine::GridConfig config2D = Lupine::GridRenderer::GetDefault2DConfig();
                Lupine::GridRenderer::Render2DGrid(m_editorCamera.get(), config2D);
            } else {
                Lupine::GridConfig config3D = Lupine::GridRenderer::GetDefault3DConfig();
                Lupine::GridRenderer::Render3DGrid(m_editorCamera.get(), config3D);
            }
        }

        // Set rendering context based on view mode
        if (m_viewMode == ViewMode::Mode2D) {
            Lupine::Renderer::SetRenderingContext(Lupine::RenderingContext::Editor2D);
        } else {
            Lupine::Renderer::SetRenderingContext(Lupine::RenderingContext::Editor3D);
        }

        // Render the scene using the renderer (don't clear screen since we already did)
        if (m_scene) {
            Lupine::Renderer::RenderScene(m_scene, m_editorCamera.get(), false);
        }



        // Render placement mode ghost if active
        if (m_placementMode && m_placementMode->isGhostVisible()) {
            m_placementMode->renderGhost(m_editorCamera.get());
        }

        // Render selection highlight for selected node
        if (m_selectedNode) {
            renderSelectionHighlight(m_selectedNode);
        }

        // Render debug info (camera bounds, etc.)
        if (m_scene && Lupine::DebugRenderer::IsInitialized()) {
            Lupine::DebugRenderer::RenderDebugInfo(m_scene, m_editorCamera.get());
        }

        // Render gizmos for selected node (only if appropriate for current view)
        if (m_selectedNode && m_activeGizmo != GizmoType::None) {
            bool shouldRenderGizmo = false;
            if (m_viewMode == ViewMode::Mode2D) {
                // In 2D view, only show gizmos for 2D nodes
                shouldRenderGizmo = (dynamic_cast<Lupine::Node2D*>(m_selectedNode) != nullptr) ||
                                   (dynamic_cast<Lupine::Control*>(m_selectedNode) != nullptr);
            } else {
                // In 3D view, only show gizmos for 3D nodes
                shouldRenderGizmo = (dynamic_cast<Lupine::Node3D*>(m_selectedNode) != nullptr);
            }

            if (shouldRenderGizmo) {
                m_gizmoRenderer->RenderGizmos(m_editorCamera.get(), m_selectedNode, m_activeGizmo);
            }
        }
    }
}

void SceneViewPanel::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);

    // Update camera projection with new aspect ratio
    if (m_editorCamera && width > 0 && height > 0) {
        if (m_viewMode == ViewMode::Mode2D) {
            update2DCamera();
        } else {
            update3DCamera();
        }

        updateCameraMatrices();
    }
}

void SceneViewPanel::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos();

    if (event->button() == Qt::LeftButton) {
        // Handle selection or gizmo interaction
        handleMouseSelection(event->pos());
    } else if (event->button() == Qt::RightButton) {
        m_isRotating = true;
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
    }

    // Grab focus for keyboard events
    setFocus();
}

void SceneViewPanel::mouseMoveEvent(QMouseEvent* event)
{
    QPoint delta = event->pos() - m_lastMousePos;

    if (m_isDraggingGizmo) {
        updateGizmoDrag(event->pos());
    } else if (m_isRotating) {
        handleMouseRotation(delta);
    } else if (m_isPanning) {
        handleMousePanning(delta);
    }

    m_lastMousePos = event->pos();
    update(); // Trigger repaint for gizmo highlighting
}

void SceneViewPanel::wheelEvent(QWheelEvent* event)
{
    float delta = event->angleDelta().y() / 120.0f; // Standard wheel step
    handleMouseZoom(delta);
}

void SceneViewPanel::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_isDraggingGizmo) {
        endGizmoDrag();
    } else if (event->button() == Qt::RightButton) {
        m_isRotating = false;
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }
}

void SceneViewPanel::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
        case Qt::Key_F:
            focusOnScene();
            break;
        case Qt::Key_R:
            resetCamera();
            break;
        case Qt::Key_2:
            setViewMode(ViewMode::Mode2D);
            break;
        case Qt::Key_3:
            setViewMode(ViewMode::Mode3D);
            break;
        default:
            QOpenGLWidget::keyPressEvent(event);
            break;
    }
}

void SceneViewPanel::updateScene()
{
    try {
        // Safety check: ensure we're fully initialized
        if (!m_isFullyInitialized) {
            return;
        }

        // Safety check: ensure we have a valid OpenGL context
        if (!context() || !context()->isValid()) {
            // Stop the timer if context becomes invalid
            if (m_updateTimer && m_updateTimer->isActive()) {
                m_updateTimer->stop();
            }
            return;
        }

        // Update scene components if we have a scene
        if (m_scene) {
            // Update scene components (this loads export variables and updates component state)
            m_scene->OnUpdate(0.016f); // ~60 FPS delta time
        }

        // Always update the view to ensure grid is visible even without a scene
        update();

    } catch (const std::exception& e) {
        qCritical() << "SceneViewPanel::updateScene - Exception:" << e.what();
        // Stop timer on exception to prevent repeated crashes
        if (m_updateTimer && m_updateTimer->isActive()) {
            m_updateTimer->stop();
        }
    } catch (...) {
        qCritical() << "SceneViewPanel::updateScene - Unknown exception occurred";
        // Stop timer on exception to prevent repeated crashes
        if (m_updateTimer && m_updateTimer->isActive()) {
            m_updateTimer->stop();
        }
    }
}

void SceneViewPanel::forceSceneRefresh()
{
    // Force OpenGL context to refresh
    makeCurrent();

    // Update camera matrices
    updateCameraMatrices();

    // Force a complete repaint
    update();

    // Process any pending events to ensure immediate refresh
    QApplication::processEvents();
}

void SceneViewPanel::updateCameraMatrices()
{
    if (m_editorCamera) {
        m_editorCamera->UpdateMatrices();
    }
}

void SceneViewPanel::update2DCamera()
{
    if (!m_editorCamera) return;

    // Calculate aspect ratio from widget size to prevent squashing/stretching
    float aspectRatio = static_cast<float>(width()) / static_cast<float>(height());

    // Use a base height for consistent scaling, adjust width based on aspect ratio
    float baseHeight = 1080.0f; // Base reference height
    float baseWidth = baseHeight * aspectRatio;

    float halfHeight = (baseHeight * 0.5f) / m_zoom2D;
    float halfWidth = (baseWidth * 0.5f) / m_zoom2D;

    // Apply pan offset to orthographic bounds so sprites move with the grid
    float left = -halfWidth + m_pan2D.x;
    float right = halfWidth + m_pan2D.x;
    float bottom = -halfHeight + m_pan2D.y;
    float top = halfHeight + m_pan2D.y;

    m_editorCamera->SetOrthographic(left, right, bottom, top, -100.0f, 100.0f);

    // Set camera position for 2D view (no pan offset needed since it's in the projection)
    glm::vec3 position(0.0f, 0.0f, 5.0f);
    m_editorCamera->SetPosition(position);
    m_editorCamera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
}

void SceneViewPanel::update3DCamera()
{
    if (!m_editorCamera) return;

    // Calculate aspect ratio from widget size
    float aspectRatio = static_cast<float>(width()) / static_cast<float>(height());

    // Set perspective projection for 3D
    m_editorCamera->SetPerspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

    // Calculate camera position from spherical coordinates
    float yawRad = glm::radians(m_cameraYaw);
    float pitchRad = glm::radians(m_cameraPitch);

    glm::vec3 position;
    position.x = m_cameraTarget.x + m_cameraDistance * cos(pitchRad) * cos(yawRad);
    position.y = m_cameraTarget.y + m_cameraDistance * sin(pitchRad);
    position.z = m_cameraTarget.z + m_cameraDistance * cos(pitchRad) * sin(yawRad);

    m_editorCamera->SetPosition(position);
    m_editorCamera->SetTarget(m_cameraTarget);
}

void SceneViewPanel::handleMouseRotation(const QPoint& delta)
{
    if (m_viewMode == ViewMode::Mode3D) {
        // 3D rotation: orbit around target
        m_cameraYaw += delta.x() * m_mouseSensitivity;
        m_cameraPitch -= delta.y() * m_mouseSensitivity;

        // Clamp pitch to avoid gimbal lock
        m_cameraPitch = glm::clamp(m_cameraPitch, -89.0f, 89.0f);

        update3DCamera();
        updateCameraMatrices();
        update();
    }
}

void SceneViewPanel::handleMousePanning(const QPoint& delta)
{
    if (m_viewMode == ViewMode::Mode2D) {
        // 2D panning - scale by zoom level for consistent feel
        // Calculate world space movement based on current zoom and widget size
        float aspectRatio = static_cast<float>(width()) / static_cast<float>(height());
        float baseHeight = 1080.0f;
        float baseWidth = baseHeight * aspectRatio;

        // Convert pixel delta to world space delta
        float worldDeltaX = (delta.x() * baseWidth) / (width() * m_zoom2D);
        float worldDeltaY = (delta.y() * baseHeight) / (height() * m_zoom2D);

        // Apply panning (negative X for intuitive direction)
        m_pan2D.x -= worldDeltaX;
        m_pan2D.y += worldDeltaY; // Positive Y for intuitive direction

        update2DCamera();
    } else {
        // 3D panning: move target point
        glm::vec3 right = m_editorCamera->GetRight();
        glm::vec3 up = m_editorCamera->GetUp();

        float panSpeed = m_cameraDistance * 0.001f;
        glm::vec3 panOffset = -right * (float)delta.x() * panSpeed + up * (float)delta.y() * panSpeed;

        m_cameraTarget += panOffset;
        update3DCamera();
    }

    updateCameraMatrices();
    update();
}

void SceneViewPanel::handleMouseZoom(float delta)
{
    if (m_viewMode == ViewMode::Mode2D) {
        // 2D zoom
        m_zoom2D *= (1.0f + delta * m_zoomSpeed);
        m_zoom2D = glm::clamp(m_zoom2D, 0.1f, 10.0f);

        update2DCamera();
    } else {
        // 3D zoom: change distance to target
        m_cameraDistance *= (1.0f - delta * m_zoomSpeed);
        m_cameraDistance = glm::clamp(m_cameraDistance, 0.5f, 100.0f);

        update3DCamera();
    }

    updateCameraMatrices();
    update();
}

void SceneViewPanel::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    update();
}

void SceneViewPanel::setGridConfig(const Lupine::GridConfig& config)
{
    if (m_gridConfig) {
        *m_gridConfig = config;
        update();
    }
}

const Lupine::GridConfig& SceneViewPanel::getGridConfig() const
{
    return *m_gridConfig;
}

void SceneViewPanel::setSelectedNode(Lupine::Node* node)
{
    // Add safety check to prevent crashes from invalid nodes
    if (node && !isNodeValid(node)) {
        node = nullptr;
    }

    if (m_selectedNode != node) {
        m_selectedNode = node;
        update(); // Trigger repaint to show/hide selection highlight

        // Only emit signal if we're not in the middle of programmatic selection
        if (!m_blockSelectionSignals) {
            emit nodeSelected(node);
        }
    }
}

void SceneViewPanel::setSelectedNodeSilently(Lupine::Node* node)
{
    // Add safety check to prevent crashes from invalid nodes
    if (node && !isNodeValid(node)) {
        node = nullptr;
    }

    // Block signals during this operation
    m_blockSelectionSignals = true;

    if (m_selectedNode != node) {
        m_selectedNode = node;
        update(); // Trigger repaint to show/hide selection highlight
    }

    // Re-enable signals
    m_blockSelectionSignals = false;
}

void SceneViewPanel::setPlacementMode(PlacementMode* placementMode)
{
    m_placementMode = placementMode;
}

void SceneViewPanel::setActiveGizmo(GizmoType gizmo)
{
    if (m_activeGizmo != gizmo) {
        m_activeGizmo = gizmo;
        update(); // Trigger repaint to show new gizmo
    }
}

void SceneViewPanel::handleMouseSelection(const QPoint& mousePos)
{
    // Check if clicking on gizmo first
    if (m_selectedNode && m_activeGizmo != GizmoType::None) {
        GizmoHitResult gizmoHit = testGizmoHit(mousePos);
        if (gizmoHit.hit) {
            // Start gizmo dragging
            startGizmoDrag(mousePos, gizmoHit.axis);
            return;
        }
    }

    // Perform raycast to find clicked node
    Lupine::Node* clickedNode = performRaycast(mousePos);
    setSelectedNode(clickedNode);
}

Lupine::Node* SceneViewPanel::performRaycast(const QPoint& mousePos)
{
    if (!m_scene || !m_editorCamera) {
        return nullptr;
    }

    glm::vec3 rayOrigin, rayDirection;
    screenToWorldRay(mousePos, rayOrigin, rayDirection);

    // Find the closest intersected node
    Lupine::Node* closestNode = nullptr;
    float closestDistance = std::numeric_limits<float>::max();

    std::function<void(Lupine::Node*)> checkNode = [&](Lupine::Node* node) {
        if (!node || !node->IsActive()) return;

        // Filter nodes based on view mode
        bool nodeIsSelectable = false;
        if (m_viewMode == ViewMode::Mode2D) {
            // In 2D view, only select 2D nodes (Node2D, Control)
            nodeIsSelectable = (dynamic_cast<Lupine::Node2D*>(node) != nullptr) ||
                              (dynamic_cast<Lupine::Control*>(node) != nullptr);
        } else {
            // In 3D view, only select 3D nodes (Node3D)
            nodeIsSelectable = (dynamic_cast<Lupine::Node3D*>(node) != nullptr);
        }

        if (nodeIsSelectable) {
            float distance;
            if (rayIntersectsNode(rayOrigin, rayDirection, node, distance)) {
                if (distance < closestDistance) {
                    closestDistance = distance;
                    closestNode = node;
                }
            }
        }

        // Check children regardless of current node selectability
        for (const auto& child : node->GetChildren()) {
            checkNode(child.get());
        }
    };

    checkNode(m_scene->GetRootNode());
    return closestNode;
}

glm::vec3 SceneViewPanel::screenToWorldRay(const QPoint& screenPos, glm::vec3& rayOrigin, glm::vec3& rayDirection)
{
    if (!m_editorCamera) {
        rayOrigin = glm::vec3(0.0f);
        rayDirection = glm::vec3(0.0f, 0.0f, -1.0f);
        return rayDirection;
    }

    // Get viewport dimensions
    int viewportWidth = width();
    int viewportHeight = height();

    // Convert screen coordinates to normalized device coordinates (-1 to 1)
    float x = (2.0f * screenPos.x()) / viewportWidth - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / viewportHeight; // Flip Y

    if (m_viewMode == ViewMode::Mode2D) {
        // For 2D mode (orthographic projection)
        glm::mat4 invProjection = glm::inverse(m_editorCamera->GetProjectionMatrix());
        glm::mat4 invView = glm::inverse(m_editorCamera->GetViewMatrix());

        // Convert NDC to world space
        glm::vec4 worldPos = invView * invProjection * glm::vec4(x, y, 0.0f, 1.0f);
        rayOrigin = glm::vec3(worldPos) / worldPos.w;
        rayDirection = glm::vec3(0.0f, 0.0f, -1.0f); // Forward direction in 2D
    } else {
        // For 3D mode (perspective projection)
        rayOrigin = m_editorCamera->GetPosition();

        // Create ray direction from camera to mouse position
        glm::mat4 invProjection = glm::inverse(m_editorCamera->GetProjectionMatrix());
        glm::mat4 invView = glm::inverse(m_editorCamera->GetViewMatrix());

        glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
        glm::vec4 rayEye = invProjection * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
        glm::vec4 rayWorld = invView * rayEye;
        rayDirection = glm::normalize(glm::vec3(rayWorld));
    }

    return rayDirection;
}

bool SceneViewPanel::rayIntersectsNode(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, Lupine::Node* node, float& distance)
{
    if (!node || !node->IsActive()) {
        return false;
    }

    // Get node's world position and calculate accurate bounding box
    glm::vec3 nodePos(0.0f);
    glm::vec3 nodeSize(1.0f); // Default size
    bool hasValidBounds = false;

    // Get position and size based on node type
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
        glm::vec2 pos2d = node2d->GetGlobalPosition();
        glm::vec2 scale2d = node2d->GetGlobalScale();
        nodePos = glm::vec3(pos2d.x, pos2d.y, 0.0f);

        // Check for Sprite2D component to get actual size
        if (auto* sprite = node->GetComponent<Lupine::Sprite2D>()) {
            glm::vec4 rect = sprite->GetRect();
            // Apply node scale to sprite size
            nodeSize = glm::vec3(rect.z * scale2d.x, rect.w * scale2d.y, 0.2f);
            hasValidBounds = true;
        }
        // Check for Label component
        else if (auto* label = node->GetComponent<Lupine::Label>()) {
            glm::vec2 textSize = label->CalculateTextSize();
            // Apply node scale to text size
            nodeSize = glm::vec3(textSize.x * scale2d.x, textSize.y * scale2d.y, 0.2f);
            hasValidBounds = true;
        }

        if (!hasValidBounds) {
            // Default 2D size with scale applied
            nodeSize = glm::vec3(0.5f * scale2d.x, 0.5f * scale2d.y, 0.2f);
        }
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
        nodePos = node3d->GetGlobalPosition();
        glm::vec3 scale3d = node3d->GetGlobalScale();

        // Check for PrimitiveMesh component to get actual size
        if (auto* mesh = node->GetComponent<Lupine::PrimitiveMesh>()) {
            glm::vec3 meshSize = mesh->GetSize();
            // Apply node scale to mesh size
            nodeSize = meshSize * scale3d;
            hasValidBounds = true;
        }

        if (!hasValidBounds) {
            // Default 3D size with scale applied
            nodeSize = glm::vec3(1.0f) * scale3d;
        }
    } else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        glm::vec4 globalRect = control->GetGlobalRect();
        nodePos = glm::vec3(globalRect.x + globalRect.z * 0.5f, globalRect.y + globalRect.w * 0.5f, 0.0f);
        nodeSize = glm::vec3(globalRect.z, globalRect.w, 0.2f);

        // Check for Label component to get more accurate text bounds
        if (auto* label = node->GetComponent<Lupine::Label>()) {
            glm::vec2 textSize = label->CalculateTextSize();
            if (textSize.x > 0 && textSize.y > 0) {
                nodeSize = glm::vec3(textSize.x, textSize.y, 0.2f);
            }
        }
        hasValidBounds = true;
    }

    // Ensure minimum size for selection
    nodeSize = glm::max(nodeSize, glm::vec3(0.1f, 0.1f, 0.1f));

    // Create axis-aligned bounding box
    glm::vec3 minBounds = nodePos - nodeSize * 0.5f;
    glm::vec3 maxBounds = nodePos + nodeSize * 0.5f;

    // Ray-AABB intersection test with improved handling
    float tNear = -std::numeric_limits<float>::max();
    float tFar = std::numeric_limits<float>::max();

    for (int i = 0; i < 3; ++i) {
        if (glm::abs(rayDirection[i]) < 1e-8f) {
            // Ray is parallel to the slab
            if (rayOrigin[i] < minBounds[i] || rayOrigin[i] > maxBounds[i]) {
                return false; // Ray is outside the slab
            }
        } else {
            // Calculate intersection distances
            float t1 = (minBounds[i] - rayOrigin[i]) / rayDirection[i];
            float t2 = (maxBounds[i] - rayOrigin[i]) / rayDirection[i];

            if (t1 > t2) {
                std::swap(t1, t2);
            }

            tNear = glm::max(tNear, t1);
            tFar = glm::min(tFar, t2);

            if (tNear > tFar) {
                return false; // No intersection
            }
        }
    }

    // Check if intersection is in front of ray origin
    if (tFar >= 0.0f) {
        distance = tNear >= 0.0f ? tNear : tFar;
        return true;
    }

    return false;
}

glm::vec2 SceneViewPanel::worldToScreen(const glm::vec3& worldPos)
{
    if (!m_editorCamera) {
        return glm::vec2(0.0f);
    }

    // Transform world position to screen coordinates
    glm::mat4 viewProjection = m_editorCamera->GetViewProjectionMatrix();
    glm::vec4 clipPos = viewProjection * glm::vec4(worldPos, 1.0f);

    // Perspective divide
    if (clipPos.w != 0.0f) {
        clipPos /= clipPos.w;
    }

    // Convert from NDC (-1 to 1) to screen coordinates
    float screenX = (clipPos.x + 1.0f) * 0.5f * width();
    float screenY = (1.0f - clipPos.y) * 0.5f * height(); // Flip Y

    return glm::vec2(screenX, screenY);
}

void SceneViewPanel::renderSelectionHighlight(Lupine::Node* node)
{
    if (!node || !m_editorCamera) return;

    // Calculate node's bounding box for highlighting
    glm::vec3 nodePos(0.0f);
    glm::vec3 nodeSize(1.0f);
    bool hasValidBounds = false;

    // Get position and size based on node type (similar to hit testing logic)
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(node)) {
        glm::vec2 pos2d = node2d->GetGlobalPosition();
        glm::vec2 scale2d = node2d->GetGlobalScale();
        nodePos = glm::vec3(pos2d.x, pos2d.y, 0.0f);

        // Check for visual components and use the largest bounds
        glm::vec3 maxSize(0.0f);

        // Check for Sprite2D component
        if (auto* sprite = node->GetComponent<Lupine::Sprite2D>()) {
            glm::vec4 rect = sprite->GetRect();
            glm::vec3 spriteSize = glm::vec3(rect.z * scale2d.x, rect.w * scale2d.y, 0.2f);
            maxSize = glm::max(maxSize, spriteSize);
            hasValidBounds = true;
        }

        // Check for Label component
        if (auto* label = node->GetComponent<Lupine::Label>()) {
            glm::vec2 textSize = label->CalculateTextSize();
            glm::vec3 labelSize = glm::vec3(textSize.x * scale2d.x, textSize.y * scale2d.y, 0.2f);
            maxSize = glm::max(maxSize, labelSize);
            hasValidBounds = true;
        }

        // Check for Button component (which has both background and text)
        if (auto* button = node->GetComponent<Lupine::Button>()) {
            // Buttons typically have a default size when attached to Node2D
            glm::vec3 buttonSize = glm::vec3(100.0f * scale2d.x, 30.0f * scale2d.y, 0.2f);
            maxSize = glm::max(maxSize, buttonSize);
            hasValidBounds = true;
        }

        // Check for Panel component
        if (auto* panel = node->GetComponent<Lupine::Panel>()) {
            // Panels typically have a default size when attached to Node2D
            glm::vec3 panelSize = glm::vec3(100.0f * scale2d.x, 100.0f * scale2d.y, 0.2f);
            maxSize = glm::max(maxSize, panelSize);
            hasValidBounds = true;
        }

        // Check for ColorRectangle component
        if (auto* colorRect = node->GetComponent<Lupine::ColorRectangle>()) {
            // ColorRectangles typically have a default size when attached to Node2D
            glm::vec3 rectSize = glm::vec3(50.0f * scale2d.x, 50.0f * scale2d.y, 0.2f);
            maxSize = glm::max(maxSize, rectSize);
            hasValidBounds = true;
        }

        if (hasValidBounds) {
            nodeSize = maxSize;
        }

        if (!hasValidBounds) {
            nodeSize = glm::vec3(0.5f * scale2d.x, 0.5f * scale2d.y, 0.2f);
        }
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(node)) {
        nodePos = node3d->GetGlobalPosition();
        glm::vec3 scale3d = node3d->GetGlobalScale();

        // Check for visual components and use the largest bounds
        glm::vec3 maxSize(0.0f);

        // Check for PrimitiveMesh component
        if (auto* mesh = node->GetComponent<Lupine::PrimitiveMesh>()) {
            glm::vec3 meshSize = mesh->GetSize() * scale3d;
            maxSize = glm::max(maxSize, meshSize);
            hasValidBounds = true;
        }

        // Check for StaticMesh component
        if (auto* staticMesh = node->GetComponent<Lupine::StaticMesh>()) {
            // Use a default size for static meshes since we don't have easy access to mesh bounds
            glm::vec3 meshSize = glm::vec3(2.0f) * scale3d;
            maxSize = glm::max(maxSize, meshSize);
            hasValidBounds = true;
        }

        // Check for SkinnedMesh component
        if (auto* skinnedMesh = node->GetComponent<Lupine::SkinnedMesh>()) {
            // Use a default size for skinned meshes
            glm::vec3 meshSize = glm::vec3(2.0f) * scale3d;
            maxSize = glm::max(maxSize, meshSize);
            hasValidBounds = true;
        }

        if (hasValidBounds) {
            nodeSize = maxSize;
        } else {
            nodeSize = glm::vec3(1.0f) * scale3d;
        }
    } else if (auto* control = dynamic_cast<Lupine::Control*>(node)) {
        glm::vec4 globalRect = control->GetGlobalRect();
        nodePos = glm::vec3(globalRect.x + globalRect.z * 0.5f, globalRect.y + globalRect.w * 0.5f, 0.0f);

        // Start with the control's own size
        glm::vec3 maxSize = glm::vec3(globalRect.z, globalRect.w, 0.2f);

        // Check for visual components and expand bounds if needed

        // Check for Label component
        if (auto* label = node->GetComponent<Lupine::Label>()) {
            glm::vec2 textSize = label->CalculateTextSize();
            if (textSize.x > 0 && textSize.y > 0) {
                glm::vec3 labelSize = glm::vec3(textSize.x, textSize.y, 0.2f);
                maxSize = glm::max(maxSize, labelSize);
            }
        }

        // Check for Button component
        if (auto* button = node->GetComponent<Lupine::Button>()) {
            // Button should use at least the control size, but may be larger for text
            // The control size is already included in maxSize
        }

        // Check for Panel component
        if (auto* panel = node->GetComponent<Lupine::Panel>()) {
            // Panel should use the control size
            // The control size is already included in maxSize
        }

        // Check for ColorRectangle component
        if (auto* colorRect = node->GetComponent<Lupine::ColorRectangle>()) {
            // ColorRectangle should use the control size
            // The control size is already included in maxSize
        }

        // Check for TextureRectangle component
        if (auto* texRect = node->GetComponent<Lupine::TextureRectangle>()) {
            // TextureRectangle should use the control size
            // The control size is already included in maxSize
        }

        nodeSize = maxSize;
        hasValidBounds = true;
    }

    // Render selection outline
    renderBoundingBoxOutline(nodePos, nodeSize, glm::vec3(1.0f, 0.5f, 0.0f)); // Orange outline
}

void SceneViewPanel::renderBoundingBoxOutline(const glm::vec3& center, const glm::vec3& size, const glm::vec3& color)
{
    if (!m_editorCamera) return;

    // Calculate bounding box corners
    glm::vec3 halfSize = size * 0.5f;
    glm::vec3 minBounds = center - halfSize;
    glm::vec3 maxBounds = center + halfSize;

    // Define the 8 corners of the bounding box
    std::vector<glm::vec3> corners = {
        glm::vec3(minBounds.x, minBounds.y, minBounds.z), // 0: min corner
        glm::vec3(maxBounds.x, minBounds.y, minBounds.z), // 1: +X
        glm::vec3(maxBounds.x, maxBounds.y, minBounds.z), // 2: +X+Y
        glm::vec3(minBounds.x, maxBounds.y, minBounds.z), // 3: +Y
        glm::vec3(minBounds.x, minBounds.y, maxBounds.z), // 4: +Z
        glm::vec3(maxBounds.x, minBounds.y, maxBounds.z), // 5: +X+Z
        glm::vec3(maxBounds.x, maxBounds.y, maxBounds.z), // 6: max corner
        glm::vec3(minBounds.x, maxBounds.y, maxBounds.z)  // 7: +Y+Z
    };

    // Define the 12 edges of the bounding box
    std::vector<std::pair<int, int>> edges = {
        // Bottom face (Z = min)
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        // Top face (Z = max)
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        // Vertical edges
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    // Set up OpenGL state for line rendering
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Don't write to depth buffer
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(2.0f);

    // Use a simple line shader (we can reuse the gizmo shader if available)
    if (m_gizmoRenderer) {
        // Render each edge as a line
        for (const auto& edge : edges) {
            glm::vec3 start = corners[edge.first];
            glm::vec3 end = corners[edge.second];

            // For 2D objects, only render the front face if in 2D mode
            if (m_viewMode == ViewMode::Mode2D && size.z <= 0.3f) {
                // Only render the front face (Z = max) for 2D objects
                if (edge.first >= 4 && edge.second >= 4) {
                    // This is a top face edge, render it
                    renderOutlineLine(start, end, color);
                } else if ((edge.first < 4 && edge.second >= 4) || (edge.first >= 4 && edge.second < 4)) {
                    // This is a vertical edge, skip it for 2D
                    continue;
                } else {
                    // This is a bottom face edge, skip it for 2D
                    continue;
                }
            } else {
                // Render all edges for 3D objects
                renderOutlineLine(start, end, color);
            }
        }
    }

    // Restore OpenGL state
    glDepthMask(GL_TRUE);
    glLineWidth(1.0f);
}

void SceneViewPanel::renderOutlineLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color)
{
    // Use the gizmo renderer's line rendering capability for consistency
    if (m_gizmoRenderer) {
        // We need to access the gizmo renderer's line rendering method
        // For now, we'll use a simple OpenGL immediate mode approach

        // Enable basic OpenGL state
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Set up matrices
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadMatrixf(glm::value_ptr(m_editorCamera->GetProjectionMatrix()));

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixf(glm::value_ptr(m_editorCamera->GetViewMatrix()));

        // Set line color and width
        glColor4f(color.r, color.g, color.b, 1.0f);
        glLineWidth(2.0f);

        // Render the line
        glBegin(GL_LINES);
        glVertex3f(start.x, start.y, start.z);
        glVertex3f(end.x, end.y, end.z);
        glEnd();

        // Restore matrices
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);

        // Restore line width
        glLineWidth(1.0f);
    }
}

GizmoHitResult SceneViewPanel::testGizmoHit(const QPoint& mousePos)
{
    if (!m_selectedNode || m_activeGizmo == GizmoType::None) {
        return GizmoHitResult{};
    }

    glm::vec3 rayOrigin, rayDirection;
    screenToWorldRay(mousePos, rayOrigin, rayDirection);

    return m_gizmoRenderer->TestGizmoHit(rayOrigin, rayDirection, m_selectedNode, m_activeGizmo, m_editorCamera.get());
}

void SceneViewPanel::startGizmoDrag(const QPoint& mousePos, GizmoAxis axis)
{
    if (!m_selectedNode) return;

    m_isDraggingGizmo = true;
    m_draggedGizmoAxis = axis;

    // Store initial transform values
    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(m_selectedNode)) {
        m_dragStartPosition = glm::vec3(node2d->GetPosition(), 0.0f);
        m_dragStartRotation = glm::angleAxis(node2d->GetRotation(), glm::vec3(0, 0, 1));
        m_dragStartScale = glm::vec3(node2d->GetScale(), 1.0f);
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(m_selectedNode)) {
        m_dragStartPosition = node3d->GetPosition();
        m_dragStartRotation = node3d->GetRotation();
        m_dragStartScale = node3d->GetScale();
    } else if (auto* control = dynamic_cast<Lupine::Control*>(m_selectedNode)) {
        m_dragStartPosition = glm::vec3(control->GetPosition(), 0.0f);
        m_dragStartRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        m_dragStartScale = glm::vec3(control->GetSize(), 1.0f);
    }

    // Store initial mouse world position
    glm::vec3 rayOrigin, rayDirection;
    screenToWorldRay(mousePos, rayOrigin, rayDirection);
    m_dragStartMouseWorld = rayOrigin;
}

void SceneViewPanel::updateGizmoDrag(const QPoint& mousePos)
{
    if (!m_isDraggingGizmo || !m_selectedNode) return;

    glm::vec3 rayOrigin, rayDirection;
    screenToWorldRay(mousePos, rayOrigin, rayDirection);

    if (m_activeGizmo == GizmoType::Move) {
        updateMoveGizmoDrag(rayOrigin, rayDirection);
    } else if (m_activeGizmo == GizmoType::Rotate) {
        updateRotateGizmoDrag(rayOrigin, rayDirection);
    } else if (m_activeGizmo == GizmoType::Scale) {
        updateScaleGizmoDrag(rayOrigin, rayDirection);
    }

    update(); // Trigger repaint
}

void SceneViewPanel::endGizmoDrag()
{
    m_isDraggingGizmo = false;
    m_draggedGizmoAxis = GizmoAxis::None;
}

void SceneViewPanel::updateMoveGizmoDrag(const glm::vec3& rayOrigin, const glm::vec3& rayDirection)
{
    glm::vec3 nodePosition = m_gizmoRenderer->GetNodeWorldPosition(m_selectedNode);
    glm::vec3 deltaMovement(0.0f);

    if (m_draggedGizmoAxis == GizmoAxis::X) {
        // Project ray onto X axis
        glm::vec3 xAxis(1, 0, 0);
        float t = glm::dot(nodePosition - rayOrigin, xAxis) / glm::dot(rayDirection, xAxis);
        glm::vec3 projectedPoint = rayOrigin + t * rayDirection;
        deltaMovement.x = projectedPoint.x - nodePosition.x;
    } else if (m_draggedGizmoAxis == GizmoAxis::Y) {
        // Project ray onto Y axis
        glm::vec3 yAxis(0, 1, 0);
        float t = glm::dot(nodePosition - rayOrigin, yAxis) / glm::dot(rayDirection, yAxis);
        glm::vec3 projectedPoint = rayOrigin + t * rayDirection;
        deltaMovement.y = projectedPoint.y - nodePosition.y;
    } else if (m_draggedGizmoAxis == GizmoAxis::Z) {
        // Project ray onto Z axis
        glm::vec3 zAxis(0, 0, 1);
        float t = glm::dot(nodePosition - rayOrigin, zAxis) / glm::dot(rayDirection, zAxis);
        glm::vec3 projectedPoint = rayOrigin + t * rayDirection;
        deltaMovement.z = projectedPoint.z - nodePosition.z;
    } else if (m_draggedGizmoAxis == GizmoAxis::XY || m_draggedGizmoAxis == GizmoAxis::XZ || m_draggedGizmoAxis == GizmoAxis::YZ) {
        // Plane movement - intersect ray with appropriate plane
        glm::vec3 planeNormal;
        if (m_draggedGizmoAxis == GizmoAxis::XY) planeNormal = glm::vec3(0, 0, 1);
        else if (m_draggedGizmoAxis == GizmoAxis::XZ) planeNormal = glm::vec3(0, 1, 0);
        else planeNormal = glm::vec3(1, 0, 0);

        float denom = glm::dot(planeNormal, rayDirection);
        if (glm::abs(denom) > 1e-6f) {
            float t = glm::dot(nodePosition - rayOrigin, planeNormal) / denom;
            glm::vec3 hitPoint = rayOrigin + t * rayDirection;
            deltaMovement = hitPoint - nodePosition;

            // Constrain to plane
            if (m_draggedGizmoAxis == GizmoAxis::XY) deltaMovement.z = 0;
            else if (m_draggedGizmoAxis == GizmoAxis::XZ) deltaMovement.y = 0;
            else deltaMovement.x = 0;
        }
    }

    // Apply movement to node
    glm::vec3 newPosition = m_dragStartPosition + deltaMovement;

    if (auto* node2d = dynamic_cast<Lupine::Node2D*>(m_selectedNode)) {
        node2d->SetPosition(glm::vec2(newPosition.x, newPosition.y));
    } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(m_selectedNode)) {
        node3d->SetPosition(newPosition);
    } else if (auto* control = dynamic_cast<Lupine::Control*>(m_selectedNode)) {
        control->SetPosition(glm::vec2(newPosition.x, newPosition.y));
    }
}

void SceneViewPanel::updateRotateGizmoDrag(const glm::vec3& rayOrigin, const glm::vec3& rayDirection)
{
    glm::vec3 nodePosition = m_gizmoRenderer->GetNodeWorldPosition(m_selectedNode);

    // Calculate rotation based on mouse movement around the selected axis
    glm::vec3 rotationAxis;
    if (m_draggedGizmoAxis == GizmoAxis::X) rotationAxis = glm::vec3(1, 0, 0);
    else if (m_draggedGizmoAxis == GizmoAxis::Y) rotationAxis = glm::vec3(0, 1, 0);
    else if (m_draggedGizmoAxis == GizmoAxis::Z) rotationAxis = glm::vec3(0, 0, 1);
    else return;

    // Project ray onto the rotation plane
    float denom = glm::dot(rotationAxis, rayDirection);
    if (glm::abs(denom) > 1e-6f) {
        float t = glm::dot(nodePosition - rayOrigin, rotationAxis) / denom;
        glm::vec3 hitPoint = rayOrigin + t * rayDirection;

        // Calculate angle from center to hit point
        glm::vec3 toHit = glm::normalize(hitPoint - nodePosition);
        glm::vec3 startDir = glm::normalize(m_dragStartMouseWorld - nodePosition);

        // Calculate rotation angle
        float angle = glm::acos(glm::clamp(glm::dot(startDir, toHit), -1.0f, 1.0f));
        glm::vec3 cross = glm::cross(startDir, toHit);
        if (glm::dot(cross, rotationAxis) < 0) angle = -angle;

        // Apply rotation
        glm::quat deltaRotation = glm::angleAxis(angle, rotationAxis);
        glm::quat newRotation = deltaRotation * m_dragStartRotation;

        if (auto* node2d = dynamic_cast<Lupine::Node2D*>(m_selectedNode)) {
            // For 2D nodes, only Z rotation matters
            if (m_draggedGizmoAxis == GizmoAxis::Z) {
                float eulerZ = glm::eulerAngles(newRotation).z;
                node2d->SetRotation(eulerZ);
            }
        } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(m_selectedNode)) {
            node3d->SetRotation(newRotation);
        }
    }
}

void SceneViewPanel::updateScaleGizmoDrag(const glm::vec3& rayOrigin, const glm::vec3& rayDirection)
{
    glm::vec3 nodePosition = m_gizmoRenderer->GetNodeWorldPosition(m_selectedNode);

    // Calculate scale factor based on distance from center
    glm::vec3 currentMouseWorld = rayOrigin;
    float startDistance = glm::length(m_dragStartMouseWorld - nodePosition);
    float currentDistance = glm::length(currentMouseWorld - nodePosition);

    if (startDistance > 1e-6f) {
        float scaleFactor = currentDistance / startDistance;
        glm::vec3 newScale = m_dragStartScale;

        if (m_draggedGizmoAxis == GizmoAxis::X) {
            newScale.x *= scaleFactor;
        } else if (m_draggedGizmoAxis == GizmoAxis::Y) {
            newScale.y *= scaleFactor;
        } else if (m_draggedGizmoAxis == GizmoAxis::Z) {
            newScale.z *= scaleFactor;
        } else if (m_draggedGizmoAxis == GizmoAxis::XYZ) {
            newScale *= scaleFactor; // Uniform scaling
        }

        // Clamp scale to reasonable values
        newScale = glm::clamp(newScale, glm::vec3(0.01f), glm::vec3(100.0f));

        // Apply scale to node
        if (auto* node2d = dynamic_cast<Lupine::Node2D*>(m_selectedNode)) {
            node2d->SetScale(glm::vec2(newScale.x, newScale.y));
        } else if (auto* node3d = dynamic_cast<Lupine::Node3D*>(m_selectedNode)) {
            node3d->SetScale(newScale);
        } else if (auto* control = dynamic_cast<Lupine::Control*>(m_selectedNode)) {
            control->SetSize(glm::vec2(newScale.x, newScale.y));
        }
    }
}

void SceneViewPanel::updateGizmoHighlight(const QPoint& mousePos)
{
    if (!m_selectedNode || m_activeGizmo == GizmoType::None) {
        m_gizmoRenderer->SetHighlightedAxis(GizmoAxis::None);
        return;
    }

    GizmoHitResult hit = testGizmoHit(mousePos);
    m_gizmoRenderer->SetHighlightedAxis(hit.hit ? hit.axis : GizmoAxis::None);

    if (m_hoveredGizmoAxis != hit.axis) {
        m_hoveredGizmoAxis = hit.axis;
        update(); // Trigger repaint to show highlight change
    }
}

glm::vec3 SceneViewPanel::screenToWorldPosition(const QPoint& screenPos) {
    glm::vec3 rayOrigin, rayDirection;
    screenToWorldRay(screenPos, rayOrigin, rayDirection);

    if (m_viewMode == ViewMode::Mode2D) {
        // For 2D mode, use the ray origin directly (already in world space)
        return rayOrigin;
    } else {
        // For 3D mode, try surface raycasting first, then fall back to ground plane
        return screenToWorldPositionWithSurfaceDetection(rayOrigin, rayDirection);
    }
}

glm::vec3 SceneViewPanel::screenToWorldPositionWithSurfaceDetection(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) {
    // Try raycasting to find surface intersection
    Lupine::Node3D* hitNode = nullptr;
    glm::vec3 hitPoint, hitNormal;

    // Cast ray far enough to hit most surfaces
    glm::vec3 rayEnd = rayOrigin + rayDirection * 1000.0f;

    // Use physics raycasting to find surface
    if (Lupine::PhysicsManager::Raycast3D(rayOrigin, rayEnd, hitNode, hitPoint, hitNormal)) {
        return hitPoint;
    }

    // Fallback: Try intersection with different planes based on camera orientation
    glm::vec3 cameraForward = glm::normalize(m_editorCamera->GetPosition() - m_cameraTarget);

    // Determine the best plane to intersect with based on camera angle
    float groundPlaneY = 0.0f; // Default ground level

    // If looking mostly down, use XZ plane (ground)
    if (std::abs(rayDirection.y) > 0.3f) {
        if (std::abs(rayDirection.y) > 0.0001f) {
            float t = (groundPlaneY - rayOrigin.y) / rayDirection.y;
            if (t > 0) {
                return rayOrigin + rayDirection * t;
            }
        }
    }

    // If looking mostly horizontally, use a vertical plane
    if (std::abs(rayDirection.z) > std::abs(rayDirection.x) && std::abs(rayDirection.z) > 0.0001f) {
        // Use XY plane at camera target Z
        float t = (m_cameraTarget.z - rayOrigin.z) / rayDirection.z;
        if (t > 0) {
            return rayOrigin + rayDirection * t;
        }
    } else if (std::abs(rayDirection.x) > 0.0001f) {
        // Use YZ plane at camera target X
        float t = (m_cameraTarget.x - rayOrigin.x) / rayDirection.x;
        if (t > 0) {
            return rayOrigin + rayDirection * t;
        }
    }

    // Final fallback: project to a plane at a reasonable distance from camera
    float distance = glm::length(m_cameraTarget - m_editorCamera->GetPosition());
    return rayOrigin + rayDirection * distance;
}

void SceneViewPanel::dragEnterEvent(QDragEnterEvent* event)
{
    // Reset drag state
    m_isDragging = false;
    m_currentDragFilePath.clear();

    if (event->mimeData()->hasUrls()) {
        // Check if any of the URLs are supported file types
        QList<QUrl> urls = event->mimeData()->urls();

        for (const QUrl& url : urls) {
            if (url.isLocalFile()) {
                QString filePath = url.toLocalFile();

                // Simple file type validation - assume valid if file type is supported
                if (m_assetDropHandler->isImageFile(filePath) ||
                    m_assetDropHandler->is3DModelFile(filePath)) {

                    // Cache the file path for consistent validation
                    m_currentDragFilePath = filePath;

                    // Accept the drag operation
                    event->setDropAction(Qt::CopyAction);
                    event->accept();
                    m_isDragging = true;

                    // Start placement mode if enabled
                    if (m_placementMode && m_placementMode->isEnabled()) {
                        m_placementMode->startPlacement(filePath, event->pos(), this);
                    }
                    return;
                }
            }
        }
    }

    event->ignore();
}

void SceneViewPanel::dragMoveEvent(QDragMoveEvent* event)
{
    // Simple validation - if we have a cached file path, it's valid
    if (!m_currentDragFilePath.isEmpty()) {
        // Update placement mode if active
        if (m_placementMode && m_placementMode->isPlacing()) {
            bool ctrlPressed = (event->keyboardModifiers() & Qt::ControlModifier) != 0;
            m_placementMode->updatePlacement(event->pos(), ctrlPressed);
            update(); // Trigger repaint for ghost rendering
        }

        m_isDragging = true;
        event->setDropAction(Qt::CopyAction);
        event->accept();
    } else {
        event->ignore();
    }
}

void SceneViewPanel::dragLeaveEvent(QDragLeaveEvent* event)
{
    if (m_isDragging) {
        // Cancel placement mode if active
        if (m_placementMode && m_placementMode->isPlacing()) {
            m_placementMode->cancelPlacement();
        }

        m_isDragging = false;
    }

    // Clear cached drag data
    m_currentDragFilePath.clear();

    event->accept();
}

void SceneViewPanel::dropEvent(QDropEvent* event)
{
    if (m_isDragging && event->mimeData()->hasUrls()) {
        // End placement mode if active
        if (m_placementMode && m_placementMode->isPlacing()) {
            m_placementMode->endPlacement();
        }

        m_isDragging = false;

        QList<QUrl> urls = event->mimeData()->urls();
        QStringList filePaths;

        for (const QUrl& url : urls) {
            if (url.isLocalFile()) {
                filePaths.append(url.toLocalFile());
            }
        }

        if (!filePaths.isEmpty() && m_scene) {
            // Check if Alt key is held for options popup
            bool showOptions = (event->keyboardModifiers() & Qt::AltModifier) != 0;

            // Handle the file drop
            bool success = m_assetDropHandler->handleFileDrop(filePaths, event->pos(), showOptions, m_scene);

            if (success) {
                event->acceptProposedAction();
                update(); // Refresh the scene view
            } else {
                event->ignore();
            }
        } else {
            event->ignore();
        }
    } else {
        event->ignore();
    }
}

bool SceneViewPanel::isNodeValid(Lupine::Node* node) const
{
    if (!node || !m_scene) return false;

    try {
        // Check if the node exists in the scene hierarchy using safer recursive search
        if (m_scene->GetRootNode()) {
            // For root node, check directly
            if (node == m_scene->GetRootNode()) {
                return true;
            }

            // For other nodes, use recursive search without creating vectors
            return isNodeInHierarchy(m_scene->GetRootNode(), node);
        }
    } catch (...) {
        // If any exception occurs during validation, consider the node invalid
        return false;
    }

    return false;
}

bool SceneViewPanel::isNodeInHierarchy(Lupine::Node* root, Lupine::Node* target) const
{
    if (!root || !target) return false;

    try {
        // Check if this is the target node
        if (root == target) {
            return true;
        }

        // Check children recursively
        for (const auto& child : root->GetChildren()) {
            if (child.get() == target || isNodeInHierarchy(child.get(), target)) {
                return true;
            }
        }
    } catch (...) {
        // If any exception occurs during traversal, return false
        return false;
    }

    return false;
}
