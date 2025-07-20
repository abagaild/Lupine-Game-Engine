// Include glad before any Qt OpenGL headers to avoid conflicts
#include <glad/glad.h>

#include "TilemapBuilder3DDialog.h"
#include "lupine/resources/Tileset3DResource.h"
#include "lupine/resources/MeshLoader.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHeaderView>
#include <QtGui/QKeySequence>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QKeyEvent>
#include <QtOpenGL/QOpenGLShaderProgram>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QTabWidget>
#include <QtCore/QStandardPaths>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// TilemapCanvas3D Implementation
TilemapCanvas3D::TilemapCanvas3D(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_cameraPosition(5.0f, 5.0f, 5.0f)
    , m_cameraTarget(0.0f, 0.0f, 0.0f)
    , m_cameraUp(0.0f, 1.0f, 0.0f)
    , m_cameraDistance(10.0f)
    , m_cameraYaw(45.0f)
    , m_cameraPitch(30.0f)
    , m_mousePressed(false)
    , m_pressedButton(Qt::NoButton)
    , m_selectedTile(nullptr)
    , m_currentTool(TileTool::Place)
    , m_placementMode(TilePlacementMode::GridSnap)
    , m_gridSize(1.0f)
    , m_gridBaseY(0.0f)
    , m_showGrid(true)
    , m_selectedTileId(-1)
    , m_showPreview(false)
    , m_previewPosition(0.0f)
    , m_shaderProgram(0)
    , m_gridShaderProgram(0)
    , m_gridVAO(0)
    , m_gridVBO(0)
    , m_cubeVAO(0)
    , m_cubeVBO(0)
    , m_cubeEBO(0)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    updateCamera();
}

TilemapCanvas3D::~TilemapCanvas3D() {
    // Cleanup will be handled automatically
}

void TilemapCanvas3D::AddTile(int tile_id, const glm::vec3& position) {
    // Check if tile already exists at this position
    for (const auto& tile : m_placedTiles) {
        if (glm::distance(tile.position, position) < 0.01f) {
            return; // Tile already exists at this position
        }
    }

    PlacedTile3D tile(tile_id, position);
    m_placedTiles.push_back(tile);

    update();
    emit tileAdded(tile_id, position);
    emit sceneModified();
}

void TilemapCanvas3D::RemoveTile(const glm::vec3& position) {
    for (auto it = m_placedTiles.begin(); it != m_placedTiles.end(); ++it) {
        if (glm::distance(it->position, position) < 0.01f) {
            if (m_selectedTile == &(*it)) {
                m_selectedTile = nullptr;
            }
            m_placedTiles.erase(it);
            update();
            emit tileRemoved(position);
            emit sceneModified();
            break;
        }
    }
}

void TilemapCanvas3D::ClearTiles() {
    m_placedTiles.clear();
    m_selectedTile = nullptr;
    update();
    emit sceneModified();
}

void TilemapCanvas3D::SelectTile(const glm::vec3& position) {
    // Clear previous selection
    if (m_selectedTile) {
        m_selectedTile->selected = false;
    }
    
    // Find and select tile at position
    m_selectedTile = getTileAt(position);
    if (m_selectedTile) {
        m_selectedTile->selected = true;
        emit tileSelected(m_selectedTile);
    }
    
    update();
}

void TilemapCanvas3D::ClearSelection() {
    if (m_selectedTile) {
        m_selectedTile->selected = false;
        m_selectedTile = nullptr;
    }
    update();
}

PlacedTile3D* TilemapCanvas3D::GetSelectedTile() {
    return m_selectedTile;
}

void TilemapCanvas3D::SetTileset(std::shared_ptr<Lupine::Tileset3DResource> tileset) {
    m_tileset = tileset;
    update();
}

void TilemapCanvas3D::ResetCamera() {
    m_cameraDistance = 10.0f;
    m_cameraYaw = 45.0f;
    m_cameraPitch = 30.0f;
    m_cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    updateCamera();
    update();
}

void TilemapCanvas3D::FocusOnTiles() {
    if (m_placedTiles.empty()) {
        ResetCamera();
        return;
    }
    
    // Calculate bounding box of all tiles
    glm::vec3 minPos = m_placedTiles[0].position;
    glm::vec3 maxPos = m_placedTiles[0].position;
    
    for (const auto& tile : m_placedTiles) {
        minPos = glm::min(minPos, tile.position);
        maxPos = glm::max(maxPos, tile.position);
    }
    
    // Set camera target to center of bounding box
    m_cameraTarget = (minPos + maxPos) * 0.5f;
    
    // Adjust distance based on bounding box size
    glm::vec3 size = maxPos - minPos;
    float maxSize = glm::max(glm::max(size.x, size.y), size.z);
    m_cameraDistance = maxSize * 2.0f + 5.0f;
    
    updateCamera();
    update();
}

void TilemapCanvas3D::initializeGL() {
    initializeOpenGLFunctions();
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    
    setupShaders();
    setupBuffers();
}

void TilemapCanvas3D::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    m_projMatrix = glm::perspective(glm::radians(45.0f), (float)w / (float)h, 0.1f, 1000.0f);
}

void TilemapCanvas3D::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (m_showGrid) {
        drawGrid();
    }
    
    drawTiles();

    if (m_showPreview && m_selectedTileId >= 0) {
        drawPreviewTile();
    }

    if (m_selectedTile) {
        drawGizmos();
    }
}

void TilemapCanvas3D::mousePressEvent(QMouseEvent* event) {
    m_mousePressed = true;
    m_lastMousePos = event->pos();
    m_pressedButton = event->button();

    if (event->button() == Qt::LeftButton) {
        glm::vec3 worldPos = screenToWorld(event->pos());
        glm::vec3 snapPos = getSnapPosition(worldPos);

        switch (m_currentTool) {
            case TileTool::Place:
                {
                    PlacedTile3D* clickedTile = getTileAt(snapPos);
                    if (clickedTile) {
                        SelectTile(clickedTile->position);
                    } else {
                        // Place new tile if we have a tileset and selected tile type
                        ClearSelection();
                        if (m_selectedTileId >= 0 && m_tileset) {
                            AddTile(m_selectedTileId, snapPos);
                        }
                    }
                }
                break;

            case TileTool::Erase:
                RemoveTile(snapPos);
                break;

            case TileTool::Select:
                {
                    PlacedTile3D* clickedTile = getTileAt(snapPos);
                    if (clickedTile) {
                        SelectTile(clickedTile->position);
                    } else {
                        ClearSelection();
                    }
                }
                break;
        }
    } else if (event->button() == Qt::RightButton) {
        // Right-click always removes tiles regardless of tool
        glm::vec3 removeWorldPos = screenToWorld(event->pos());
        glm::vec3 removeSnapPos = getSnapPosition(removeWorldPos);
        RemoveTile(removeSnapPos);
    }
}

