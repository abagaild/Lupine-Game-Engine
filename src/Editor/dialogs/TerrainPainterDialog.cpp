#include "TerrainPainterDialog.h"
#include "lupine/core/Scene.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/terrain/TerrainData.h"
#include "lupine/terrain/TerrainBrush.h"
#include "lupine/components/TerrainRenderer.h"
#include "lupine/components/TerrainLoader.h"
#include "../panels/AssetBrowserPanel.h"
#include <iostream>
#include <QApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QSplitter>
#include <QTabWidget>
#include <QGroupBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QListWidget>
#include <QOpenGLWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QRegularExpression>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// TerrainViewport Implementation
TerrainViewport::TerrainViewport(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_scene(nullptr)
    , m_terrain(nullptr)
    , m_cameraPosition(0.0f, 25.0f, 25.0f)
    , m_cameraTarget(0.0f, 5.0f, 0.0f)
    , m_cameraDistance(35.0f)
    , m_cameraYaw(45.0f)
    , m_cameraPitch(-25.0f)
    , m_aspectRatio(1.0f)
    , m_viewMatrix(1.0f)
    , m_projectionMatrix(1.0f)
    , m_currentTool(TerrainTool::None)
    , m_brushSize(5.0f)
    , m_brushStrength(1.0f)
    , m_brushShape(TerrainBrushShape::Circle)
    , m_brushFalloff(0.5f)
    , m_isPainting(false)
    , m_isPanning(false)
    , m_shaderProgram(0)
    , m_gridVAO(0), m_gridVBO(0)
    , m_terrainVAO(0), m_terrainVBO(0), m_terrainEBO(0)
    , m_brushVAO(0), m_brushVBO(0)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    // Initialize camera position properly
    updateCamera();

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, QOverload<>::of(&TerrainViewport::update));
    m_updateTimer->start(16); // ~60 FPS
}

TerrainViewport::~TerrainViewport() {
    // Clean up OpenGL resources
    makeCurrent();

    if (m_shaderProgram != 0) {
        glDeleteProgram(m_shaderProgram);
    }

    if (m_gridVAO != 0) {
        glDeleteVertexArrays(1, &m_gridVAO);
    }
    if (m_gridVBO != 0) {
        glDeleteBuffers(1, &m_gridVBO);
    }

    if (m_terrainVAO != 0) {
        glDeleteVertexArrays(1, &m_terrainVAO);
    }
    if (m_terrainVBO != 0) {
        glDeleteBuffers(1, &m_terrainVBO);
    }
    if (m_terrainEBO != 0) {
        glDeleteBuffers(1, &m_terrainEBO);
    }

    if (m_brushVAO != 0) {
        glDeleteVertexArrays(1, &m_brushVAO);
    }
    if (m_brushVBO != 0) {
        glDeleteBuffers(1, &m_brushVBO);
    }

    doneCurrent();
}

void TerrainViewport::setScene(Lupine::Scene* scene) {
    m_scene = scene;
    update();
}

void TerrainViewport::setTerrain(Lupine::TerrainRenderer* terrain) {
    m_terrain = terrain;
    update();
}

void TerrainViewport::setCurrentTool(TerrainTool tool) {
    m_currentTool = tool;
    setCursor(tool == TerrainTool::None ? Qt::ArrowCursor : Qt::CrossCursor);
}

void TerrainViewport::setBrushSize(float size) {
    m_brushSize = size;
}

void TerrainViewport::setBrushStrength(float strength) {
    m_brushStrength = strength;
}

void TerrainViewport::setBrushShape(TerrainBrushShape shape) {
    m_brushShape = shape;
}

void TerrainViewport::setBrushFalloff(float falloff) {
    m_brushFalloff = falloff;
}

void TerrainViewport::resetCamera() {
    m_cameraTarget = glm::vec3(0.0f, 5.0f, 0.0f);
    m_cameraDistance = 35.0f;
    m_cameraYaw = 45.0f;
    m_cameraPitch = -25.0f;
    updateCamera();
    update();
}

void TerrainViewport::initializeGL() {
    // Initialize OpenGL functions
    initializeOpenGLFunctions();

    // Enable depth testing and face culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);

    // Initialize shaders
    setupShaders();

    // Initialize grid and terrain rendering buffers
    setupBuffers();

    std::cout << "TerrainViewport: OpenGL initialization complete" << std::endl;
}

void TerrainViewport::paintGL() {
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update camera matrices
    updateCameraMatrices();

    // Render grid for reference
    renderGrid();

    // Render terrain if available
    if (m_terrain && m_terrain->GetTerrainData()) {
        renderTerrain();
    }

    // Render brush preview if painting
    if (m_currentTool != TerrainTool::None) {
        renderBrushPreview();
    }
}

void TerrainViewport::resizeGL(int width, int height) {
    glViewport(0, 0, width, height);

    // Update projection matrix
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    updateCameraMatrices();
}

void TerrainViewport::mousePressEvent(QMouseEvent* event) {
    if (!event) {
        std::cout << "TerrainViewport: Null mouse event received" << std::endl;
        return;
    }

    if (event->button() == Qt::LeftButton && m_currentTool != TerrainTool::None) {
        m_isPainting = true;
        handleTerrainPainting(event->pos());
    } else if (event->button() == Qt::RightButton) {
        // Start camera rotation
        m_lastMousePos = event->pos();
    } else if (event->button() == Qt::MiddleButton) {
        // Start camera panning
        m_isPanning = true;
        m_lastMousePos = event->pos();
    }
}

void TerrainViewport::mouseMoveEvent(QMouseEvent* event) {
    if (!event) {
        std::cout << "TerrainViewport: Null mouse move event received" << std::endl;
        return;
    }

    if (m_isPainting && (event->buttons() & Qt::LeftButton)) {
        handleTerrainPainting(event->pos());
    } else if (event->buttons() & Qt::RightButton) {
        // Handle camera rotation
        QPoint delta = event->pos() - m_lastMousePos;
        m_cameraYaw += delta.x() * 0.3f;
        m_cameraPitch += delta.y() * 0.3f;
        m_cameraPitch = glm::clamp(m_cameraPitch, -89.0f, 89.0f);
        updateCamera();
        m_lastMousePos = event->pos();
        update();
    } else if (m_isPanning && (event->buttons() & Qt::MiddleButton)) {
        // Handle camera panning
        QPoint delta = event->pos() - m_lastMousePos;

        // Calculate pan speed based on camera distance
        float panSpeed = m_cameraDistance * 0.01f;

        // Get camera right and up vectors for panning
        glm::vec3 forward = glm::normalize(m_cameraTarget - m_cameraPosition);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, forward));

        // Apply panning to camera target
        m_cameraTarget -= right * (delta.x() * panSpeed);
        m_cameraTarget += up * (delta.y() * panSpeed);

        updateCamera();
        m_lastMousePos = event->pos();
        update();
    }
}

void TerrainViewport::mouseReleaseEvent(QMouseEvent* event) {
    if (!event) {
        std::cout << "TerrainViewport: Null mouse release event received" << std::endl;
        return;
    }

    if (event->button() == Qt::LeftButton) {
        m_isPainting = false;
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }
}

void TerrainViewport::wheelEvent(QWheelEvent* event) {
    if (!event) {
        std::cout << "TerrainViewport: Null wheel event received" << std::endl;
        return;
    }

    // Handle camera zoom with improved responsiveness
    float delta = event->angleDelta().y() / 120.0f;
    float zoomFactor = 1.0f + (delta * 0.15f);
    m_cameraDistance /= zoomFactor;
    m_cameraDistance = glm::clamp(m_cameraDistance, 2.0f, 200.0f);
    updateCamera();
    update();
}

void TerrainViewport::keyPressEvent(QKeyEvent* event) {
    // Handle camera movement with WASD
    float moveSpeed = m_cameraDistance * 0.05f;
    glm::vec3 forward = glm::normalize(m_cameraTarget - m_cameraPosition);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    bool cameraUpdated = false;

    switch (event->key()) {
        // Camera movement
        case Qt::Key_W:
            m_cameraTarget += forward * moveSpeed;
            cameraUpdated = true;
            break;
        case Qt::Key_S:
            m_cameraTarget -= forward * moveSpeed;
            cameraUpdated = true;
            break;
        case Qt::Key_A:
            m_cameraTarget -= right * moveSpeed;
            cameraUpdated = true;
            break;
        case Qt::Key_D:
            m_cameraTarget += right * moveSpeed;
            cameraUpdated = true;
            break;
        case Qt::Key_Q:
            m_cameraTarget.y += moveSpeed;
            cameraUpdated = true;
            break;
        case Qt::Key_E:
            m_cameraTarget.y -= moveSpeed;
            cameraUpdated = true;
            break;

        // Camera shortcuts
        case Qt::Key_R:
            resetCamera();
            break;

        // Tool shortcuts
        case Qt::Key_1: setCurrentTool(TerrainTool::HeightRaise); break;
        case Qt::Key_2: setCurrentTool(TerrainTool::HeightLower); break;
        case Qt::Key_3: setCurrentTool(TerrainTool::HeightFlatten); break;
        case Qt::Key_4: setCurrentTool(TerrainTool::HeightSmooth); break;
        case Qt::Key_5: setCurrentTool(TerrainTool::TexturePaint); break;
        case Qt::Key_6: setCurrentTool(TerrainTool::AssetScatter); break;
        case Qt::Key_7: setCurrentTool(TerrainTool::AssetErase); break;

        default: QOpenGLWidget::keyPressEvent(event); break;
    }

    if (cameraUpdated) {
        updateCamera();
        update();
    }
}