void TilemapCanvas3D::mouseMoveEvent(QMouseEvent* event) {
    if (m_mousePressed && m_pressedButton == Qt::MiddleButton) {
        // Camera rotation
        QPoint delta = event->pos() - m_lastMousePos;
        m_cameraYaw += delta.x() * 0.5f;
        m_cameraPitch -= delta.y() * 0.5f;

        // Clamp pitch
        m_cameraPitch = glm::clamp(m_cameraPitch, -89.0f, 89.0f);

        updateCamera();
        update();
    } else if (!m_mousePressed) {
        // Update preview when not dragging
        UpdatePreview(event->pos());
    }

    m_lastMousePos = event->pos();
}

void TilemapCanvas3D::mouseReleaseEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    m_mousePressed = false;
    m_pressedButton = Qt::NoButton;
}

void TilemapCanvas3D::wheelEvent(QWheelEvent* event) {
    // Camera zoom
    float delta = event->angleDelta().y() / 120.0f;
    m_cameraDistance -= delta * 0.5f;
    m_cameraDistance = glm::clamp(m_cameraDistance, 1.0f, 100.0f);
    
    updateCamera();
    update();
}

void TilemapCanvas3D::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_1:
            SetPlacementMode(TilePlacementMode::GridSnap);
            break;
        case Qt::Key_2:
            SetPlacementMode(TilePlacementMode::FaceSnap);
            break;
        case Qt::Key_3:
            SetPlacementMode(TilePlacementMode::FreePlace);
            break;
        case Qt::Key_R:
            ResetCamera();
            break;
        case Qt::Key_F:
            FocusOnTiles();
            break;
        case Qt::Key_Delete:
            if (m_selectedTile) {
                RemoveTile(m_selectedTile->position);
            }
            break;
        case Qt::Key_Up:
            if (event->modifiers() & Qt::ShiftModifier) {
                // Shift+Up: Raise grid base Y
                m_gridBaseY += m_gridSize;
                emit gridBaseYChanged(m_gridBaseY);
                update();
            }
            break;
        case Qt::Key_Down:
            if (event->modifiers() & Qt::ShiftModifier) {
                // Shift+Down: Lower grid base Y
                m_gridBaseY -= m_gridSize;
                emit gridBaseYChanged(m_gridBaseY);
                update();
            }
            break;
        default:
            QOpenGLWidget::keyPressEvent(event);
            break;
    }
}

void TilemapCanvas3D::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);
    setMouseTracking(true);
    m_showPreview = true;
}

void TilemapCanvas3D::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    setMouseTracking(false);
    m_showPreview = false;
    update();
}

void TilemapCanvas3D::UpdatePreview(const QPoint& mousePos) {
    if (m_currentTool == TileTool::Place && m_selectedTileId >= 0) {
        glm::vec3 worldPos = screenToWorld(mousePos);
        m_previewPosition = getSnapPosition(worldPos);
        m_showPreview = true;
        update();
    } else {
        m_showPreview = false;
    }
}

void TilemapCanvas3D::updateCamera() {
    // Convert spherical coordinates to cartesian
    float yawRad = glm::radians(m_cameraYaw);
    float pitchRad = glm::radians(m_cameraPitch);

    glm::vec3 offset;
    offset.x = cos(pitchRad) * cos(yawRad);
    offset.y = sin(pitchRad);
    offset.z = cos(pitchRad) * sin(yawRad);

    m_cameraPosition = m_cameraTarget + offset * m_cameraDistance;
    m_viewMatrix = glm::lookAt(m_cameraPosition, m_cameraTarget, m_cameraUp);
}

glm::vec3 TilemapCanvas3D::screenToWorld(const QPoint& screenPos) {
    // This is a simplified implementation
    // In a real implementation, you'd use proper ray casting
    
    // Get normalized device coordinates
    float x = (2.0f * screenPos.x()) / width() - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / height();
    
    // Create ray direction
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 rayEye = glm::inverse(m_projMatrix) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::vec3(glm::inverse(m_viewMatrix) * rayEye);
    rayWorld = glm::normalize(rayWorld);
    
    // Cast ray from camera position in the direction of the mouse
    // For tile placement, we'll intersect with the Y=0 plane (ground)
    glm::vec3 rayOrigin = m_cameraPosition;
    
    // Calculate intersection with Y=0 plane
    if (abs(rayWorld.y) > 0.001f) {
        float t = -rayOrigin.y / rayWorld.y;
        if (t > 0) {
            return rayOrigin + t * rayWorld;
        }
    }
    
    // Fallback: project to a plane at distance from camera
    float distance = 5.0f;
    return rayOrigin + distance * rayWorld;
}

glm::vec3 TilemapCanvas3D::snapToGrid(const glm::vec3& position) {
    float x = round(position.x / m_gridSize) * m_gridSize;
    float y = round(position.y / m_gridSize) * m_gridSize;
    float z = round(position.z / m_gridSize) * m_gridSize;
    return glm::vec3(x, y, z);
}

glm::vec3 TilemapCanvas3D::snapToFace(const glm::vec3& position) {
    // Find nearest tile and snap to its face
    PlacedTile3D* nearestTile = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (auto& tile : m_placedTiles) {
        float distance = glm::distance(position, tile.position);
        if (distance < minDistance) {
            minDistance = distance;
            nearestTile = &tile;
        }
    }

    if (!nearestTile) {
        return snapToGrid(position);
    }

    // Determine which face to snap to based on relative position
    glm::vec3 relative = position - nearestTile->position;
    glm::vec3 snapPos = nearestTile->position;
    
    // Find the axis with the largest absolute component
    if (abs(relative.x) > abs(relative.y) && abs(relative.x) > abs(relative.z)) {
        snapPos.x += (relative.x > 0) ? m_gridSize : -m_gridSize;
    } else if (abs(relative.y) > abs(relative.z)) {
        snapPos.y += (relative.y > 0) ? m_gridSize : -m_gridSize;
    } else {
        snapPos.z += (relative.z > 0) ? m_gridSize : -m_gridSize;
    }
    
    return snapPos;
}

PlacedTile3D* TilemapCanvas3D::getTileAt(const glm::vec3& position) {
    for (auto& tile : m_placedTiles) {
        if (glm::distance(tile.position, position) < m_gridSize * 0.5f) {
            return &tile;
        }
    }
    return nullptr;
}

glm::vec3 TilemapCanvas3D::getSnapPosition(const glm::vec3& worldPos) {
    switch (m_placementMode) {
        case TilePlacementMode::GridSnap:
            return snapToGrid(worldPos);
        case TilePlacementMode::FaceSnap:
            return snapToFace(worldPos);
        case TilePlacementMode::FreePlace:
        default:
            return worldPos;
    }
}

void TilemapCanvas3D::setupShaders() {
    // Vertex shader for tile rendering
    std::string vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 normal;
        out vec3 fragPos;
        out vec2 texCoord;

        void main() {
            normal = mat3(transpose(inverse(model))) * aNormal;
            fragPos = vec3(model * vec4(aPos, 1.0));
            texCoord = aTexCoord;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    // Fragment shader with basic lighting
    std::string fragmentShaderSource = R"(
        #version 330 core
        in vec3 normal;
        in vec3 fragPos;
        in vec2 texCoord;

        out vec4 FragColor;

        uniform vec3 objectColor;
        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 viewPos;
        uniform sampler2D texture1;
        uniform bool useTexture;

        void main() {
            // Ambient lighting
            float ambientStrength = 0.4;
            vec3 ambient = ambientStrength * lightColor;

            // Diffuse lighting
            vec3 norm = normalize(normal);
            vec3 lightDir = normalize(lightPos - fragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            // Specular lighting
            float specularStrength = 0.3;
            vec3 viewDir = normalize(viewPos - fragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
            vec3 specular = specularStrength * spec * lightColor;

            vec3 result = (ambient + diffuse + specular) * objectColor;

            if (useTexture) {
                vec3 texColor = texture(texture1, texCoord).rgb;
                result *= texColor;
            }

            FragColor = vec4(result, 1.0);
        }
    )";

    // Grid vertex shader
    std::string gridVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";

    // Grid fragment shader
    std::string gridFragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        uniform vec3 gridColor;

        void main() {
            FragColor = vec4(gridColor, 1.0);
        }
    )";

    // Compile main shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vShaderCode = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &vShaderCode, NULL);
    glCompileShader(vertexShader);

    // Check for vertex shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        qCritical() << "Vertex shader compilation failed:" << infoLog;
        return;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fShaderCode = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        qCritical() << "Fragment shader compilation failed:" << infoLog;
        glDeleteShader(vertexShader);
        return;
    }

    // Create main shader program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        qCritical() << "Shader program linking failed:" << infoLog;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }

    // Compile grid shaders
    unsigned int gridVertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* gridVShaderCode = gridVertexShaderSource.c_str();
    glShaderSource(gridVertexShader, 1, &gridVShaderCode, NULL);
    glCompileShader(gridVertexShader);

    glGetShaderiv(gridVertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(gridVertexShader, 512, NULL, infoLog);
        qCritical() << "Grid vertex shader compilation failed:" << infoLog;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }

    unsigned int gridFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* gridFShaderCode = gridFragmentShaderSource.c_str();
    glShaderSource(gridFragmentShader, 1, &gridFShaderCode, NULL);
    glCompileShader(gridFragmentShader);

    glGetShaderiv(gridFragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(gridFragmentShader, 512, NULL, infoLog);
        qCritical() << "Grid fragment shader compilation failed:" << infoLog;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteShader(gridVertexShader);
        return;
    }

    // Create grid shader program
    m_gridShaderProgram = glCreateProgram();
    glAttachShader(m_gridShaderProgram, gridVertexShader);
    glAttachShader(m_gridShaderProgram, gridFragmentShader);
    glLinkProgram(m_gridShaderProgram);

    glGetProgramiv(m_gridShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_gridShaderProgram, 512, NULL, infoLog);
        qCritical() << "Grid shader program linking failed:" << infoLog;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(gridVertexShader);
    glDeleteShader(gridFragmentShader);

    qDebug() << "Shaders compiled and linked successfully";
}

void TilemapCanvas3D::setupBuffers() {
    try {
        // Generate VAO, VBO, and EBO for cube rendering
        glGenVertexArrays(1, &m_cubeVAO);
        glGenBuffers(1, &m_cubeVBO);
        glGenBuffers(1, &m_cubeEBO);

        // Check for OpenGL errors after buffer generation
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            qCritical() << "OpenGL error during buffer generation in TilemapCanvas3D:" << error;
            return;
        }

        // Validate buffer IDs
        if (m_cubeVAO == 0 || m_cubeVBO == 0 || m_cubeEBO == 0) {
            qCritical() << "Failed to generate OpenGL buffers in TilemapCanvas3D";
            return;
        }

        // Setup cube geometry
        float vertices[] = {
            // positions          // normals           // texture coords
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
        };

        glBindVertexArray(m_cubeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // Texture coordinate attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        // Generate grid VAO and VBO
        glGenVertexArrays(1, &m_gridVAO);
        glGenBuffers(1, &m_gridVBO);

        // Setup grid geometry
        std::vector<float> gridVertices;
        float gridExtent = 10.0f;

        // Generate grid lines
        for (float i = -gridExtent; i <= gridExtent; i += m_gridSize) {
            // X lines
            gridVertices.push_back(i); gridVertices.push_back(m_gridBaseY); gridVertices.push_back(-gridExtent);
            gridVertices.push_back(i); gridVertices.push_back(m_gridBaseY); gridVertices.push_back(gridExtent);

            // Z lines
            gridVertices.push_back(-gridExtent); gridVertices.push_back(m_gridBaseY); gridVertices.push_back(i);
            gridVertices.push_back(gridExtent); gridVertices.push_back(m_gridBaseY); gridVertices.push_back(i);
        }

        glBindVertexArray(m_gridVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);

        // Position attribute for grid
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);

        qDebug() << "TilemapCanvas3D buffers setup successfully";
    } catch (const std::exception& e) {
        qCritical() << "Exception in TilemapCanvas3D::setupBuffers:" << e.what();
    }
}

void TilemapCanvas3D::drawGrid() {
    if (!m_showGrid || m_gridShaderProgram == 0 || m_gridVAO == 0) return;

    // Use grid shader program
    glUseProgram(m_gridShaderProgram);

    // Set uniforms
    GLint viewLoc = glGetUniformLocation(m_gridShaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_gridShaderProgram, "projection");
    GLint colorLoc = glGetUniformLocation(m_gridShaderProgram, "gridColor");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projMatrix));
    glUniform3f(colorLoc, 0.5f, 0.5f, 0.5f); // Light gray

    // Disable depth testing for grid to ensure it's always visible
    glDisable(GL_DEPTH_TEST);

    // Draw grid
    glBindVertexArray(m_gridVAO);

    // Calculate number of lines (2 vertices per line, 2 lines per grid step)
    float gridExtent = 10.0f;
    int numGridLines = (int)((2.0f * gridExtent) / m_gridSize) + 1;
    int numVertices = numGridLines * 4; // 2 directions * 2 vertices per line

    glDrawArrays(GL_LINES, 0, numVertices);
    glBindVertexArray(0);

    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);

    glUseProgram(0);
}