void TerrainViewport::updateCamera() {
    // Clamp camera parameters to reasonable bounds
    m_cameraDistance = glm::clamp(m_cameraDistance, 2.0f, 200.0f);
    m_cameraPitch = glm::clamp(m_cameraPitch, -89.0f, 89.0f);

    // Normalize yaw to 0-360 range
    while (m_cameraYaw > 360.0f) m_cameraYaw -= 360.0f;
    while (m_cameraYaw < 0.0f) m_cameraYaw += 360.0f;

    // Update camera position based on spherical coordinates
    float radYaw = glm::radians(m_cameraYaw);
    float radPitch = glm::radians(m_cameraPitch);

    m_cameraPosition.x = m_cameraTarget.x + m_cameraDistance * cos(radPitch) * cos(radYaw);
    m_cameraPosition.y = m_cameraTarget.y + m_cameraDistance * sin(radPitch);
    m_cameraPosition.z = m_cameraTarget.z + m_cameraDistance * cos(radPitch) * sin(radYaw);

    // Ensure camera doesn't go below ground level
    if (m_cameraPosition.y < 1.0f) {
        m_cameraPosition.y = 1.0f;
    }
}

void TerrainViewport::handleTerrainPainting(const QPoint& position) {
    // Safety checks to prevent segmentation fault
    if (!m_terrain) {
        std::cout << "TerrainViewport: No terrain renderer available for painting" << std::endl;
        return;
    }

    // Check if terrain has valid terrain data
    if (!m_terrain->GetTerrainData()) {
        std::cout << "TerrainViewport: No terrain data available for painting" << std::endl;
        return;
    }

    // Convert screen position to world coordinates
    glm::vec3 worldPos = screenToWorld(position);

    // Validate world position (screenToWorld might return invalid coordinates)
    if (std::isnan(worldPos.x) || std::isnan(worldPos.y) || std::isnan(worldPos.z)) {
        std::cout << "TerrainViewport: Invalid world position from screen coordinates" << std::endl;
        return;
    }

    // Apply terrain modification based on current tool
    switch (m_currentTool) {
        case TerrainTool::HeightRaise:
            m_terrain->ModifyHeight(worldPos, m_brushStrength * 2.0f, m_brushSize, m_brushFalloff);
            break;
        case TerrainTool::HeightLower:
            m_terrain->ModifyHeight(worldPos, -m_brushStrength * 2.0f, m_brushSize, m_brushFalloff);
            break;
        case TerrainTool::HeightFlatten:
            m_terrain->FlattenHeight(worldPos, 0.0f, m_brushSize, m_brushStrength);
            break;
        case TerrainTool::HeightSmooth:
            m_terrain->SmoothHeight(worldPos, m_brushSize, m_brushStrength);
            break;
        case TerrainTool::TexturePaint: {
            // Get selected texture layer with validation
            int selectedLayer = 0; // Default to first layer
            if (auto* dialog = qobject_cast<TerrainPainterDialog*>(parent())) {
                selectedLayer = dialog->getSelectedTextureLayer();
            }

            // Validate texture layer index
            if (selectedLayer < 0 || selectedLayer >= m_terrain->GetTextureLayerCount()) {
                std::cout << "TerrainViewport: Invalid texture layer index: " << selectedLayer << std::endl;
                return;
            }

            m_terrain->PaintTexture(worldPos, selectedLayer, m_brushStrength, m_brushSize, m_brushFalloff);
            break;
        }
        case TerrainTool::AssetScatter: {
            // Get selected asset from the dialog
            std::vector<std::string> assets;

            if (auto* dialog = qobject_cast<TerrainPainterDialog*>(parent())) {
                QString selectedAsset = dialog->getSelectedAssetPath();
                if (!selectedAsset.isEmpty()) {
                    assets.push_back(selectedAsset.toStdString());
                }
            }

            // Fall back to default asset if none selected
            if (assets.empty()) {
                assets.push_back("assets/models/grass.obj");
                std::cout << "TerrainViewport: No asset selected, using default grass.obj" << std::endl;
            }

            // Get scatter settings from dialog
            float density = 1.0f;
            float scaleVariance = 0.2f;
            float rotationVariance = 1.0f;
            glm::vec2 heightOffset(-0.1f, 0.1f);

            if (auto* dialog = qobject_cast<TerrainPainterDialog*>(parent())) {
                density = dialog->getScatterDensity();
                scaleVariance = dialog->getScatterScaleVariance();
                rotationVariance = dialog->getScatterRotationVariance();
                heightOffset = dialog->getScatterHeightOffset();
            }

            m_terrain->ScatterAssets(worldPos, assets, density, m_brushSize, scaleVariance, rotationVariance, heightOffset);
            break;
        }
        case TerrainTool::AssetErase:
            m_terrain->RemoveAssets(worldPos, m_brushSize);
            break;
        default:
            break;
    }

    emit terrainModified();
}

glm::vec3 TerrainViewport::screenToWorld(const QPoint& screenPos) {
    // Convert screen coordinates to world coordinates
    // Basic implementation using viewport dimensions and camera position

    // Get viewport dimensions
    int viewportWidth = width();
    int viewportHeight = height();

    if (viewportWidth <= 0 || viewportHeight <= 0) {
        std::cout << "TerrainViewport: Invalid viewport dimensions" << std::endl;
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    // Normalize screen coordinates to [-1, 1] range
    float normalizedX = (2.0f * screenPos.x()) / viewportWidth - 1.0f;
    float normalizedY = 1.0f - (2.0f * screenPos.y()) / viewportHeight; // Flip Y axis

    // Simple projection from screen to world plane
    // Assume we're looking down at the terrain from above
    float worldX = m_cameraTarget.x + normalizedX * m_cameraDistance * 0.5f;
    float worldZ = m_cameraTarget.z + normalizedY * m_cameraDistance * 0.5f;
    float worldY = 0.0f; // Project onto ground plane

    return glm::vec3(worldX, worldY, worldZ);
}

void TerrainViewport::setupShaders() {
    // Vertex shader source
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;

        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;

            gl_Position = projection * view * vec4(FragPos, 1.0);
        }
    )";

    // Fragment shader source
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;

        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 viewPos;
        uniform vec4 objectColor;

        void main() {
            // Ambient lighting
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * lightColor;

            // Diffuse lighting
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            // Specular lighting
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * lightColor;

            vec3 result = (ambient + diffuse + specular) * objectColor.rgb;
            FragColor = vec4(result, objectColor.a);
        }
    )";

    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    // Check for vertex shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "TerrainViewport: Vertex shader compilation failed: " << infoLog << std::endl;
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Check for fragment shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "TerrainViewport: Fragment shader compilation failed: " << infoLog << std::endl;
    }

    // Link shaders
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    // Check for linking errors
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        std::cout << "TerrainViewport: Shader program linking failed: " << infoLog << std::endl;
    }

    // Delete shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void TerrainViewport::setupBuffers() {
    // Setup grid VAO/VBO for both grid and terrain rendering
    float gridVertices[] = {
        // Quad vertices for terrain chunks (position, normal, texcoord)
        -1.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, 0.0f, -1.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         1.0f, 0.0f,  1.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -1.0f, 0.0f,  1.0f,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
    };

    glGenVertexArrays(1, &m_gridVAO);
    glGenBuffers(1, &m_gridVBO);

    glBindVertexArray(m_gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridVertices), gridVertices, GL_STATIC_DRAW);

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
}