void TilemapCanvas3D::drawTiles() {
    if (m_placedTiles.empty() || !m_tileset) return;

    // Use shader program for rendering
    if (m_shaderProgram == 0) return;

    glUseProgram(m_shaderProgram);

    // Set common uniforms
    GLint viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    GLint modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    GLint lightPosLoc = glGetUniformLocation(m_shaderProgram, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    GLint viewPosLoc = glGetUniformLocation(m_shaderProgram, "viewPos");
    GLint useTextureLoc = glGetUniformLocation(m_shaderProgram, "useTexture");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projMatrix));

    // Set lighting uniforms
    glm::vec3 lightPos = m_cameraPosition + glm::vec3(2.0f, 2.0f, 2.0f);
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // White light
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(m_cameraPosition));
    glUniform1i(useTextureLoc, 0); // No texture for now

    // Draw all placed tiles
    for (const auto& tile : m_placedTiles) {
        // Get tile data from tileset
        const auto& tiles = m_tileset->GetTiles();
        auto tileIt = tiles.find(tile.tile_id);
        if (tileIt == tiles.end() || !tileIt->second.model_loaded || !tileIt->second.model) {
            continue;
        }

        const auto& tileData = tileIt->second;

        // Calculate transform matrix
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, tile.position);

        // Convert Euler angles (degrees) to quaternion
        glm::vec3 rotationRad = glm::radians(tile.rotation);
        glm::quat rotationQuat = glm::quat(rotationRad);
        transform = transform * glm::mat4_cast(rotationQuat);

        transform = glm::scale(transform, tile.scale);

        // Apply tile's default transform
        glm::mat4 tileTransform = glm::mat4(1.0f);
        tileTransform = glm::translate(tileTransform, tileData.default_transform.position);
        tileTransform = tileTransform * glm::mat4_cast(tileData.default_transform.rotation);
        tileTransform = glm::scale(tileTransform, tileData.default_transform.scale);

        glm::mat4 finalTransform = transform * tileTransform;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(finalTransform));

        // Set color based on selection
        GLint colorLoc = glGetUniformLocation(m_shaderProgram, "objectColor");
        if (tile.selected) {
            glUniform3f(colorLoc, 1.0f, 0.5f, 0.0f); // Orange for selected
        } else {
            glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // White for normal
        }

        // Render the model (simplified - would need proper mesh rendering)
        // This is a placeholder for actual model rendering
        if (tileData.model && tileData.model->IsLoaded()) {
            // TODO: Implement actual model rendering using the mesh data
            // For now, draw a simple cube as placeholder
            drawCube(1.0f);
        }
    }

    glUseProgram(0);
}

void TilemapCanvas3D::drawCube(float size) {
    if (m_cubeVAO == 0) return;

    // Scale the cube by the size parameter
    // The model matrix should already be set by the caller

    // Bind and draw the cube VAO
    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void TilemapCanvas3D::drawGizmos() {
    // Draw transformation gizmos for selected tile
    if (m_selectedTile) {
        // Draw move/rotate/scale gizmos at m_selectedTile->position
    }
}

void TilemapCanvas3D::drawPreviewTile() {
    if (!m_showPreview || m_selectedTileId < 0 || !m_tileset) {
        return;
    }

    // Get the tile data
    const auto& tiles = m_tileset->GetTiles();
    auto tileIt = tiles.find(m_selectedTileId);
    if (tileIt == tiles.end()) {
        return;
    }

    const auto& tileData = tileIt->second;
    if (!tileData.model_loaded || !tileData.model) {
        return;
    }

    // Enable blending for semi-transparent preview
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Calculate transform matrix for preview position
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, m_previewPosition);

    // Apply tile's default transform
    glm::mat4 tileTransform = glm::mat4(1.0f);
    tileTransform = glm::translate(tileTransform, tileData.default_transform.position);
    tileTransform = tileTransform * glm::mat4_cast(tileData.default_transform.rotation);
    tileTransform = glm::scale(tileTransform, tileData.default_transform.scale);

    glm::mat4 finalTransform = transform * tileTransform;

    // Render with semi-transparent color
    glm::vec4 previewColor(1.0f, 1.0f, 1.0f, 0.5f);

    // Render the 3D model (simplified - would use proper renderer)
    // for (const auto& mesh : tileData.model->GetMeshes()) {
    //     Lupine::Renderer::RenderMesh(mesh, finalTransform, previewColor, 0, true);
    // }

    // Disable blending
    glDisable(GL_BLEND);
}

bool TilemapCanvas3D::ExportToOBJ(const QString& filepath, bool optimizeMesh) {
    Q_UNUSED(optimizeMesh); // TODO: Implement mesh optimization

    if (!m_tileset || m_placedTiles.empty()) {
        return false;
    }

    try {
        std::ofstream file(filepath.toStdString());
        if (!file.is_open()) {
            return false;
        }

        file << "# 3D Tilemap Export\n";
        file << "# Generated by Lupine Engine\n\n";

        int vertex_offset = 1; // OBJ indices start at 1

        for (const auto& placed_tile : m_placedTiles) {
            const auto* tile_data = m_tileset->GetTile(placed_tile.tile_id);
            if (!tile_data || !tile_data->model || !tile_data->model_loaded) {
                continue;
            }

            file << "# Tile " << placed_tile.tile_id << " at position "
                 << placed_tile.position.x << " " << placed_tile.position.y << " " << placed_tile.position.z << "\n";

            // Transform matrix for this tile
            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, placed_tile.position);
            transform = glm::rotate(transform, glm::radians(placed_tile.rotation.x), glm::vec3(1, 0, 0));
            transform = glm::rotate(transform, glm::radians(placed_tile.rotation.y), glm::vec3(0, 1, 0));
            transform = glm::rotate(transform, glm::radians(placed_tile.rotation.z), glm::vec3(0, 0, 1));
            transform = glm::scale(transform, placed_tile.scale);

            // Export each mesh in the model
            for (const auto& mesh : tile_data->model->GetMeshes()) {
                // Export vertices
                for (const auto& vertex : mesh.vertices) {
                    glm::vec4 pos = transform * glm::vec4(vertex.position, 1.0f);
                    file << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
                }

                // Export texture coordinates
                for (const auto& vertex : mesh.vertices) {
                    file << "vt " << vertex.tex_coords.x << " " << vertex.tex_coords.y << "\n";
                }

                // Export normals
                for (const auto& vertex : mesh.vertices) {
                    glm::vec3 normal = glm::mat3(transform) * vertex.normal;
                    normal = glm::normalize(normal);
                    file << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
                }

                // Export faces
                for (size_t i = 0; i < mesh.indices.size(); i += 3) {
                    int v1 = vertex_offset + mesh.indices[i];
                    int v2 = vertex_offset + mesh.indices[i + 1];
                    int v3 = vertex_offset + mesh.indices[i + 2];
                    file << "f " << v1 << "/" << v1 << "/" << v1 << " "
                         << v2 << "/" << v2 << "/" << v2 << " "
                         << v3 << "/" << v3 << "/" << v3 << "\n";
                }

                vertex_offset += static_cast<int>(mesh.vertices.size());
            }

            file << "\n";
        }

        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error exporting to OBJ: " << e.what() << std::endl;
        return false;
    }
}

bool TilemapCanvas3D::ExportToFBX(const QString& filepath, bool optimizeMesh) {
    // For now, export as OBJ since FBX export requires additional libraries
    // In a full implementation, you would use FBX SDK or similar
    QString objPath = filepath;
    objPath.replace(".fbx", ".obj");

    std::cout << "FBX export not fully implemented, exporting as OBJ instead: " << objPath.toStdString() << std::endl;
    return ExportToOBJ(objPath, optimizeMesh);
}

bool TilemapCanvas3D::SaveTilemap(const QString& filepath) {
    try {
        json j;
        j["type"] = "3DTilemap";
        j["version"] = "1.0";
        j["grid_size"] = m_gridSize;
        j["tileset_path"] = m_tileset ? "tileset.tileset3d" : "";

        json tiles_array = json::array();
        for (const auto& tile : m_placedTiles) {
            json tile_json;
            tile_json["id"] = tile.tile_id;
            tile_json["position"] = {tile.position.x, tile.position.y, tile.position.z};
            tile_json["rotation"] = {tile.rotation.x, tile.rotation.y, tile.rotation.z};
            tile_json["scale"] = {tile.scale.x, tile.scale.y, tile.scale.z};

            // Save custom tile data if available
            if (m_tileset) {
                const auto& tiles = m_tileset->GetTiles();
                auto tileIt = tiles.find(tile.tile_id);
                if (tileIt != tiles.end()) {
                    const auto& tileData = tileIt->second;

                    // Save collision data
                    json collision_json;
                    collision_json["type"] = static_cast<int>(tileData.collision.type);
                    collision_json["size"] = {tileData.collision.size.x, tileData.collision.size.y, tileData.collision.size.z};
                    collision_json["offset"] = {tileData.collision.offset.x, tileData.collision.offset.y, tileData.collision.offset.z};
                    tile_json["collision"] = collision_json;

                    // Save custom data
                    if (!tileData.custom_data.empty()) {
                        json custom_data_json = json::object();
                        for (const auto& pair : tileData.custom_data) {
                            json value_json;
                            const auto& value = pair.second;
                            value_json["type"] = static_cast<int>(value.type);

                            switch (value.type) {
                                case Lupine::Tile3DDataType::String:
                                    value_json["value"] = value.string_value;
                                    break;
                                case Lupine::Tile3DDataType::Integer:
                                    value_json["value"] = value.int_value;
                                    break;
                                case Lupine::Tile3DDataType::Float:
                                    value_json["value"] = value.float_value;
                                    break;
                                case Lupine::Tile3DDataType::Boolean:
                                    value_json["value"] = value.bool_value;
                                    break;
                                case Lupine::Tile3DDataType::Vector3:
                                    value_json["value"] = {value.vec3_value.x, value.vec3_value.y, value.vec3_value.z};
                                    break;
                                case Lupine::Tile3DDataType::Color:
                                    value_json["value"] = {value.color_value.x, value.color_value.y, value.color_value.z, value.color_value.w};
                                    break;
                            }
                            custom_data_json[pair.first] = value_json;
                        }
                        tile_json["custom_data"] = custom_data_json;
                    }
                }
            }

            tiles_array.push_back(tile_json);
        }
        j["tiles"] = tiles_array;

        std::ofstream file(filepath.toStdString());
        if (!file.is_open()) {
            return false;
        }

        file << j.dump(4);
        file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving 3D tilemap: " << e.what() << std::endl;
        return false;
    }
}