void TerrainViewport::updateCameraMatrices() {
    // Update view matrix
    m_viewMatrix = glm::lookAt(m_cameraPosition, m_cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    // Update projection matrix
    m_projectionMatrix = glm::perspective(glm::radians(45.0f), m_aspectRatio, 0.1f, 1000.0f);
}

void TerrainViewport::renderGrid() {
    if (m_shaderProgram == 0 || m_gridVAO == 0) return;

    glUseProgram(m_shaderProgram);

    // Set matrices
    glm::mat4 model = glm::mat4(1.0f);

    int modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    int viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    int projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    int lightPosLoc = glGetUniformLocation(m_shaderProgram, "lightPos");
    int lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    int viewPosLoc = glGetUniformLocation(m_shaderProgram, "viewPos");
    int objectColorLoc = glGetUniformLocation(m_shaderProgram, "objectColor");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &m_viewMatrix[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &m_projectionMatrix[0][0]);

    // Set lighting uniforms
    glUniform3f(lightPosLoc, m_cameraPosition.x, m_cameraPosition.y + 10.0f, m_cameraPosition.z);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(viewPosLoc, m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z);
    glUniform4f(objectColorLoc, 0.4f, 0.4f, 0.4f, 0.8f); // Semi-transparent gray grid

    // Render grid lines
    glBindVertexArray(m_gridVAO);

    // Enable blending for semi-transparent grid
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw grid as wireframe quads
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Render a grid of wireframe quads
    int gridSize = 5;
    int numQuads = 20;

    for (int x = -numQuads; x <= numQuads; x++) {
        for (int z = -numQuads; z <= numQuads; z++) {
            model = glm::translate(glm::mat4(1.0f), glm::vec3(x * gridSize, 0.0f, z * gridSize));
            model = glm::scale(model, glm::vec3(gridSize * 0.5f, 1.0f, gridSize * 0.5f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void TerrainViewport::renderTerrain() {
    if (!m_terrain || !m_terrain->GetTerrainData()) return;

    if (m_shaderProgram == 0) return;

    glUseProgram(m_shaderProgram);

    // Set matrices
    glm::mat4 model = glm::mat4(1.0f);

    int modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    int viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    int projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    int lightPosLoc = glGetUniformLocation(m_shaderProgram, "lightPos");
    int lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    int viewPosLoc = glGetUniformLocation(m_shaderProgram, "viewPos");
    int objectColorLoc = glGetUniformLocation(m_shaderProgram, "objectColor");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &m_viewMatrix[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &m_projectionMatrix[0][0]);

    // Set lighting uniforms
    glUniform3f(lightPosLoc, m_cameraPosition.x, m_cameraPosition.y + 10.0f, m_cameraPosition.z);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3f(viewPosLoc, m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z);
    glUniform4f(objectColorLoc, 0.3f, 0.7f, 0.3f, 1.0f); // Green terrain

    // Update terrain chunks if needed
    m_terrain->UpdateAllDirtyChunks();

    // Get terrain data for rendering
    auto terrain_data = m_terrain->GetTerrainData();
    auto bounds = terrain_data->GetBounds();
    glm::vec2 dimensions = terrain_data->GetDimensions();

    // Render multiple terrain chunks to make it more visible
    glBindVertexArray(m_gridVAO);

    // Render a grid of terrain chunks
    int chunkSize = 10;
    int numChunksX = static_cast<int>(dimensions.x / chunkSize);
    int numChunksZ = static_cast<int>(dimensions.y / chunkSize);

    for (int x = 0; x < numChunksX; x++) {
        for (int z = 0; z < numChunksZ; z++) {
            // Position each chunk
            glm::vec3 chunkPos(
                (x - numChunksX * 0.5f) * chunkSize,
                0.0f,
                (z - numChunksZ * 0.5f) * chunkSize
            );

            model = glm::translate(glm::mat4(1.0f), chunkPos);
            model = glm::scale(model, glm::vec3(chunkSize * 0.5f, 1.0f, chunkSize * 0.5f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

            // Render chunk as a quad
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
    }

    glBindVertexArray(0);
}

void TerrainViewport::renderBrushPreview() {
    if (m_currentTool == TerrainTool::None || m_brushSize <= 0.0f) return;

    // Get mouse position in world coordinates
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    if (!rect().contains(mousePos)) return;

    glm::vec3 worldPos = screenToWorld(mousePos);

    if (m_shaderProgram == 0) return;

    glUseProgram(m_shaderProgram);

    // Set matrices for brush preview
    glm::mat4 model = glm::translate(glm::mat4(1.0f), worldPos);
    model = glm::scale(model, glm::vec3(m_brushSize, 0.1f, m_brushSize));

    int modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    int objectColorLoc = glGetUniformLocation(m_shaderProgram, "objectColor");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

    // Set brush color based on tool
    switch (m_currentTool) {
        case TerrainTool::HeightRaise:
            glUniform4f(objectColorLoc, 0.0f, 1.0f, 0.0f, 0.5f); // Green
            break;
        case TerrainTool::HeightLower:
            glUniform4f(objectColorLoc, 1.0f, 0.0f, 0.0f, 0.5f); // Red
            break;
        case TerrainTool::HeightFlatten:
            glUniform4f(objectColorLoc, 0.0f, 0.0f, 1.0f, 0.5f); // Blue
            break;
        case TerrainTool::HeightSmooth:
            glUniform4f(objectColorLoc, 1.0f, 1.0f, 0.0f, 0.5f); // Yellow
            break;
        case TerrainTool::TexturePaint:
            glUniform4f(objectColorLoc, 1.0f, 0.0f, 1.0f, 0.5f); // Magenta
            break;
        case TerrainTool::AssetScatter:
            glUniform4f(objectColorLoc, 0.0f, 1.0f, 1.0f, 0.5f); // Cyan
            break;
        case TerrainTool::AssetErase:
            glUniform4f(objectColorLoc, 1.0f, 0.5f, 0.0f, 0.5f); // Orange
            break;
        default:
            glUniform4f(objectColorLoc, 1.0f, 1.0f, 1.0f, 0.5f); // White
            break;
    }

    // Enable blending for transparent brush preview
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render brush preview as wireframe circle
    glBindVertexArray(m_gridVAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

// TerrainPainterDialog Implementation
TerrainPainterDialog::TerrainPainterDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_rightSplitter(nullptr)
    , m_statusBar(nullptr)
    , m_viewport(nullptr)
    , m_toolTabs(nullptr)
    , m_terrainScene(nullptr)
    , m_terrainNode(nullptr)
    , m_terrainRenderer(nullptr)
    , m_terrainData(nullptr)
    , m_assetBrowser(nullptr)
    , m_terrainStatsDisplay(nullptr)
    , m_isModified(false)
    , m_currentTool(TerrainTool::None)
{
    try {
        std::cout << "TerrainPainterDialog: Starting constructor" << std::endl;

        setWindowTitle("Terrain Painter");
        setMinimumSize(1200, 800);
        resize(1600, 1000);

        std::cout << "TerrainPainterDialog: Basic setup complete" << std::endl;

        // Create new terrain scene
        m_terrainScene = std::make_unique<Lupine::Scene>("Terrain");
        std::cout << "TerrainPainterDialog: Scene created" << std::endl;

        m_terrainScene->CreateRootNode<Lupine::Node3D>("TerrainRoot");
        std::cout << "TerrainPainterDialog: Root node created" << std::endl;

        setupUI();
        std::cout << "TerrainPainterDialog: UI setup complete" << std::endl;

        setupConnections();
        std::cout << "TerrainPainterDialog: Connections setup complete" << std::endl;

        updateWindowTitle();

        // Create a default terrain to start with
        onNewTerrain();

        std::cout << "TerrainPainterDialog: Constructor completed successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "TerrainPainterDialog: Exception in constructor: " << e.what() << std::endl;
        throw;
    }
    catch (...) {
        std::cout << "TerrainPainterDialog: Unknown exception in constructor" << std::endl;
        throw;
    }
}

TerrainPainterDialog::~TerrainPainterDialog() = default;

void TerrainPainterDialog::setupUI() {
    try {
        std::cout << "TerrainPainterDialog: Starting setupMenuBar" << std::endl;
        setupMenuBar();
        std::cout << "TerrainPainterDialog: setupMenuBar completed" << std::endl;

        std::cout << "TerrainPainterDialog: Starting setupToolBar" << std::endl;
        setupToolBar();
        std::cout << "TerrainPainterDialog: setupToolBar completed" << std::endl;

        std::cout << "TerrainPainterDialog: Starting setupMainLayout" << std::endl;
        setupMainLayout();
        std::cout << "TerrainPainterDialog: setupMainLayout completed" << std::endl;

        std::cout << "TerrainPainterDialog: Starting setupStatusBar" << std::endl;
        setupStatusBar();
        std::cout << "TerrainPainterDialog: setupStatusBar completed" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "TerrainPainterDialog: Exception in setupUI: " << e.what() << std::endl;
        throw;
    }
    catch (...) {
        std::cout << "TerrainPainterDialog: Unknown exception in setupUI" << std::endl;
        throw;
    }
}

void TerrainPainterDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    
    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    m_newTerrainAction = fileMenu->addAction("&New Terrain", this, &TerrainPainterDialog::onNewTerrain, QKeySequence::New);
    m_openTerrainAction = fileMenu->addAction("&Open Terrain...", this, &TerrainPainterDialog::onOpenTerrain, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveTerrainAction = fileMenu->addAction("&Save Terrain", this, &TerrainPainterDialog::onSaveTerrain, QKeySequence::Save);
    m_saveTerrainAsAction = fileMenu->addAction("Save Terrain &As...", this, &TerrainPainterDialog::onSaveTerrainAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_exportOBJAction = fileMenu->addAction("&Export to OBJ...", this, &TerrainPainterDialog::onExportOBJ);
    
    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    m_resetViewAction = viewMenu->addAction("&Reset View", this, &TerrainPainterDialog::onResetView);
    viewMenu->addSeparator();
    m_showGridAction = viewMenu->addAction("Show &Grid", this, &TerrainPainterDialog::onShowGrid);
    m_showGridAction->setCheckable(true);
    m_showGridAction->setChecked(true);
    m_showWireframeAction = viewMenu->addAction("Show &Wireframe", this, &TerrainPainterDialog::onShowWireframe);
    m_showWireframeAction->setCheckable(true);

    // Note: QDialog doesn't need explicit menu bar setup in layout
}

void TerrainPainterDialog::setupToolBar() {
    m_toolBar = new QToolBar("Tools", this);
    m_toolBar->addAction(m_newTerrainAction);
    m_toolBar->addAction(m_openTerrainAction);
    m_toolBar->addAction(m_saveTerrainAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_exportOBJAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_resetViewAction);
    m_toolBar->addAction(m_showGridAction);
    m_toolBar->addAction(m_showWireframeAction);
}

void TerrainPainterDialog::setupMainLayout() {
    try {
        std::cout << "TerrainPainterDialog: Creating main layout" << std::endl;
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(0, 0, 0, 0);
        m_mainLayout->setSpacing(0);

        std::cout << "TerrainPainterDialog: Adding menu bar to layout" << std::endl;
        // Add menu bar
        if (m_menuBar) {
            m_mainLayout->addWidget(m_menuBar);
        }

        std::cout << "TerrainPainterDialog: Adding toolbar to layout" << std::endl;
        // Add toolbar
        if (m_toolBar) {
            m_mainLayout->addWidget(m_toolBar);
        }

        std::cout << "TerrainPainterDialog: Creating main splitter" << std::endl;
        // Create main splitter
        m_mainSplitter = new QSplitter(Qt::Horizontal, this);
        m_mainLayout->addWidget(m_mainSplitter);

        std::cout << "TerrainPainterDialog: Creating viewport" << std::endl;
        // Create viewport
        m_viewport = new TerrainViewport(this);
        std::cout << "TerrainPainterDialog: Viewport created successfully" << std::endl;

        m_mainSplitter->addWidget(m_viewport);
        std::cout << "TerrainPainterDialog: Viewport added to splitter" << std::endl;

        std::cout << "TerrainPainterDialog: Setting up tool panel" << std::endl;
        // Create right panel with tabs
        setupToolPanel();
        std::cout << "TerrainPainterDialog: Tool panel setup completed" << std::endl;

        m_mainSplitter->addWidget(m_toolTabs);

        std::cout << "TerrainPainterDialog: Setting splitter proportions" << std::endl;
        // Set splitter proportions
        m_mainSplitter->setSizes({800, 400});
        std::cout << "TerrainPainterDialog: Main layout setup completed" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "TerrainPainterDialog: Exception in setupMainLayout: " << e.what() << std::endl;
        throw;
    }
    catch (...) {
        std::cout << "TerrainPainterDialog: Unknown exception in setupMainLayout" << std::endl;
        throw;
    }
}

void TerrainPainterDialog::setupToolPanel() {
    m_toolTabs = new QTabWidget(this);
    m_toolTabs->setMinimumWidth(350);
    m_toolTabs->setMaximumWidth(400);
    
    setupBrushPanel();
    setupTexturePanel();
    setupAssetPanel();
    setupTerrainPanel();
    
    m_toolTabs->addTab(m_brushPanel, "Brush");
    m_toolTabs->addTab(m_texturePanel, "Textures");
    m_toolTabs->addTab(m_assetPanel, "Assets");
    m_toolTabs->addTab(m_terrainPanel, "Terrain");
}

void TerrainPainterDialog::setupBrushPanel() {
    m_brushPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_brushPanel);

    // Tool selection
    QGroupBox* toolGroup = new QGroupBox("Tool", m_brushPanel);
    QVBoxLayout* toolLayout = new QVBoxLayout(toolGroup);

    m_toolCombo = new QComboBox();
    m_toolCombo->addItems({"None", "Raise Height", "Lower Height", "Flatten", "Smooth", "Paint Texture", "Scatter Assets", "Erase Assets"});
    toolLayout->addWidget(m_toolCombo);

    layout->addWidget(toolGroup);

    // Brush settings
    QGroupBox* brushGroup = new QGroupBox("Brush Settings", m_brushPanel);
    QFormLayout* brushLayout = new QFormLayout(brushGroup);

    // Brush size
    m_brushSizeSlider = new QSlider(Qt::Horizontal);
    m_brushSizeSlider->setRange(1, 50);
    m_brushSizeSlider->setValue(5);
    m_brushSizeSpin = new QDoubleSpinBox();
    m_brushSizeSpin->setRange(0.1, 50.0);
    m_brushSizeSpin->setValue(5.0);
    m_brushSizeSpin->setSingleStep(0.1);

    QHBoxLayout* sizeLayout = new QHBoxLayout();
    sizeLayout->addWidget(m_brushSizeSlider);
    sizeLayout->addWidget(m_brushSizeSpin);
    brushLayout->addRow("Size:", sizeLayout);

    // Brush strength
    m_brushStrengthSlider = new QSlider(Qt::Horizontal);
    m_brushStrengthSlider->setRange(1, 100);
    m_brushStrengthSlider->setValue(50);
    m_brushStrengthSpin = new QDoubleSpinBox();
    m_brushStrengthSpin->setRange(0.01, 1.0);
    m_brushStrengthSpin->setValue(0.5);
    m_brushStrengthSpin->setSingleStep(0.01);

    QHBoxLayout* strengthLayout = new QHBoxLayout();
    strengthLayout->addWidget(m_brushStrengthSlider);
    strengthLayout->addWidget(m_brushStrengthSpin);
    brushLayout->addRow("Strength:", strengthLayout);

    // Brush falloff
    m_brushFalloffSlider = new QSlider(Qt::Horizontal);
    m_brushFalloffSlider->setRange(0, 100);
    m_brushFalloffSlider->setValue(50);
    m_brushFalloffSpin = new QDoubleSpinBox();
    m_brushFalloffSpin->setRange(0.0, 1.0);
    m_brushFalloffSpin->setValue(0.5);
    m_brushFalloffSpin->setSingleStep(0.01);

    QHBoxLayout* falloffLayout = new QHBoxLayout();
    falloffLayout->addWidget(m_brushFalloffSlider);
    falloffLayout->addWidget(m_brushFalloffSpin);
    brushLayout->addRow("Falloff:", falloffLayout);

    // Brush shape
    m_brushShapeCombo = new QComboBox();
    m_brushShapeCombo->addItems({"Circle", "Square", "Diamond", "Custom"});
    brushLayout->addRow("Shape:", m_brushShapeCombo);

    layout->addWidget(brushGroup);
    layout->addStretch();
}

void TerrainPainterDialog::setupTexturePanel() {
    m_texturePanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_texturePanel);

    // Texture layers
    QGroupBox* layersGroup = new QGroupBox("Texture Layers", m_texturePanel);
    QVBoxLayout* layersLayout = new QVBoxLayout(layersGroup);

    m_textureList = new QListWidget();
    m_textureList->setMaximumHeight(200);
    layersLayout->addWidget(m_textureList);

    // Layer management buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_addTextureButton = new QPushButton("Add Texture");
    m_removeTextureButton = new QPushButton("Remove Texture");
    buttonLayout->addWidget(m_addTextureButton);
    buttonLayout->addWidget(m_removeTextureButton);
    layersLayout->addLayout(buttonLayout);

    layout->addWidget(layersGroup);

    // Texture painting settings
    QGroupBox* paintGroup = new QGroupBox("Paint Settings", m_texturePanel);
    QFormLayout* paintLayout = new QFormLayout(paintGroup);

    // Texture opacity
    m_textureOpacitySlider = new QSlider(Qt::Horizontal);
    m_textureOpacitySlider->setRange(1, 100);
    m_textureOpacitySlider->setValue(100);
    paintLayout->addRow("Opacity:", m_textureOpacitySlider);

    // Texture scale
    m_textureScaleSlider = new QSlider(Qt::Horizontal);
    m_textureScaleSlider->setRange(1, 100);
    m_textureScaleSlider->setValue(10);
    paintLayout->addRow("Scale:", m_textureScaleSlider);

    layout->addWidget(paintGroup);
    layout->addStretch();
}

void TerrainPainterDialog::setupAssetPanel() {
    m_assetPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_assetPanel);

    // Asset browser integration
    QGroupBox* browserGroup = new QGroupBox("Asset Browser", m_assetPanel);
    QVBoxLayout* browserLayout = new QVBoxLayout(browserGroup);

    // Create and integrate the actual AssetBrowserPanel
    m_assetBrowser = new AssetBrowserPanel(this);
    m_assetBrowser->setMinimumHeight(200);
    m_assetBrowser->setMaximumHeight(300);

    // Set a default project path (this should be set properly when a project is loaded)
    // For now, use current directory as fallback
    QString projectPath = QDir::currentPath();
    m_assetBrowser->setProjectPath(projectPath);

    // Connect asset selection signals
    connect(m_assetBrowser, &AssetBrowserPanel::assetSelected, this, &TerrainPainterDialog::onAssetSelected);

    browserLayout->addWidget(m_assetBrowser);
    layout->addWidget(browserGroup);

    // Scatter settings
    QGroupBox* scatterGroup = new QGroupBox("Scatter Settings", m_assetPanel);
    QFormLayout* scatterLayout = new QFormLayout(scatterGroup);

    // Scatter mode
    m_scatterModeCheck = new QCheckBox("Enable Scatter Mode");
    scatterLayout->addRow(m_scatterModeCheck);

    // Density
    m_scatterDensitySlider = new QSlider(Qt::Horizontal);
    m_scatterDensitySlider->setRange(1, 100);
    m_scatterDensitySlider->setValue(10);
    scatterLayout->addRow("Density:", m_scatterDensitySlider);

    // Scale variance
    m_scatterScaleVarianceSlider = new QSlider(Qt::Horizontal);
    m_scatterScaleVarianceSlider->setRange(0, 100);
    m_scatterScaleVarianceSlider->setValue(20);
    scatterLayout->addRow("Scale Variance:", m_scatterScaleVarianceSlider);

    // Rotation variance
    m_scatterRotationVarianceSlider = new QSlider(Qt::Horizontal);
    m_scatterRotationVarianceSlider->setRange(0, 100);
    m_scatterRotationVarianceSlider->setValue(100);
    scatterLayout->addRow("Rotation Variance:", m_scatterRotationVarianceSlider);

    // Height offset
    m_scatterHeightOffsetSlider = new QSlider(Qt::Horizontal);
    m_scatterHeightOffsetSlider->setRange(-100, 100);
    m_scatterHeightOffsetSlider->setValue(0);
    scatterLayout->addRow("Height Offset:", m_scatterHeightOffsetSlider);

    layout->addWidget(scatterGroup);
    layout->addStretch();
}

void TerrainPainterDialog::setupTerrainPanel() {
    m_terrainPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_terrainPanel);

    // Terrain creation group
    QGroupBox* creationGroup = new QGroupBox("Terrain Creation", m_terrainPanel);
    QFormLayout* creationLayout = new QFormLayout(creationGroup);

    // Terrain dimensions
    m_terrainWidthSpin = new QSpinBox();
    m_terrainWidthSpin->setRange(32, 1024);
    m_terrainWidthSpin->setValue(128);
    m_terrainWidthSpin->setSuffix(" units");
    creationLayout->addRow("Width:", m_terrainWidthSpin);

    m_terrainHeightSpin = new QSpinBox();
    m_terrainHeightSpin->setRange(32, 1024);
    m_terrainHeightSpin->setValue(128);
    m_terrainHeightSpin->setSuffix(" units");
    creationLayout->addRow("Height:", m_terrainHeightSpin);

    // Terrain resolution
    m_terrainScaleSpin = new QDoubleSpinBox();
    m_terrainScaleSpin->setRange(0.1, 10.0);
    m_terrainScaleSpin->setValue(1.0);
    m_terrainScaleSpin->setSingleStep(0.1);
    m_terrainScaleSpin->setSuffix(" units/sample");
    creationLayout->addRow("Resolution:", m_terrainScaleSpin);

    // Chunk size
    m_chunkSizeSpin = new QSpinBox();
    m_chunkSizeSpin->setRange(16, 128);
    m_chunkSizeSpin->setValue(32);
    m_chunkSizeSpin->setSuffix(" units");
    creationLayout->addRow("Chunk Size:", m_chunkSizeSpin);

    // Create new terrain button
    QPushButton* createTerrainBtn = new QPushButton("Create New Terrain");
    connect(createTerrainBtn, &QPushButton::clicked, this, &TerrainPainterDialog::createTerrainChunk);
    creationLayout->addRow(createTerrainBtn);

    layout->addWidget(creationGroup);

    // Terrain properties group
    QGroupBox* propertiesGroup = new QGroupBox("Terrain Properties", m_terrainPanel);
    QFormLayout* propertiesLayout = new QFormLayout(propertiesGroup);

    // Collision settings
    m_enableCollisionCheck = new QCheckBox();
    m_enableCollisionCheck->setChecked(true);
    propertiesLayout->addRow("Enable Collision:", m_enableCollisionCheck);

    m_collisionTypeCombo = new QComboBox();
    m_collisionTypeCombo->addItems({"Convex", "Concave", "Heightfield"});
    m_collisionTypeCombo->setCurrentText("Heightfield");
    propertiesLayout->addRow("Collision Type:", m_collisionTypeCombo);

    layout->addWidget(propertiesGroup);

    // Terrain management group
    QGroupBox* managementGroup = new QGroupBox("Terrain Management", m_terrainPanel);
    QVBoxLayout* managementLayout = new QVBoxLayout(managementGroup);

    // Terrain operations
    QPushButton* clearTerrainBtn = new QPushButton("Clear Terrain");
    clearTerrainBtn->setToolTip("Reset all terrain heights to zero");
    connect(clearTerrainBtn, &QPushButton::clicked, this, &TerrainPainterDialog::clearTerrain);
    managementLayout->addWidget(clearTerrainBtn);

    QPushButton* flattenTerrainBtn = new QPushButton("Flatten Terrain");
    flattenTerrainBtn->setToolTip("Flatten entire terrain to a specific height");
    connect(flattenTerrainBtn, &QPushButton::clicked, this, &TerrainPainterDialog::flattenTerrain);
    managementLayout->addWidget(flattenTerrainBtn);

    QPushButton* generateNoiseBtn = new QPushButton("Generate Noise");
    generateNoiseBtn->setToolTip("Generate random noise across the terrain");
    connect(generateNoiseBtn, &QPushButton::clicked, this, &TerrainPainterDialog::generateNoise);
    managementLayout->addWidget(generateNoiseBtn);

    // Terrain statistics
    QLabel* statsLabel = new QLabel("Terrain Statistics:");
    statsLabel->setStyleSheet("font-weight: bold;");
    managementLayout->addWidget(statsLabel);

    m_terrainStatsDisplay = new QLabel("No terrain loaded");
    m_terrainStatsDisplay->setWordWrap(true);
    m_terrainStatsDisplay->setStyleSheet("padding: 5px; background-color: #f0f0f0; border: 1px solid #ccc;");
    managementLayout->addWidget(m_terrainStatsDisplay);

    layout->addWidget(managementGroup);

    layout->addStretch();
}

void TerrainPainterDialog::setupStatusBar() {
    m_statusBar = new QStatusBar(this);
    m_statusLabel = new QLabel("Ready");
    m_terrainStatsLabel = new QLabel("No terrain loaded");
    m_operationProgress = new QProgressBar();
    m_operationProgress->setVisible(false);
    
    m_statusBar->addWidget(m_statusLabel);
    m_statusBar->addPermanentWidget(m_terrainStatsLabel);
    m_statusBar->addPermanentWidget(m_operationProgress);
    
    m_mainLayout->addWidget(m_statusBar);
}

void TerrainPainterDialog::setupConnections() {
    // Safety checks to prevent crashes
    if (!m_toolCombo) {
        std::cout << "TerrainPainterDialog: m_toolCombo is null in setupConnections" << std::endl;
        return;
    }
    if (!m_viewport) {
        std::cout << "TerrainPainterDialog: m_viewport is null in setupConnections" << std::endl;
        return;
    }
    if (!m_statusLabel) {
        std::cout << "TerrainPainterDialog: m_statusLabel is null in setupConnections" << std::endl;
        return;
    }

    connect(m_toolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TerrainPainterDialog::onToolChanged);
    connect(m_viewport, &TerrainViewport::terrainModified, this, &TerrainPainterDialog::onTerrainModified);
    connect(m_viewport, &TerrainViewport::statusMessage, m_statusLabel, &QLabel::setText);

    // Brush size connections
    if (m_brushSizeSlider && m_brushSizeSpin) {
        connect(m_brushSizeSlider, &QSlider::valueChanged, [this](int value) {
            m_brushSizeSpin->setValue(static_cast<double>(value));
            onBrushSettingsChanged();
        });
        connect(m_brushSizeSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value) {
            m_brushSizeSlider->setValue(static_cast<int>(value));
            onBrushSettingsChanged();
        });
    }

    // Brush strength connections
    if (m_brushStrengthSlider && m_brushStrengthSpin) {
        connect(m_brushStrengthSlider, &QSlider::valueChanged, [this](int value) {
            m_brushStrengthSpin->setValue(value / 100.0);
            onBrushSettingsChanged();
        });
        connect(m_brushStrengthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value) {
            m_brushStrengthSlider->setValue(static_cast<int>(value * 100));
            onBrushSettingsChanged();
        });
    }

    // Brush falloff connections
    if (m_brushFalloffSlider && m_brushFalloffSpin) {
        connect(m_brushFalloffSlider, &QSlider::valueChanged, [this](int value) {
            m_brushFalloffSpin->setValue(value / 100.0);
            onBrushSettingsChanged();
        });
        connect(m_brushFalloffSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value) {
            m_brushFalloffSlider->setValue(static_cast<int>(value * 100));
            onBrushSettingsChanged();
        });
    }

    // Brush shape connection
    if (m_brushShapeCombo) {
        connect(m_brushShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TerrainPainterDialog::onBrushSettingsChanged);
    }

    // Texture layer connections
    if (m_addTextureButton) {
        connect(m_addTextureButton, &QPushButton::clicked, [this]() {
            QString filePath = QFileDialog::getOpenFileName(this, "Select Texture", "",
                "Image Files (*.png *.jpg *.jpeg *.tga *.bmp)");
            if (!filePath.isEmpty()) {
                addTextureLayer(filePath);
            }
        });
    }

    if (m_removeTextureButton) {
        connect(m_removeTextureButton, &QPushButton::clicked, [this]() {
            int currentRow = m_textureList ? m_textureList->currentRow() : -1;
            if (currentRow >= 0) {
                removeTextureLayer(currentRow);
            }
        });
    }

    if (m_textureList) {
        connect(m_textureList, &QListWidget::currentRowChanged, this, &TerrainPainterDialog::onTextureSelected);
    }
}

void TerrainPainterDialog::updateWindowTitle() {
    QString title = "Terrain Painter";
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.baseName();
    }
    if (m_isModified) {
        title += "*";
    }
    setWindowTitle(title);
}

// Placeholder implementations for slots
void TerrainPainterDialog::onNewTerrain() {
    // Safety checks to prevent crashes
    if (!m_terrainScene) {
        std::cout << "TerrainPainterDialog: No terrain scene available" << std::endl;
        return;
    }

    if (!m_viewport) {
        std::cout << "TerrainPainterDialog: No viewport available" << std::endl;
        return;
    }

    try {
        // Create new terrain data
        m_terrainData = std::make_shared<Lupine::TerrainData>(128.0f, 128.0f, 1.0f, 32.0f);

        // Initialize with flat terrain
        m_terrainData->InitializeFlatTerrain(0.0f, 4, 4);

        // Create terrain node and renderer if not exists
        if (!m_terrainNode) {
            // Ensure we have a root node
            if (!m_terrainScene->GetRootNode()) {
                m_terrainScene->CreateRootNode("Root");
            }

            if (!m_terrainScene->GetRootNode()) {
                std::cout << "TerrainPainterDialog: Failed to create root node" << std::endl;
                return;
            }

            m_terrainNode = m_terrainScene->GetRootNode()->CreateChild<Lupine::Node3D>("Terrain");
            if (!m_terrainNode) {
                std::cout << "TerrainPainterDialog: Failed to create terrain node" << std::endl;
                return;
            }

            m_terrainRenderer = m_terrainNode->AddComponent<Lupine::TerrainRenderer>();
            if (!m_terrainRenderer) {
                std::cout << "TerrainPainterDialog: Failed to create terrain renderer" << std::endl;
                return;
            }
        }

        // Set terrain data
        m_terrainRenderer->SetTerrainData(m_terrainData);
        m_viewport->setTerrain(m_terrainRenderer);

        // Update UI
        m_isModified = false;
        m_currentFilePath.clear();
        updateWindowTitle();
        updateTerrainStats();

        std::cout << "TerrainPainterDialog: New terrain created successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "TerrainPainterDialog: Error creating new terrain: " << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "TerrainPainterDialog: Unknown error creating new terrain" << std::endl;
    }
}

void TerrainPainterDialog::onOpenTerrain() {
    // Check if current terrain has unsaved changes
    if (m_isModified) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
            "Unsaved Changes",
            "The current terrain has unsaved changes. Do you want to save before opening a new terrain?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (reply == QMessageBox::Save) {
            onSaveTerrain();
            if (m_isModified) return; // Save was cancelled or failed
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }

    // Get supported file extensions from TerrainLoader
    QStringList filters;
    filters << "Lupine Terrain Files (*.terrain)";
    filters << "Heightmap Files (*.raw *.r16 *.r32)";
    filters << "Image Files (*.png *.jpg *.jpeg *.tga *.exr)";
    filters << "All Supported Files (*.terrain *.raw *.r16 *.r32 *.png *.jpg *.jpeg *.tga *.exr)";

    QString fileName = QFileDialog::getOpenFileName(this,
        "Open Terrain File",
        m_currentFilePath.isEmpty() ? QDir::currentPath() : QFileInfo(m_currentFilePath).absolutePath(),
        filters.join(";;"));

    if (!fileName.isEmpty()) {
        loadTerrainFile(fileName);
    }
}

void TerrainPainterDialog::onSaveTerrain() {
    if (!m_terrainData) {
        m_statusLabel->setText("No terrain to save");
        return;
    }

    if (m_currentFilePath.isEmpty()) {
        // No current file path, use Save As
        onSaveTerrainAs();
    } else {
        // Save to current file path
        saveTerrainFile(m_currentFilePath);
    }
}

void TerrainPainterDialog::onSaveTerrainAs() {
    if (!m_terrainData) {
        m_statusLabel->setText("No terrain to save");
        return;
    }

    // Get supported file extensions for saving
    QStringList filters;
    filters << "Lupine Terrain Files (*.terrain)";
    filters << "Heightmap Files (*.raw)";

    QString fileName = QFileDialog::getSaveFileName(this,
        "Save Terrain File",
        m_currentFilePath.isEmpty() ? QDir::currentPath() + "/terrain.terrain" : m_currentFilePath,
        filters.join(";;"));

    if (!fileName.isEmpty()) {
        saveTerrainFile(fileName);
    }
}

void TerrainPainterDialog::onExportOBJ() {
    if (!m_terrainData) {
        m_statusLabel->setText("No terrain to export");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Terrain to OBJ",
        m_currentFilePath.isEmpty() ? QDir::currentPath() + "/terrain.obj" :
            QFileInfo(m_currentFilePath).absolutePath() + "/" + QFileInfo(m_currentFilePath).baseName() + ".obj",
        "Wavefront OBJ Files (*.obj)");

    if (!fileName.isEmpty()) {
        try {
            // Create a TerrainLoader to handle the OBJ export
            auto terrainLoader = std::make_unique<Lupine::TerrainLoader>();
            terrainLoader->SetTerrainData(m_terrainData);

            // Set up progress callback
            auto progressCallback = [this](float progress, const std::string& status) {
                m_statusLabel->setText(QString("Exporting: %1 (%2%)").arg(QString::fromStdString(status)).arg(static_cast<int>(progress * 100)));
                QApplication::processEvents(); // Allow UI updates
            };

            // Set up OBJ export options
            Lupine::TerrainExportOptions options;
            options.format = Lupine::TerrainFileFormat::OBJ;
            options.export_materials = true;
            options.export_uv_mapping = true;
            options.mesh_resolution = 1.0f;

            // Export the terrain
            bool success = terrainLoader->SaveTerrain(fileName.toStdString(), options, progressCallback);

            if (success) {
                m_statusLabel->setText(QString("Exported terrain to: %1").arg(QFileInfo(fileName).fileName()));
                QMessageBox::information(this, "Export Complete",
                    QString("Terrain successfully exported to:\n%1").arg(fileName));
                std::cout << "TerrainPainterDialog: Successfully exported terrain to " << fileName.toStdString() << std::endl;
            } else {
                QString errorMsg = QString::fromStdString(terrainLoader->GetLastError());
                m_statusLabel->setText("Export failed: " + errorMsg);
                QMessageBox::warning(this, "Export Error", "Failed to export terrain to OBJ:\n" + errorMsg);
            }
        }
        catch (const std::exception& e) {
            QString errorMsg = QString("Exception during terrain export: %1").arg(e.what());
            m_statusLabel->setText("Export failed");
            QMessageBox::critical(this, "Export Error", errorMsg);
            std::cout << "TerrainPainterDialog: " << errorMsg.toStdString() << std::endl;
        }
    }
}

void TerrainPainterDialog::onResetView() {
    if (m_viewport) {
        m_viewport->resetCamera();
    }
}

void TerrainPainterDialog::onShowGrid(bool show) {
    // Will be implemented in next phase
}

void TerrainPainterDialog::onShowWireframe(bool show) {
    // Will be implemented in next phase
}

void TerrainPainterDialog::onEnableCollision(bool enable) {
    // Will be implemented in next phase
}

void TerrainPainterDialog::onToolChanged() {
    int toolIndex = m_toolCombo->currentIndex();
    m_currentTool = static_cast<TerrainTool>(toolIndex);
    m_viewport->setCurrentTool(m_currentTool);
    updateToolUI();
}

void TerrainPainterDialog::onBrushSettingsChanged() {
    // Update viewport brush settings
    if (m_viewport) {
        m_viewport->setBrushSize(static_cast<float>(m_brushSizeSpin->value()));
        m_viewport->setBrushStrength(static_cast<float>(m_brushStrengthSpin->value()));
        m_viewport->setBrushFalloff(static_cast<float>(m_brushFalloffSpin->value()));

        TerrainBrushShape shape = static_cast<TerrainBrushShape>(m_brushShapeCombo->currentIndex());
        m_viewport->setBrushShape(shape);
    }

    updateBrushPreview();
}

void TerrainPainterDialog::onTerrainSettingsChanged() {
    // Will be implemented in next phase
}

void TerrainPainterDialog::onTextureSelected() {
    int selectedLayer = m_textureList->currentRow();
    if (selectedLayer >= 0 && m_terrainRenderer) {
        // Update UI to reflect selected texture layer
        updateToolUI();
    }
}

void TerrainPainterDialog::addTextureLayer(const QString& texturePath) {
    if (!m_terrainRenderer) return;

    // Add texture layer to terrain renderer
    int layerIndex = m_terrainRenderer->AddTextureLayer(texturePath.toStdString(), 1.0f, 1.0f);

    // Add to UI list
    QFileInfo fileInfo(texturePath);
    QString displayName = QString("Layer %1: %2").arg(layerIndex).arg(fileInfo.baseName());
    m_textureList->addItem(displayName);

    // Select the new layer
    m_textureList->setCurrentRow(layerIndex);

    m_isModified = true;
    updateWindowTitle();
}

void TerrainPainterDialog::removeTextureLayer(int layerIndex) {
    if (!m_terrainRenderer || layerIndex < 0 || layerIndex >= m_textureList->count()) return;

    // Remove from terrain renderer
    m_terrainRenderer->RemoveTextureLayer(layerIndex);

    // Remove from UI list
    delete m_textureList->takeItem(layerIndex);

    // Update remaining layer names
    for (int i = layerIndex; i < m_textureList->count(); ++i) {
        QListWidgetItem* item = m_textureList->item(i);
        if (item) {
            QString text = item->text();
            // Update layer index in display name
            text.replace(QRegularExpression("Layer \\d+:"), QString("Layer %1:").arg(i));
            item->setText(text);
        }
    }

    m_isModified = true;
    updateWindowTitle();
}

int TerrainPainterDialog::getSelectedTextureLayer() const {
    if (!m_textureList) {
        std::cout << "TerrainPainterDialog: Texture list not initialized" << std::endl;
        return 0;
    }

    int selectedRow = m_textureList->currentRow();
    if (selectedRow < 0) {
        // No selection, return 0 for first layer
        return 0;
    }

    return selectedRow;
}

QString TerrainPainterDialog::getSelectedAssetPath() const {
    return m_selectedAssetPath;
}

float TerrainPainterDialog::getScatterDensity() const {
    if (m_scatterDensitySlider) {
        return static_cast<float>(m_scatterDensitySlider->value()) / 100.0f;
    }
    return 1.0f;
}

float TerrainPainterDialog::getScatterScaleVariance() const {
    if (m_scatterScaleVarianceSlider) {
        return static_cast<float>(m_scatterScaleVarianceSlider->value()) / 100.0f;
    }
    return 0.2f;
}

float TerrainPainterDialog::getScatterRotationVariance() const {
    if (m_scatterRotationVarianceSlider) {
        return static_cast<float>(m_scatterRotationVarianceSlider->value()) / 100.0f * 2.0f * 3.14159f; // Convert to radians
    }
    return 1.0f;
}

glm::vec2 TerrainPainterDialog::getScatterHeightOffset() const {
    if (m_scatterHeightOffsetSlider) {
        float offset = static_cast<float>(m_scatterHeightOffsetSlider->value()) / 100.0f;
        return glm::vec2(-offset, offset);
    }
    return glm::vec2(-0.1f, 0.1f);
}

void TerrainPainterDialog::onAssetSelected(const QString& assetPath) {
    if (assetPath.isEmpty()) return;

    // Store the selected asset for scattering
    m_selectedAssetPath = assetPath;

    // Update status to show selected asset
    m_statusLabel->setText(QString("Selected asset: %1").arg(QFileInfo(assetPath).baseName()));

    // If scatter mode is enabled and asset scatter tool is selected, we're ready to scatter
    if (m_scatterModeCheck && m_scatterModeCheck->isChecked() &&
        m_currentTool == TerrainTool::AssetScatter) {
        m_statusLabel->setText(QString("Ready to scatter: %1").arg(QFileInfo(assetPath).baseName()));
    }

    std::cout << "TerrainPainterDialog: Selected asset for scattering: "
              << assetPath.toStdString() << std::endl;
}

void TerrainPainterDialog::onTerrainModified() {
    m_isModified = true;
    updateWindowTitle();
    updateTerrainStats();
}

void TerrainPainterDialog::updateToolUI() {
    // Update UI based on current tool
    // Will be expanded in next phase
}

void TerrainPainterDialog::updateBrushPreview() {
    // Update brush preview
    // Will be implemented in next phase
}

void TerrainPainterDialog::updateTerrainStats() {
    if (!m_terrainData) {
        if (m_terrainStatsLabel) {
            m_terrainStatsLabel->setText("No terrain loaded");
        }
        if (m_terrainStatsDisplay) {
            m_terrainStatsDisplay->setText("No terrain loaded");
        }
        return;
    }

    // Calculate terrain statistics
    auto bounds = m_terrainData->GetBounds();
    glm::vec2 dimensions = m_terrainData->GetDimensions();
    float resolution = m_terrainData->GetResolution();
    auto chunks = m_terrainData->GetAllChunks();

    // Calculate height range
    float minHeight = std::numeric_limits<float>::max();
    float maxHeight = std::numeric_limits<float>::lowest();
    int totalVertices = 0;

    for (auto* chunk : chunks) {
        if (chunk) {
            glm::ivec2 heightmapSize = chunk->GetHeightMapSize();
            totalVertices += heightmapSize.x * heightmapSize.y;

            for (int z = 0; z < heightmapSize.y; ++z) {
                for (int x = 0; x < heightmapSize.x; ++x) {
                    float height = chunk->GetHeight(x, z);
                    minHeight = std::min(minHeight, height);
                    maxHeight = std::max(maxHeight, height);
                }
            }
        }
    }

    // Format statistics
    QString stats = QString(
        "Dimensions: %1 x %2 units\n"
        "Resolution: %3 units/sample\n"
        "Chunks: %4\n"
        "Vertices: %5\n"
        "Height Range: %6 to %7 units"
    ).arg(dimensions.x)
     .arg(dimensions.y)
     .arg(resolution)
     .arg(chunks.size())
     .arg(totalVertices)
     .arg(minHeight, 0, 'f', 2)
     .arg(maxHeight, 0, 'f', 2);

    // Update status bar
    if (m_terrainStatsLabel) {
        m_terrainStatsLabel->setText(QString("Terrain: %1x%2, %3 chunks")
                                    .arg(dimensions.x)
                                    .arg(dimensions.y)
                                    .arg(chunks.size()));
    }

    // Update detailed stats display
    if (m_terrainStatsDisplay) {
        m_terrainStatsDisplay->setText(stats);
    }
}

// File operation placeholders
void TerrainPainterDialog::newTerrain() {
    onNewTerrain();
}

void TerrainPainterDialog::openTerrain() {
    onOpenTerrain();
}

void TerrainPainterDialog::saveTerrain() {
    onSaveTerrain();
}

void TerrainPainterDialog::saveTerrainAs() {
    onSaveTerrainAs();
}

void TerrainPainterDialog::exportToOBJ() {
    onExportOBJ();
}

void TerrainPainterDialog::createTerrainChunk() {
    try {
        // Get terrain parameters from UI
        float width = static_cast<float>(m_terrainWidthSpin->value());
        float height = static_cast<float>(m_terrainHeightSpin->value());
        float resolution = static_cast<float>(m_terrainScaleSpin->value());
        float chunkSize = static_cast<float>(m_chunkSizeSpin->value());

        // Create new terrain data with custom parameters
        m_terrainData = std::make_shared<Lupine::TerrainData>(width, height, resolution, chunkSize);

        // Calculate number of chunks needed
        int chunkCountX = static_cast<int>(std::ceil(width / chunkSize));
        int chunkCountZ = static_cast<int>(std::ceil(height / chunkSize));

        // Initialize with flat terrain
        m_terrainData->InitializeFlatTerrain(0.0f, chunkCountX, chunkCountZ);

        // Create terrain node and renderer if not exists
        if (!m_terrainNode) {
            // Ensure we have a root node
            if (!m_terrainScene->GetRootNode()) {
                m_terrainScene->CreateRootNode("Root");
            }

            m_terrainNode = m_terrainScene->GetRootNode()->CreateChild<Lupine::Node3D>("Terrain");
            if (!m_terrainNode) {
                std::cout << "TerrainPainterDialog: Failed to create terrain node" << std::endl;
                return;
            }

            m_terrainRenderer = m_terrainNode->AddComponent<Lupine::TerrainRenderer>();
            if (!m_terrainRenderer) {
                std::cout << "TerrainPainterDialog: Failed to create terrain renderer" << std::endl;
                return;
            }
        }

        // Set terrain data
        m_terrainRenderer->SetTerrainData(m_terrainData);
        m_viewport->setTerrain(m_terrainRenderer);

        // Update UI
        m_isModified = false;
        m_currentFilePath.clear();
        updateWindowTitle();
        updateTerrainStats();

        // Update status
        m_statusLabel->setText(QString("Created terrain: %1x%2 units, %3x%4 chunks")
                              .arg(width).arg(height).arg(chunkCountX).arg(chunkCountZ));

        std::cout << "TerrainPainterDialog: Created custom terrain successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "TerrainPainterDialog: Error creating terrain: " << e.what() << std::endl;
        m_statusLabel->setText("Error creating terrain");
    }
}

void TerrainPainterDialog::deleteTerrainChunk() {
    // Will be implemented in next phase
}

void TerrainPainterDialog::resizeTerrain() {
    // Will be implemented in next phase
}

void TerrainPainterDialog::clearTerrain() {
    if (!m_terrainData) {
        m_statusLabel->setText("No terrain to clear");
        return;
    }

    // Confirm with user
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Clear Terrain",
        "Are you sure you want to clear all terrain heights to zero? This action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // Get all chunks and reset their heights
        auto chunks = m_terrainData->GetAllChunks();
        for (auto* chunk : chunks) {
            if (chunk) {
                glm::ivec2 heightmapSize = chunk->GetHeightMapSize();
                for (int z = 0; z < heightmapSize.y; ++z) {
                    for (int x = 0; x < heightmapSize.x; ++x) {
                        chunk->SetHeight(x, z, 0.0f);
                    }
                }
                chunk->SetDirty(true);
            }
        }

        m_isModified = true;
        updateWindowTitle();
        updateTerrainStats();
        m_statusLabel->setText("Terrain cleared");

        std::cout << "TerrainPainterDialog: Terrain cleared successfully" << std::endl;
    }
}

void TerrainPainterDialog::flattenTerrain() {
    if (!m_terrainData) {
        m_statusLabel->setText("No terrain to flatten");
        return;
    }

    // Get height from user
    bool ok;
    double height = QInputDialog::getDouble(this, "Flatten Terrain",
        "Enter height to flatten terrain to:", 0.0, -1000.0, 1000.0, 2, &ok);

    if (ok) {
        // Get all chunks and set their heights
        auto chunks = m_terrainData->GetAllChunks();
        for (auto* chunk : chunks) {
            if (chunk) {
                glm::ivec2 heightmapSize = chunk->GetHeightMapSize();
                for (int z = 0; z < heightmapSize.y; ++z) {
                    for (int x = 0; x < heightmapSize.x; ++x) {
                        chunk->SetHeight(x, z, static_cast<float>(height));
                    }
                }
                chunk->SetDirty(true);
            }
        }

        m_isModified = true;
        updateWindowTitle();
        updateTerrainStats();
        m_statusLabel->setText(QString("Terrain flattened to height %1").arg(height));

        std::cout << "TerrainPainterDialog: Terrain flattened to height " << height << std::endl;
    }
}

void TerrainPainterDialog::generateNoise() {
    if (!m_terrainData) {
        m_statusLabel->setText("No terrain to generate noise on");
        return;
    }

    // Get noise parameters from user
    bool ok;
    double amplitude = QInputDialog::getDouble(this, "Generate Noise",
        "Enter noise amplitude:", 10.0, 0.1, 100.0, 2, &ok);

    if (ok) {
        // Simple noise generation using sine waves
        auto chunks = m_terrainData->GetAllChunks();
        for (auto* chunk : chunks) {
            if (chunk) {
                glm::ivec2 chunkCoords = chunk->GetChunkCoords();
                glm::ivec2 heightmapSize = chunk->GetHeightMapSize();

                for (int z = 0; z < heightmapSize.y; ++z) {
                    for (int x = 0; x < heightmapSize.x; ++x) {
                        // Calculate world position
                        float worldX = chunkCoords.x * 32.0f + x;
                        float worldZ = chunkCoords.y * 32.0f + z;

                        // Generate noise using multiple sine waves
                        float noise = 0.0f;
                        noise += std::sin(worldX * 0.1f) * std::cos(worldZ * 0.1f);
                        noise += std::sin(worldX * 0.05f) * std::cos(worldZ * 0.05f) * 0.5f;
                        noise += std::sin(worldX * 0.2f) * std::cos(worldZ * 0.2f) * 0.25f;

                        float height = noise * static_cast<float>(amplitude);
                        chunk->SetHeight(x, z, height);
                    }
                }
                chunk->SetDirty(true);
            }
        }

        m_isModified = true;
        updateWindowTitle();
        updateTerrainStats();
        m_statusLabel->setText(QString("Generated noise with amplitude %1").arg(amplitude));

        std::cout << "TerrainPainterDialog: Generated noise with amplitude " << amplitude << std::endl;
    }
}

void TerrainPainterDialog::loadTerrainFile(const QString& filePath) {
    try {
        // Create a TerrainLoader to handle the file loading
        auto terrainLoader = std::make_unique<Lupine::TerrainLoader>();

        // Set up progress callback
        auto progressCallback = [this](float progress, const std::string& status) {
            m_statusLabel->setText(QString("Loading: %1 (%2%)").arg(QString::fromStdString(status)).arg(static_cast<int>(progress * 100)));
            QApplication::processEvents(); // Allow UI updates
        };

        // Set up import options
        Lupine::TerrainImportOptions options;
        options.auto_detect_format = true;
        options.height_scale = 1.0f;
        options.world_scale = 1.0f;

        // Load the terrain
        bool success = terrainLoader->LoadTerrain(filePath.toStdString(), options, progressCallback);

        if (success) {
            // Get the loaded terrain data
            auto loadedTerrainData = terrainLoader->GetTerrainData();
            if (loadedTerrainData) {
                // Replace current terrain data
                m_terrainData = loadedTerrainData;

                // Update terrain renderer
                if (m_terrainRenderer) {
                    m_terrainRenderer->SetTerrainData(m_terrainData);
                    m_viewport->setTerrain(m_terrainRenderer);
                }

                // Update UI state
                m_currentFilePath = filePath;
                m_isModified = false;
                updateWindowTitle();
                updateTerrainStats();

                m_statusLabel->setText(QString("Loaded terrain: %1").arg(QFileInfo(filePath).fileName()));
                std::cout << "TerrainPainterDialog: Successfully loaded terrain from " << filePath.toStdString() << std::endl;
            } else {
                m_statusLabel->setText("Error: No terrain data loaded");
                QMessageBox::warning(this, "Load Error", "Failed to load terrain data from file.");
            }
        } else {
            QString errorMsg = QString::fromStdString(terrainLoader->GetLastError());
            m_statusLabel->setText("Load failed: " + errorMsg);
            QMessageBox::warning(this, "Load Error", "Failed to load terrain file:\n" + errorMsg);
        }
    }
    catch (const std::exception& e) {
        QString errorMsg = QString("Exception during terrain loading: %1").arg(e.what());
        m_statusLabel->setText("Load failed");
        QMessageBox::critical(this, "Load Error", errorMsg);
        std::cout << "TerrainPainterDialog: " << errorMsg.toStdString() << std::endl;
    }
}

void TerrainPainterDialog::saveTerrainFile(const QString& filePath) {
    if (!m_terrainData) {
        m_statusLabel->setText("No terrain to save");
        return;
    }

    try {
        // Create a TerrainLoader to handle the file saving
        auto terrainLoader = std::make_unique<Lupine::TerrainLoader>();
        terrainLoader->SetTerrainData(m_terrainData);

        // Set up progress callback
        auto progressCallback = [this](float progress, const std::string& status) {
            m_statusLabel->setText(QString("Saving: %1 (%2%)").arg(QString::fromStdString(status)).arg(static_cast<int>(progress * 100)));
            QApplication::processEvents(); // Allow UI updates
        };

        // Set up export options based on file extension
        Lupine::TerrainExportOptions options;
        QString extension = QFileInfo(filePath).suffix().toLower();

        if (extension == "terrain") {
            options.format = Lupine::TerrainFileFormat::LupineTerrain;
            options.include_textures = true;
            options.include_assets = true;
            options.compress_data = true;
        } else if (extension == "raw") {
            options.format = Lupine::TerrainFileFormat::Heightmap;
            options.include_textures = false;
            options.include_assets = false;
        } else {
            // Default to Lupine terrain format
            options.format = Lupine::TerrainFileFormat::LupineTerrain;
        }

        // Save the terrain
        bool success = terrainLoader->SaveTerrain(filePath.toStdString(), options, progressCallback);

        if (success) {
            // Update UI state
            m_currentFilePath = filePath;
            m_isModified = false;
            updateWindowTitle();

            m_statusLabel->setText(QString("Saved terrain: %1").arg(QFileInfo(filePath).fileName()));
            std::cout << "TerrainPainterDialog: Successfully saved terrain to " << filePath.toStdString() << std::endl;
        } else {
            QString errorMsg = QString::fromStdString(terrainLoader->GetLastError());
            m_statusLabel->setText("Save failed: " + errorMsg);
            QMessageBox::warning(this, "Save Error", "Failed to save terrain file:\n" + errorMsg);
        }
    }
    catch (const std::exception& e) {
        QString errorMsg = QString("Exception during terrain saving: %1").arg(e.what());
        m_statusLabel->setText("Save failed");
        QMessageBox::critical(this, "Save Error", errorMsg);
        std::cout << "TerrainPainterDialog: " << errorMsg.toStdString() << std::endl;
    }
}

#include "TerrainPainterDialog.moc"