bool TilemapCanvas3D::LoadTilemap(const QString& filepath) {
    try {
        std::ifstream file(filepath.toStdString());
        if (!file.is_open()) {
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();

        json j = json::parse(content);

        if (j["type"] != "3DTilemap") {
            std::cerr << "Invalid 3D tilemap file format" << std::endl;
            return false;
        }

        // Clear existing tiles
        ClearTiles();

        // Load settings
        if (j.contains("grid_size")) {
            m_gridSize = j["grid_size"];
        }

        // Load tiles
        if (j.contains("tiles")) {
            for (const auto& tile_json : j["tiles"]) {
                PlacedTile3D tile(
                    tile_json["id"],
                    glm::vec3(
                        tile_json["position"][0],
                        tile_json["position"][1],
                        tile_json["position"][2]
                    )
                );

                if (tile_json.contains("rotation")) {
                    tile.rotation = glm::vec3(
                        tile_json["rotation"][0],
                        tile_json["rotation"][1],
                        tile_json["rotation"][2]
                    );
                }

                if (tile_json.contains("scale")) {
                    tile.scale = glm::vec3(
                        tile_json["scale"][0],
                        tile_json["scale"][1],
                        tile_json["scale"][2]
                    );
                }

                m_placedTiles.push_back(tile);
            }
        }

        update();
        emit sceneModified();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading 3D tilemap: " << e.what() << std::endl;
        return false;
    }
}

// TilePaletteWidget Implementation
TilePaletteWidget::TilePaletteWidget(QWidget* parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_tileList(nullptr)
    , m_selectedTileId(-1)
{
    setupUI();
}

void TilePaletteWidget::setupUI() {
    m_layout = new QVBoxLayout(this);

    // Title
    QLabel* titleLabel = new QLabel("Tile Palette");
    titleLabel->setStyleSheet("font-weight: bold; padding: 5px;");
    m_layout->addWidget(titleLabel);

    // Tab widget for different views
    m_tabWidget = new QTabWidget();
    m_layout->addWidget(m_tabWidget);

    // List view tab
    QWidget* listTab = new QWidget();
    QVBoxLayout* listLayout = new QVBoxLayout(listTab);

    m_tileList = new QListWidget();
    m_tileList->setIconSize(QSize(64, 64));
    m_tileList->setViewMode(QListWidget::IconMode);
    m_tileList->setResizeMode(QListWidget::Adjust);
    m_tileList->setUniformItemSizes(true);
    listLayout->addWidget(m_tileList);

    m_tabWidget->addTab(listTab, "List View");

    // Grid view tab (2D tileset view)
    m_gridWidget = new QWidget();
    QVBoxLayout* gridMainLayout = new QVBoxLayout(m_gridWidget);

    // Scroll area for the grid
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    QWidget* gridContainer = new QWidget();
    m_gridLayout = new QGridLayout(gridContainer);
    m_gridLayout->setSpacing(2);

    scrollArea->setWidget(gridContainer);
    gridMainLayout->addWidget(scrollArea);

    m_tabWidget->addTab(m_gridWidget, "2D View");

    connect(m_tileList, &QListWidget::itemClicked, this, &TilePaletteWidget::onTileClicked);
}

void TilePaletteWidget::SetTileset(std::shared_ptr<Lupine::Tileset3DResource> tileset) {
    m_tileset = tileset;
    updateTileList();
    updateTileGrid();
}

void TilePaletteWidget::updateTileList() {
    m_tileList->clear();

    if (!m_tileset) {
        return;
    }

    // Add tiles from tileset
    const auto& tiles = m_tileset->GetTiles();
    for (const auto& pair : tiles) {
        const auto& tileData = pair.second;

        QListWidgetItem* item = new QListWidgetItem();
        item->setText(QString::fromStdString(tileData.name));
        item->setData(Qt::UserRole, tileData.id);

        // TODO: Load preview image if available
        // item->setIcon(QIcon(QString::fromStdString(tileData.preview_image_path)));

        m_tileList->addItem(item);
    }
}

void TilePaletteWidget::updateTileGrid() {
    // Clear existing grid items
    QLayoutItem* child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    if (!m_tileset) {
        return;
    }

    // Add tiles to grid in a 2D layout
    const auto& tiles = m_tileset->GetTiles();
    int row = 0;
    int col = 0;
    int maxCols = 4; // 4 tiles per row

    for (const auto& pair : tiles) {
        const auto& tileData = pair.second;

        // Create a clickable tile button
        QPushButton* tileButton = new QPushButton();
        tileButton->setFixedSize(80, 80);
        tileButton->setText(QString::fromStdString(tileData.name));
        tileButton->setToolTip(QString("ID: %1\nName: %2").arg(tileData.id).arg(QString::fromStdString(tileData.name)));

        // Store tile ID in button
        tileButton->setProperty("tileId", tileData.id);

        // Connect button click
        connect(tileButton, &QPushButton::clicked, [this, tileData]() {
            m_selectedTileId = tileData.id;
            emit tileSelected(m_selectedTileId);
        });

        // TODO: Load preview image if available
        // if (!tileData.preview_image_path.empty()) {
        //     QPixmap pixmap(QString::fromStdString(tileData.preview_image_path));
        //     if (!pixmap.isNull()) {
        //         tileButton->setIcon(QIcon(pixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        //         tileButton->setText("");
        //     }
        // }

        m_gridLayout->addWidget(tileButton, row, col);

        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }

    // Add stretch to push tiles to top-left
    m_gridLayout->setRowStretch(row + 1, 1);
    m_gridLayout->setColumnStretch(maxCols, 1);
}

void TilePaletteWidget::onTileClicked(QListWidgetItem* item) {
    if (item) {
        m_selectedTileId = item->data(Qt::UserRole).toInt();
        emit tileSelected(m_selectedTileId);
    }
}

// TilemapBuilder3DDialog Implementation
TilemapBuilder3DDialog::TilemapBuilder3DDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_canvas(nullptr)
    , m_toolPanel(nullptr)
    , m_paletteWidget(nullptr)
    , m_placementModeCombo(nullptr)
    , m_gridSizeSlider(nullptr)
    , m_gridSizeSpinBox(nullptr)
    , m_showGridCheck(nullptr)
    , m_tileCountLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_modified(false)
{
    setWindowTitle("3D Tilemap Builder");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    setupUI();
    updateWindowTitle();
    updateTileCount();
}

TilemapBuilder3DDialog::~TilemapBuilder3DDialog() {
}

void TilemapBuilder3DDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupMenuBar();
    setupToolBar();
    setupMainPanels();
}

void TilemapBuilder3DDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_mainLayout->addWidget(m_menuBar);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    fileMenu->addAction("&New Tilemap", this, &TilemapBuilder3DDialog::onNewTilemap, QKeySequence::New);
    fileMenu->addAction("&Open Tilemap...", this, &TilemapBuilder3DDialog::onOpenTilemap, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save Tilemap", this, &TilemapBuilder3DDialog::onSaveTilemap, QKeySequence::Save);
    fileMenu->addAction("Save &As...", this, &TilemapBuilder3DDialog::onSaveAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("Load &Tileset...", this, &TilemapBuilder3DDialog::onLoadTileset);
    fileMenu->addSeparator();
    fileMenu->addAction("Export &OBJ...", this, &TilemapBuilder3DDialog::onExportOBJ);
    fileMenu->addAction("Export &FBX...", this, &TilemapBuilder3DDialog::onExportFBX);

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    viewMenu->addAction("&Reset Camera", this, &TilemapBuilder3DDialog::onResetCamera, QKeySequence("R"));
    viewMenu->addAction("&Focus on Tiles", this, &TilemapBuilder3DDialog::onFocusOnTiles, QKeySequence("F"));
}

void TilemapBuilder3DDialog::setupToolBar() {
    m_toolBar = new QToolBar(this);
    m_toolBar->setFixedHeight(32);  // Set toolbar height to 32 pixels
    m_toolBar->setIconSize(QSize(16, 16));  // Set smaller icon size
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_mainLayout->addWidget(m_toolBar);

    m_toolBar->addAction("New", this, &TilemapBuilder3DDialog::onNewTilemap);
    m_toolBar->addAction("Open", this, &TilemapBuilder3DDialog::onOpenTilemap);
    m_toolBar->addAction("Save", this, &TilemapBuilder3DDialog::onSaveTilemap);
    m_toolBar->addSeparator();
    m_toolBar->addAction("Load Tileset", this, &TilemapBuilder3DDialog::onLoadTileset);
    m_toolBar->addSeparator();
    m_toolBar->addAction("Reset Camera", this, &TilemapBuilder3DDialog::onResetCamera);
    m_toolBar->addAction("Focus", this, &TilemapBuilder3DDialog::onFocusOnTiles);
}

void TilemapBuilder3DDialog::setupMainPanels() {
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);

    setupToolPanel();
    setupViewportPanel();

    // Set splitter proportions
    m_mainSplitter->setSizes({300, 1100});
}

void TilemapBuilder3DDialog::setupToolPanel() {
    m_toolPanel = new QWidget();
    m_toolPanel->setMinimumWidth(280);
    m_toolPanel->setMaximumWidth(350);
    m_mainSplitter->addWidget(m_toolPanel);

    QVBoxLayout* toolLayout = new QVBoxLayout(m_toolPanel);

    // Tile Palette
    m_paletteWidget = new TilePaletteWidget();
    toolLayout->addWidget(m_paletteWidget);

    // Tool Settings
    QGroupBox* settingsGroup = new QGroupBox("Tool Settings");
    toolLayout->addWidget(settingsGroup);

    QVBoxLayout* settingsLayout = new QVBoxLayout(settingsGroup);

    // Tool Selection
    settingsLayout->addWidget(new QLabel("Tool:"));
    QHBoxLayout* toolButtonLayout = new QHBoxLayout();
    m_toolButtonGroup = new QButtonGroup(this);

    m_placeToolButton = new QPushButton("Place");
    m_placeToolButton->setCheckable(true);
    m_placeToolButton->setChecked(true);
    m_toolButtonGroup->addButton(m_placeToolButton, static_cast<int>(TileTool::Place));
    toolButtonLayout->addWidget(m_placeToolButton);

    m_eraseToolButton = new QPushButton("Erase");
    m_eraseToolButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_eraseToolButton, static_cast<int>(TileTool::Erase));
    toolButtonLayout->addWidget(m_eraseToolButton);

    m_selectToolButton = new QPushButton("Select");
    m_selectToolButton->setCheckable(true);
    m_toolButtonGroup->addButton(m_selectToolButton, static_cast<int>(TileTool::Select));
    toolButtonLayout->addWidget(m_selectToolButton);

    settingsLayout->addLayout(toolButtonLayout);

    // Placement Mode
    settingsLayout->addWidget(new QLabel("Placement Mode:"));
    m_placementModeCombo = new QComboBox();
    m_placementModeCombo->addItem("Grid Snap", static_cast<int>(TilePlacementMode::GridSnap));
    m_placementModeCombo->addItem("Face Snap", static_cast<int>(TilePlacementMode::FaceSnap));
    m_placementModeCombo->addItem("Free Place", static_cast<int>(TilePlacementMode::FreePlace));
    settingsLayout->addWidget(m_placementModeCombo);

    // Grid Size
    settingsLayout->addWidget(new QLabel("Grid Size:"));
    QHBoxLayout* gridSizeLayout = new QHBoxLayout();
    m_gridSizeSlider = new QSlider(Qt::Horizontal);
    m_gridSizeSlider->setRange(1, 50);
    m_gridSizeSlider->setValue(10);
    m_gridSizeSpinBox = new QDoubleSpinBox();
    m_gridSizeSpinBox->setRange(0.1, 5.0);
    m_gridSizeSpinBox->setSingleStep(0.1);
    m_gridSizeSpinBox->setValue(1.0);
    gridSizeLayout->addWidget(m_gridSizeSlider);
    gridSizeLayout->addWidget(m_gridSizeSpinBox);
    settingsLayout->addLayout(gridSizeLayout);

    // Show Grid
    m_showGridCheck = new QCheckBox("Show Grid");
    m_showGridCheck->setChecked(true);
    settingsLayout->addWidget(m_showGridCheck);

    toolLayout->addStretch();

    // Status
    QGroupBox* statusGroup = new QGroupBox("Status");
    toolLayout->addWidget(statusGroup);

    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);
    m_tileCountLabel = new QLabel("Tiles: 0");
    m_statusLabel = new QLabel("Ready");
    statusLayout->addWidget(m_tileCountLabel);
    statusLayout->addWidget(m_statusLabel);

    // Connect signals
    connect(m_paletteWidget, &TilePaletteWidget::tileSelected, this, &TilemapBuilder3DDialog::onTileSelected);
    connect(m_toolButtonGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &TilemapBuilder3DDialog::onToolChanged);
    connect(m_placeToolButton, &QPushButton::clicked, this, &TilemapBuilder3DDialog::onPlaceToolSelected);
    connect(m_eraseToolButton, &QPushButton::clicked, this, &TilemapBuilder3DDialog::onEraseToolSelected);
    connect(m_selectToolButton, &QPushButton::clicked, this, &TilemapBuilder3DDialog::onSelectToolSelected);
    connect(m_placementModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TilemapBuilder3DDialog::onPlacementModeChanged);
    connect(m_gridSizeSlider, &QSlider::valueChanged, this, &TilemapBuilder3DDialog::onGridSizeChanged);
    connect(m_gridSizeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TilemapBuilder3DDialog::onGridSizeChanged);
    connect(m_showGridCheck, &QCheckBox::toggled, this, &TilemapBuilder3DDialog::onShowGridChanged);
}

void TilemapBuilder3DDialog::setupViewportPanel() {
    m_canvas = new TilemapCanvas3D();
    m_mainSplitter->addWidget(m_canvas);

    // Connect canvas signals
    connect(m_canvas, &TilemapCanvas3D::tileAdded, this, &TilemapBuilder3DDialog::onTileAdded);
    connect(m_canvas, &TilemapCanvas3D::tileRemoved, this, &TilemapBuilder3DDialog::onTileRemoved);
    connect(m_canvas, &TilemapCanvas3D::sceneModified, this, &TilemapBuilder3DDialog::onSceneModified);
}

// Slot implementations
void TilemapBuilder3DDialog::onNewTilemap() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    m_canvas->ClearTiles();
    m_currentFilePath.clear();
    setModified(false);
    updateWindowTitle();
    updateTileCount();
}

void TilemapBuilder3DDialog::onOpenTilemap() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    QString filepath = QFileDialog::getOpenFileName(this, "Open 3D Tilemap", "", "3D Tilemap Files (*.3dtilemap)");
    if (!filepath.isEmpty()) {
        if (m_canvas->LoadTilemap(filepath)) {
            m_currentFilePath = filepath;
            setModified(false);
            updateWindowTitle();
            updateTileCount();
        } else {
            QMessageBox::warning(this, "Error", "Failed to load tilemap file.");
        }
    }
}

void TilemapBuilder3DDialog::onSaveTilemap() {
    if (m_currentFilePath.isEmpty()) {
        onSaveAs();
        return;
    }

    if (m_canvas->SaveTilemap(m_currentFilePath)) {
        setModified(false);
        updateWindowTitle();
    } else {
        QMessageBox::warning(this, "Error", "Failed to save tilemap file.");
    }
}

void TilemapBuilder3DDialog::onSaveAs() {
    QString filepath = QFileDialog::getSaveFileName(this, "Save 3D Tilemap", "", "3D Tilemap Files (*.3dtilemap)");
    if (!filepath.isEmpty()) {
        if (m_canvas->SaveTilemap(filepath)) {
            m_currentFilePath = filepath;
            setModified(false);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save tilemap file.");
        }
    }
}

void TilemapBuilder3DDialog::onLoadTileset() {
    QString filepath = QFileDialog::getOpenFileName(this, "Load 3D Tileset", "", "3D Tileset Files (*.tileset3d)");
    if (!filepath.isEmpty()) {
        auto tileset = std::make_shared<Lupine::Tileset3DResource>();
        if (tileset->LoadFromFile(filepath.toStdString())) {
            m_tileset = tileset;
            m_currentTilesetPath = filepath;
            m_canvas->SetTileset(m_tileset);
            m_paletteWidget->SetTileset(m_tileset);
            m_statusLabel->setText("Tileset loaded: " + QFileInfo(filepath).baseName());
        } else {
            QMessageBox::warning(this, "Error", "Failed to load tileset file.");
        }
    }
}

void TilemapBuilder3DDialog::onExportOBJ() {
    QString filepath = QFileDialog::getSaveFileName(this, "Export OBJ", "", "OBJ Files (*.obj)");
    if (!filepath.isEmpty()) {
        if (m_canvas->ExportToOBJ(filepath, true)) {
            QMessageBox::information(this, "Success", "Successfully exported to OBJ file.");
        } else {
            QMessageBox::warning(this, "Error", "Failed to export OBJ file.");
        }
    }
}

void TilemapBuilder3DDialog::onExportFBX() {
    QString filepath = QFileDialog::getSaveFileName(this, "Export FBX", "", "FBX Files (*.fbx)");
    if (!filepath.isEmpty()) {
        if (m_canvas->ExportToFBX(filepath, true)) {
            QMessageBox::information(this, "Success", "Successfully exported to FBX file.");
        } else {
            QMessageBox::warning(this, "Error", "Failed to export FBX file.");
        }
    }
}

void TilemapBuilder3DDialog::onPlacementModeChanged() {
    TilePlacementMode mode = static_cast<TilePlacementMode>(m_placementModeCombo->currentData().toInt());
    m_canvas->SetPlacementMode(mode);
}

void TilemapBuilder3DDialog::onGridSizeChanged() {
    double value = m_gridSizeSpinBox->value();
    m_canvas->SetGridSize(static_cast<float>(value));

    // Sync slider and spinbox
    m_gridSizeSlider->blockSignals(true);
    m_gridSizeSlider->setValue(static_cast<int>(value * 10));
    m_gridSizeSlider->blockSignals(false);
}

void TilemapBuilder3DDialog::onShowGridChanged() {
    m_canvas->SetShowGrid(m_showGridCheck->isChecked());
}

void TilemapBuilder3DDialog::onResetCamera() {
    m_canvas->ResetCamera();
}

void TilemapBuilder3DDialog::onFocusOnTiles() {
    m_canvas->FocusOnTiles();
}

void TilemapBuilder3DDialog::onToolChanged(int toolId) {
    TileTool tool = static_cast<TileTool>(toolId);
    if (m_canvas) {
        m_canvas->SetCurrentTool(tool);
    }

    // Update UI based on selected tool
    switch (tool) {
        case TileTool::Place:
            m_placementModeCombo->setEnabled(true);
            break;
        case TileTool::Erase:
            m_placementModeCombo->setEnabled(true);
            break;
        case TileTool::Select:
            m_placementModeCombo->setEnabled(false);
            break;
    }
}

void TilemapBuilder3DDialog::onPlaceToolSelected() {
    m_placeToolButton->setChecked(true);
    onToolChanged(static_cast<int>(TileTool::Place));
}

void TilemapBuilder3DDialog::onEraseToolSelected() {
    m_eraseToolButton->setChecked(true);
    onToolChanged(static_cast<int>(TileTool::Erase));
}

void TilemapBuilder3DDialog::onSelectToolSelected() {
    m_selectToolButton->setChecked(true);
    onToolChanged(static_cast<int>(TileTool::Select));
}

void TilemapBuilder3DDialog::onTileSelected(int tile_id) {
    m_canvas->SetSelectedTileId(tile_id);
}

void TilemapBuilder3DDialog::onTileAdded(int tile_id, const glm::vec3& position) {
    Q_UNUSED(tile_id);
    Q_UNUSED(position);
    updateTileCount();
    setModified(true);
}

void TilemapBuilder3DDialog::onTileRemoved(const glm::vec3& position) {
    Q_UNUSED(position);
    updateTileCount();
    setModified(true);
}

void TilemapBuilder3DDialog::onSceneModified() {
    setModified(true);
}

// Utility methods
void TilemapBuilder3DDialog::updateWindowTitle() {
    QString title = "3D Tilemap Builder";
    if (!m_currentFilePath.isEmpty()) {
        title += " - " + QFileInfo(m_currentFilePath).baseName();
    }
    if (m_modified) {
        title += "*";
    }
    setWindowTitle(title);
}

void TilemapBuilder3DDialog::updateTileCount() {
    // Count tiles in canvas
    int count = static_cast<int>(m_canvas->GetTileCount());
    m_tileCountLabel->setText(QString("Tiles: %1").arg(count));
}

bool TilemapBuilder3DDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool TilemapBuilder3DDialog::promptSaveChanges() {
    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<TilemapBuilder3DDialog*>(this),
        "Unsaved Changes",
        "You have unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    if (result == QMessageBox::Save) {
        const_cast<TilemapBuilder3DDialog*>(this)->onSaveTilemap();
        return !m_modified; // Return true if save was successful
    } else if (result == QMessageBox::Discard) {
        return true;
    } else {
        return false; // Cancel
    }
}

void TilemapBuilder3DDialog::setModified(bool modified) {
    m_modified = modified;
    updateWindowTitle();
}


