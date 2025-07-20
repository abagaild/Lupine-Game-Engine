// Include glad before any Qt OpenGL headers to avoid conflicts
#include <glad/glad.h>

#include "Tileset3DEditorDialog.h"
#include "lupine/editor/TileBuilderDialog.h"
#include "lupine/resources/MeshLoader.h"
#include <QApplication>
#include <QMenuBar>
#include <QStandardPaths>
#include <QDir>
#include <QHeaderView>
#include <QDebug>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QMatrix4x4>
#include <QVector3D>
#include <QQuaternion>

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <cmath>

// Tile3DPreviewWidget Implementation
Tile3DPreviewWidget::Tile3DPreviewWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_tile(nullptr)
    , m_camera_distance(5.0f)
    , m_camera_rotation_x(30.0f)
    , m_camera_rotation_y(45.0f)
    , m_mouse_pressed(false)
    , m_shader_program(0)
    , m_grid_shader_program(0)
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
    , m_grid_vao(0)
    , m_grid_vbo(0)
{
    setMinimumSize(300, 300);
}

Tile3DPreviewWidget::~Tile3DPreviewWidget() {
}

void Tile3DPreviewWidget::SetTile(Lupine::Tile3DData* tile) {
    m_tile = tile;
    update();
}

void Tile3DPreviewWidget::ClearTile() {
    m_tile = nullptr;
    update();
}

void Tile3DPreviewWidget::initializeGL() {
    // Initialize OpenGL functions first
    initializeOpenGLFunctions();

    // Initialize GLAD to load OpenGL function pointers
    if (!gladLoadGL()) {
        qDebug() << "Failed to initialize GLAD";
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    setupShaders();
    setupBuffers();
}

void Tile3DPreviewWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    setupCamera();

    // Always draw grid for reference
    drawGrid();

    if (m_tile && m_tile->model_loaded && m_tile->model) {
        renderTile();
    } else {
        // Draw placeholder cube when no tile is loaded
        drawPlaceholderCube();
    }
}

void Tile3DPreviewWidget::resizeGL(int width, int height) {
    glViewport(0, 0, width, height);
}

void Tile3DPreviewWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_mouse_pressed = true;
        m_last_mouse_pos = event->pos();
        emit tileClicked();
    }
    QOpenGLWidget::mousePressEvent(event);
}

void Tile3DPreviewWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_mouse_pressed && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->pos() - m_last_mouse_pos;
        m_camera_rotation_y += delta.x() * 0.5f;
        m_camera_rotation_x += delta.y() * 0.5f;
        
        // Clamp rotation
        m_camera_rotation_x = std::max(-90.0f, std::min(90.0f, m_camera_rotation_x));
        
        m_last_mouse_pos = event->pos();
        update();
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void Tile3DPreviewWidget::wheelEvent(QWheelEvent* event) {
    float delta = event->angleDelta().y() / 120.0f;
    m_camera_distance -= delta * 0.5f;
    m_camera_distance = std::max(1.0f, std::min(20.0f, m_camera_distance));
    update();
}

void Tile3DPreviewWidget::setupCamera() {
    // Camera setup is handled in renderTile method
    // This method is kept for compatibility
}

void Tile3DPreviewWidget::renderTile() {
    if (!m_tile || !m_tile->model_loaded || !m_tile->model || m_shader_program == 0) {
        return;
    }

    glUseProgram(m_shader_program);

    // Set up matrices using QMatrix4x4 for compatibility
    QMatrix4x4 model, view, projection;

    // Model matrix - apply tile's default transform
    model.setToIdentity();
    model.translate(m_tile->default_transform.position.x,
                   m_tile->default_transform.position.y,
                   m_tile->default_transform.position.z);

    // Convert quaternion to Euler angles for QMatrix4x4
    glm::vec3 euler = glm::eulerAngles(m_tile->default_transform.rotation);
    model.rotate(glm::degrees(euler.x), 1, 0, 0);
    model.rotate(glm::degrees(euler.y), 0, 1, 0);
    model.rotate(glm::degrees(euler.z), 0, 0, 1);

    model.scale(m_tile->default_transform.scale.x,
               m_tile->default_transform.scale.y,
               m_tile->default_transform.scale.z);

    // View matrix (camera)
    view.setToIdentity();
    view.translate(0, 0, -m_camera_distance);
    view.rotate(m_camera_rotation_x, 1, 0, 0);
    view.rotate(m_camera_rotation_y, 0, 1, 0);

    // Projection matrix
    projection.setToIdentity();
    float aspect = static_cast<float>(width()) / static_cast<float>(height());
    projection.perspective(45.0f, aspect, 0.1f, 100.0f);

    // Set uniforms
    int model_loc = glGetUniformLocation(m_shader_program, "model");
    int view_loc = glGetUniformLocation(m_shader_program, "view");
    int proj_loc = glGetUniformLocation(m_shader_program, "projection");
    int color_loc = glGetUniformLocation(m_shader_program, "objectColor");

    glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.data());
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.data());
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, projection.data());
    glUniform3f(color_loc, 0.8f, 0.8f, 0.8f); // Light gray color

    // Set lighting uniforms
    int light_pos_loc = glGetUniformLocation(m_shader_program, "lightPos");
    int light_color_loc = glGetUniformLocation(m_shader_program, "lightColor");
    int view_pos_loc = glGetUniformLocation(m_shader_program, "viewPos");

    glUniform3f(light_pos_loc, 5.0f, 5.0f, 5.0f);
    glUniform3f(light_color_loc, 1.0f, 1.0f, 1.0f);
    glUniform3f(view_pos_loc, 0.0f, 0.0f, m_camera_distance);

    // Render the model
    if (m_tile->model && m_tile->model->IsLoaded()) {
        // For now, render a simple cube as placeholder
        // In a full implementation, you would render the actual model mesh
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    glUseProgram(0);
}

void Tile3DPreviewWidget::setupShaders() {
    // Vertex shader for tile preview
    std::string vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 normal;
        out vec3 fragPos;

        void main() {
            normal = mat3(transpose(inverse(model))) * aNormal;
            fragPos = vec3(model * vec4(aPos, 1.0));
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    // Fragment shader with basic lighting
    std::string fragmentShaderSource = R"(
        #version 330 core
        in vec3 normal;
        in vec3 fragPos;

        out vec4 FragColor;

        uniform vec3 objectColor;
        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 viewPos;

        void main() {
            // Ambient lighting
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * lightColor;

            // Diffuse lighting
            vec3 norm = normalize(normal);
            vec3 lightDir = normalize(lightPos - fragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            // Specular lighting
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - fragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * lightColor;

            vec3 result = (ambient + diffuse + specular) * objectColor;
            FragColor = vec4(result, 1.0);
        }
    )";

    // Compile vertex shader
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
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fShaderCode = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);

    // Check for fragment shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        qCritical() << "Fragment shader compilation failed:" << infoLog;
    }

    // Create shader program
    m_shader_program = glCreateProgram();
    glAttachShader(m_shader_program, vertexShader);
    glAttachShader(m_shader_program, fragmentShader);
    glLinkProgram(m_shader_program);

    // Check for linking errors
    glGetProgramiv(m_shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_shader_program, 512, NULL, infoLog);
        qCritical() << "Shader program linking failed:" << infoLog;
    }

    // Clean up main shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Grid shader sources
    std::string gridVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;

        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";

    std::string gridFragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        uniform vec3 gridColor;

        void main() {
            FragColor = vec4(gridColor, 1.0);
        }
    )";

    // Compile grid shaders
    unsigned int gridVertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* gridVShaderCode = gridVertexShaderSource.c_str();
    glShaderSource(gridVertexShader, 1, &gridVShaderCode, NULL);
    glCompileShader(gridVertexShader);

    glGetShaderiv(gridVertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(gridVertexShader, 512, NULL, infoLog);
        qCritical() << "Grid vertex shader compilation failed:" << infoLog;
    }

    unsigned int gridFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* gridFShaderCode = gridFragmentShaderSource.c_str();
    glShaderSource(gridFragmentShader, 1, &gridFShaderCode, NULL);
    glCompileShader(gridFragmentShader);

    glGetShaderiv(gridFragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(gridFragmentShader, 512, NULL, infoLog);
        qCritical() << "Grid fragment shader compilation failed:" << infoLog;
    }

    // Create grid shader program
    m_grid_shader_program = glCreateProgram();
    glAttachShader(m_grid_shader_program, gridVertexShader);
    glAttachShader(m_grid_shader_program, gridFragmentShader);
    glLinkProgram(m_grid_shader_program);

    glGetProgramiv(m_grid_shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_grid_shader_program, 512, NULL, infoLog);
        qCritical() << "Grid shader program linking failed:" << infoLog;
    }

    // Clean up grid shaders
    glDeleteShader(gridVertexShader);
    glDeleteShader(gridFragmentShader);
}

void Tile3DPreviewWidget::setupBuffers() {
    // Generate VAO, VBO for cube rendering (placeholder)
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    // Setup cube geometry
    float vertices[] = {
        // positions          // normals
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // Setup grid geometry
    glGenVertexArrays(1, &m_grid_vao);
    glGenBuffers(1, &m_grid_vbo);

    std::vector<float> gridVertices;
    float gridExtent = 5.0f;
    float gridSize = 1.0f;

    // Generate grid lines
    for (float i = -gridExtent; i <= gridExtent; i += gridSize) {
        // X lines
        gridVertices.push_back(i); gridVertices.push_back(0.0f); gridVertices.push_back(-gridExtent);
        gridVertices.push_back(i); gridVertices.push_back(0.0f); gridVertices.push_back(gridExtent);

        // Z lines
        gridVertices.push_back(-gridExtent); gridVertices.push_back(0.0f); gridVertices.push_back(i);
        gridVertices.push_back(gridExtent); gridVertices.push_back(0.0f); gridVertices.push_back(i);
    }

    glBindVertexArray(m_grid_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_grid_vbo);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_STATIC_DRAW);

    // Position attribute for grid
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Tile3DPreviewWidget::drawGrid() {
    if (m_grid_shader_program == 0 || m_grid_vao == 0) return;

    // Calculate view and projection matrices
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -m_camera_distance));
    view = glm::rotate(view, glm::radians(m_camera_rotation_x), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, glm::radians(m_camera_rotation_y), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                          (float)width() / (float)height(),
                                          0.1f, 100.0f);

    // Use grid shader program
    glUseProgram(m_grid_shader_program);

    // Set uniforms
    GLint viewLoc = glGetUniformLocation(m_grid_shader_program, "view");
    GLint projLoc = glGetUniformLocation(m_grid_shader_program, "projection");
    GLint colorLoc = glGetUniformLocation(m_grid_shader_program, "gridColor");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(colorLoc, 0.3f, 0.3f, 0.3f); // Dark gray

    // Disable depth testing for grid to ensure it's always visible
    glDisable(GL_DEPTH_TEST);

    // Draw grid
    glBindVertexArray(m_grid_vao);

    // Calculate number of lines
    float gridExtent = 5.0f;
    float gridSize = 1.0f;
    int numGridLines = (int)((2.0f * gridExtent) / gridSize) + 1;
    int numVertices = numGridLines * 4; // 2 directions * 2 vertices per line

    glDrawArrays(GL_LINES, 0, numVertices);
    glBindVertexArray(0);

    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);

    glUseProgram(0);
}

void Tile3DPreviewWidget::drawPlaceholderCube() {
    if (m_shader_program == 0 || m_vao == 0) return;

    // Calculate matrices
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.5f)); // Make it smaller

    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -m_camera_distance));
    view = glm::rotate(view, glm::radians(m_camera_rotation_x), glm::vec3(1.0f, 0.0f, 0.0f));
    view = glm::rotate(view, glm::radians(m_camera_rotation_y), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                          (float)width() / (float)height(),
                                          0.1f, 100.0f);

    // Use main shader program
    glUseProgram(m_shader_program);

    // Set uniforms
    GLint modelLoc = glGetUniformLocation(m_shader_program, "model");
    GLint viewLoc = glGetUniformLocation(m_shader_program, "view");
    GLint projLoc = glGetUniformLocation(m_shader_program, "projection");
    GLint colorLoc = glGetUniformLocation(m_shader_program, "objectColor");
    GLint lightPosLoc = glGetUniformLocation(m_shader_program, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(m_shader_program, "lightColor");
    GLint viewPosLoc = glGetUniformLocation(m_shader_program, "viewPos");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(colorLoc, 0.7f, 0.7f, 0.7f); // Light gray placeholder

    // Set lighting
    glm::vec3 lightPos(2.0f, 2.0f, 2.0f);
    glm::vec3 viewPos(0.0f, 0.0f, m_camera_distance);
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));

    // Draw cube
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glUseProgram(0);
}

// Tileset3DEditorDialog Implementation
Tileset3DEditorDialog::Tileset3DEditorDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_mainSplitter(nullptr)
    , m_tileset(std::make_unique<Lupine::Tileset3DResource>())
    , m_modified(false)
    , m_currentTileId(-1)
{
    setWindowTitle("Tileset 3D Editor");
    setMinimumSize(1400, 900);
    resize(1600, 1000);

    setupUI();
    updateWindowTitle();
}

Tileset3DEditorDialog::~Tileset3DEditorDialog() {
}

void Tileset3DEditorDialog::NewTileset() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    m_tileset = std::make_unique<Lupine::Tileset3DResource>();
    m_currentFilePath.clear();
    m_currentTileId = -1;
    setModified(false);
    
    // Reset UI
    m_tilesetNameEdit->clear();
    m_tilesetDescriptionEdit->clear();
    updateTileList();
    updateCategoryList();
    updateTileProperties();
    updateWindowTitle();
}

void Tileset3DEditorDialog::LoadTileset(const QString& filepath) {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    auto newTileset = std::make_unique<Lupine::Tileset3DResource>();
    if (newTileset->LoadFromFile(filepath.toStdString())) {
        m_tileset = std::move(newTileset);
        m_currentFilePath = filepath;
        m_currentTileId = -1;
        setModified(false);
        
        // Update UI with loaded data
        m_tilesetNameEdit->setText(QString::fromStdString(m_tileset->GetName()));
        m_tilesetDescriptionEdit->setPlainText(QString::fromStdString(m_tileset->GetDescription()));
        
        updateTileList();
        updateCategoryList();
        updateTileProperties();
        updateWindowTitle();
        
        QMessageBox::information(this, "Success", "Tileset 3D loaded successfully!");
    } else {
        QMessageBox::critical(this, "Error", "Failed to load tileset 3D file!");
    }
}

void Tileset3DEditorDialog::SaveTileset() {
    if (m_currentFilePath.isEmpty()) {
        SaveTilesetAs();
        return;
    }

    if (m_tileset->SaveToFile(m_currentFilePath.toStdString())) {
        setModified(false);
        updateWindowTitle();
        QMessageBox::information(this, "Success", "Tileset 3D saved successfully!");
    } else {
        QMessageBox::critical(this, "Error", "Failed to save tileset 3D file!");
    }
}

void Tileset3DEditorDialog::SaveTilesetAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Tileset 3D",
        QDir::currentPath(),
        "Tileset 3D Files (*.tileset3d);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        if (!filepath.endsWith(".tileset3d")) {
            filepath += ".tileset3d";
        }
        
        m_currentFilePath = filepath;
        SaveTileset();
    }
}

// Slot implementations
void Tileset3DEditorDialog::onNewTileset() {
    NewTileset();
}

void Tileset3DEditorDialog::onLoadTileset() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Load Tileset 3D",
        QDir::currentPath(),
        "Tileset 3D Files (*.tileset3d);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        LoadTileset(filepath);
    }
}

void Tileset3DEditorDialog::onSaveTileset() {
    SaveTileset();
}

void Tileset3DEditorDialog::onSaveAs() {
    SaveTilesetAs();
}

void Tileset3DEditorDialog::onTilesetNameChanged() {
    m_tileset->SetName(m_tilesetNameEdit->text().toStdString());
    setModified(true);
}

void Tileset3DEditorDialog::onTilesetDescriptionChanged() {
    m_tileset->SetDescription(m_tilesetDescriptionEdit->toPlainText().toStdString());
    setModified(true);
}

void Tileset3DEditorDialog::onImportTile() {
    QStringList supportedExtensions;
    for (const std::string& ext : Lupine::MeshLoader::GetSupportedExtensions()) {
        supportedExtensions << QString("*%1").arg(QString::fromStdString(ext));
    }
    
    QString filter = QString("3D Model Files (%1);;All Files (*)").arg(supportedExtensions.join(" "));
    
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Import 3D Tile",
        QDir::currentPath(),
        filter
    );

    if (!filepath.isEmpty()) {
        // Create new tile
        int tileId = m_tileset->GetNextTileId();
        QFileInfo fileInfo(filepath);
        QString tileName = fileInfo.baseName();
        
        Lupine::Tile3DData tile(tileId, tileName.toStdString());
        tile.mesh_path = filepath.toStdString();
        
        m_tileset->AddTile(tile);
        updateTileList();
        setModified(true);
        
        // Select the new tile
        for (int i = 0; i < m_tileList->count(); ++i) {
            QListWidgetItem* item = m_tileList->item(i);
            if (item->data(Qt::UserRole).toInt() == tileId) {
                m_tileList->setCurrentItem(item);
                break;
            }
        }
    }
}

void Tileset3DEditorDialog::onRemoveTile() {
    int tileId = getSelectedTileId();
    if (tileId >= 0) {
        m_tileset->RemoveTile(tileId);
        updateTileList();
        updateTileProperties();
        setModified(true);
    }
}

void Tileset3DEditorDialog::onDuplicateTile() {
    int tileId = getSelectedTileId();
    if (tileId >= 0) {
        const Lupine::Tile3DData* originalTile = m_tileset->GetTile(tileId);
        if (originalTile) {
            int newTileId = m_tileset->GetNextTileId();
            Lupine::Tile3DData newTile = *originalTile;
            newTile.id = newTileId;
            newTile.name += " (Copy)";
            
            m_tileset->AddTile(newTile);
            updateTileList();
            setModified(true);
        }
    }
}

void Tileset3DEditorDialog::onTileSelectionChanged() {
    m_currentTileId = getSelectedTileId();
    updateTileProperties();
    updateTilePreview();
}

void Tileset3DEditorDialog::onTileDoubleClicked(QListWidgetItem* item) {
    // Could open a detailed tile editor here
    onTileSelectionChanged();
}

void Tileset3DEditorDialog::onOpenTileBuilder() {
    // Create and show the Tile Builder dialog
    Lupine::TileBuilderDialog* tileBuilder = new Lupine::TileBuilderDialog(this);

    // Set the current tileset as the target (convert unique_ptr to shared_ptr)
    if (m_tileset) {
        std::shared_ptr<Lupine::Tileset3DResource> sharedTileset(m_tileset.get(), [](Lupine::Tileset3DResource*){});
        tileBuilder->SetTargetTileset(sharedTileset);
    }

    // Connect signal to refresh tileset when a tile is added
    connect(tileBuilder, &Lupine::TileBuilderDialog::tileAddedToTileset, this, [this](int tile_id) {
        // Load the model for the new tile
        if (m_tileset) {
            m_tileset->LoadTileModel(tile_id);
        }

        // Refresh the tile list and properties
        updateTileList();
        updateTileProperties();
        updateTilePreview();
        setModified(true);

        // Select the newly added tile
        for (int i = 0; i < m_tileList->count(); ++i) {
            QListWidgetItem* item = m_tileList->item(i);
            if (item->data(Qt::UserRole).toInt() == tile_id) {
                m_tileList->setCurrentItem(item);
                break;
            }
        }
    });

    // Show the dialog
    tileBuilder->show();
    tileBuilder->raise();
    tileBuilder->activateWindow();
}

// Category management slots
void Tileset3DEditorDialog::onAddCategory() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Category", "Category Name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        Lupine::Tile3DCategory category(name.toStdString());
        m_tileset->AddCategory(category);
        updateCategoryList();
        setModified(true);
    }
}

void Tileset3DEditorDialog::onRemoveCategory() {
    QString categoryName = getSelectedCategoryName();
    if (!categoryName.isEmpty()) {
        m_tileset->RemoveCategory(categoryName.toStdString());
        updateCategoryList();
        setModified(true);
    }
}

void Tileset3DEditorDialog::onCategorySelectionChanged() {
    // Update UI based on selected category
}

void Tileset3DEditorDialog::onAssignTileToCategory() {
    int tileId = getSelectedTileId();
    QString categoryName = getSelectedCategoryName();
    if (tileId >= 0 && !categoryName.isEmpty()) {
        m_tileset->AddTileToCategory(tileId, categoryName.toStdString());
        setModified(true);
    }
}

void Tileset3DEditorDialog::onRemoveTileFromCategory() {
    int tileId = getSelectedTileId();
    QString categoryName = getSelectedCategoryName();
    if (tileId >= 0 && !categoryName.isEmpty()) {
        m_tileset->RemoveTileFromCategory(tileId, categoryName.toStdString());
        setModified(true);
    }
}

// Tile property slots
void Tileset3DEditorDialog::onTileNameChanged() {
    if (m_currentTileId < 0) return;

    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        tile->name = m_tileNameEdit->text().toStdString();
        updateTileList(); // Refresh to show new name
        setModified(true);
    }
}

void Tileset3DEditorDialog::onTileMeshPathChanged() {
    if (m_currentTileId < 0) return;

    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        tile->mesh_path = m_tileMeshPathEdit->text().toStdString();
        tile->model.reset();
        tile->model_loaded = false;
        updateTilePreview();
        setModified(true);
    }
}

void Tileset3DEditorDialog::onBrowseMeshPath() {
    QStringList supportedExtensions;
    for (const std::string& ext : Lupine::MeshLoader::GetSupportedExtensions()) {
        supportedExtensions << QString("*%1").arg(QString::fromStdString(ext));
    }

    QString filter = QString("3D Model Files (%1);;All Files (*)").arg(supportedExtensions.join(" "));

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Select 3D Model",
        QDir::currentPath(),
        filter
    );

    if (!filepath.isEmpty()) {
        m_tileMeshPathEdit->setText(filepath);
        onTileMeshPathChanged();
    }
}

void Tileset3DEditorDialog::onTilePreviewImageChanged() {
    if (m_currentTileId < 0) return;

    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        tile->preview_image_path = m_tilePreviewImageEdit->text().toStdString();
        setModified(true);
    }
}

void Tileset3DEditorDialog::onBrowsePreviewImage() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Select Preview Image",
        QDir::currentPath(),
        "Image Files (*.png *.jpg *.jpeg *.bmp *.tga);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        m_tilePreviewImageEdit->setText(filepath);
        onTilePreviewImageChanged();
    }
}

// Transform editing slots
void Tileset3DEditorDialog::onTransformPositionChanged() {
    if (m_currentTileId < 0) return;

    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        tile->default_transform.position.x = static_cast<float>(m_positionXSpin->value());
        tile->default_transform.position.y = static_cast<float>(m_positionYSpin->value());
        tile->default_transform.position.z = static_cast<float>(m_positionZSpin->value());
        setModified(true);
    }
}

void Tileset3DEditorDialog::onTransformRotationChanged() {
    if (m_currentTileId < 0) return;

    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        // Convert Euler angles to quaternion
        float x = glm::radians(static_cast<float>(m_rotationXSpin->value()));
        float y = glm::radians(static_cast<float>(m_rotationYSpin->value()));
        float z = glm::radians(static_cast<float>(m_rotationZSpin->value()));
        tile->default_transform.rotation = glm::quat(glm::vec3(x, y, z));
        setModified(true);
    }
}

void Tileset3DEditorDialog::onTransformScaleChanged() {
    if (m_currentTileId < 0) return;

    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        tile->default_transform.scale.x = static_cast<float>(m_scaleXSpin->value());
        tile->default_transform.scale.y = static_cast<float>(m_scaleYSpin->value());
        tile->default_transform.scale.z = static_cast<float>(m_scaleZSpin->value());
        setModified(true);
    }
}

void Tileset3DEditorDialog::onResetTransform() {
    if (m_currentTileId < 0) return;

    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        tile->default_transform = Lupine::Tile3DTransform();
        updateTransformEditor();
        setModified(true);
    }
}

// Collision editing slots
void Tileset3DEditorDialog::onCollisionTypeChanged() {
    if (m_currentTileId < 0) return;

    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        tile->collision.type = static_cast<Lupine::Tile3DCollisionType>(m_collisionTypeCombo->currentIndex());
        updateCollisionEditor();
        setModified(true);
    }
}

void Tileset3DEditorDialog::onCollisionDataChanged() {
    if (m_currentTileId < 0) return;

    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        tile->collision.offset.x = static_cast<float>(m_collisionOffsetXSpin->value());
        tile->collision.offset.y = static_cast<float>(m_collisionOffsetYSpin->value());
        tile->collision.offset.z = static_cast<float>(m_collisionOffsetZSpin->value());
        tile->collision.size.x = static_cast<float>(m_collisionSizeXSpin->value());
        tile->collision.size.y = static_cast<float>(m_collisionSizeYSpin->value());
        tile->collision.size.z = static_cast<float>(m_collisionSizeZSpin->value());
        tile->collision.margin = static_cast<float>(m_collisionMarginSpin->value());
        setModified(true);
    }
}

void Tileset3DEditorDialog::onBrowseCollisionMesh() {
    QStringList supportedExtensions;
    for (const std::string& ext : Lupine::MeshLoader::GetSupportedExtensions()) {
        supportedExtensions << QString("*%1").arg(QString::fromStdString(ext));
    }

    QString filter = QString("3D Model Files (%1);;All Files (*)").arg(supportedExtensions.join(" "));

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Select Collision Mesh",
        QDir::currentPath(),
        filter
    );

    if (!filepath.isEmpty()) {
        m_collisionMeshEdit->setText(filepath);
        if (m_currentTileId >= 0) {
            Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
            if (tile) {
                tile->collision.collision_mesh_path = filepath.toStdString();
                setModified(true);
            }
        }
    }
}

// Custom data editing slots
void Tileset3DEditorDialog::onAddCustomProperty() {
    if (m_currentTileId < 0) return;

    bool ok;
    QString name = QInputDialog::getText(this, "Add Custom Property", "Property Name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
        if (tile) {
            Lupine::Tile3DDataValue value;
            value.type = Lupine::Tile3DDataType::String;
            value.string_value = "Default Value";
            tile->custom_data[name.toStdString()] = value;
            updateCustomDataEditor();
            setModified(true);
        }
    }
}

void Tileset3DEditorDialog::onRemoveCustomProperty() {
    if (m_currentTileId < 0) return;

    QTreeWidgetItem* item = m_customDataTree->currentItem();
    if (!item) return;

    QString propertyName = item->text(0);
    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        tile->custom_data.erase(propertyName.toStdString());
        updateCustomDataEditor();
        setModified(true);
    }
}

void Tileset3DEditorDialog::onCustomPropertyChanged() {
    setModified(true);
}

// Preview slots
void Tileset3DEditorDialog::onTilePreviewClicked() {
    // Handle preview clicks if needed
}

void Tileset3DEditorDialog::onLoadTileModel() {
    if (m_currentTileId >= 0) {
        m_tileset->LoadTileModel(m_currentTileId);
        updateTilePreview();
    }
}

// UI Setup methods
void Tileset3DEditorDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);

    setupMainPanels();
    setupMenuBar();
}

void Tileset3DEditorDialog::setupMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);
    menuBar->setMaximumHeight(50);  // Limit menu bar height to 50 pixels
    m_mainLayout->insertWidget(0, menuBar);

    // File menu
    QMenu* fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&New Tileset 3D", this, &Tileset3DEditorDialog::onNewTileset, QKeySequence::New);
    fileMenu->addAction("&Open Tileset 3D...", this, &Tileset3DEditorDialog::onLoadTileset, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save Tileset 3D", this, &Tileset3DEditorDialog::onSaveTileset, QKeySequence::Save);
    fileMenu->addAction("Save Tileset 3D &As...", this, &Tileset3DEditorDialog::onSaveAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("&Close", this, &QDialog::close, QKeySequence::Close);

    // Tools menu
    QMenu* toolsMenu = menuBar->addMenu("&Tools");
    toolsMenu->addAction("&Tile Builder...", this, &Tileset3DEditorDialog::onOpenTileBuilder);
}

void Tileset3DEditorDialog::setupMainPanels() {
    // Left panel - Tileset properties and management
    m_leftPanel = new QWidget();
    m_leftPanel->setMinimumWidth(350);
    m_leftPanel->setMaximumWidth(400);
    setupTilesetPropertiesPanel();
    setupTileManagementPanel();
    setupCategoryPanel();
    m_mainSplitter->addWidget(m_leftPanel);

    // Center panel - 3D preview
    m_centerPanel = new QWidget();
    m_centerLayout = new QVBoxLayout(m_centerPanel);
    m_centerLayout->setContentsMargins(5, 5, 5, 5);
    m_centerLayout->setSpacing(5);

    QLabel* titleLabel = new QLabel("3D Tile Preview");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; padding: 2px;");
    titleLabel->setMaximumHeight(20);
    m_centerLayout->addWidget(titleLabel);

    m_tilePreview = new Tile3DPreviewWidget();
    m_centerLayout->addWidget(m_tilePreview, 1); // Give it stretch factor of 1

    m_tileInfoLabel = new QLabel("No tile selected");
    m_tileInfoLabel->setStyleSheet("padding: 3px; background-color: #f0f0f0; border: 1px solid #ccc; font-size: 10px;");
    m_tileInfoLabel->setMaximumHeight(25);
    m_centerLayout->addWidget(m_tileInfoLabel);

    m_loadModelButton = new QPushButton("Load Model");
    m_loadModelButton->setMaximumHeight(30);
    m_centerLayout->addWidget(m_loadModelButton);

    m_mainSplitter->addWidget(m_centerPanel);

    // Right panel - Tile properties
    m_rightPanel = new QWidget();
    m_rightPanel->setMinimumWidth(350);
    m_rightPanel->setMaximumWidth(400);
    setupTilePropertiesPanel();
    m_mainSplitter->addWidget(m_rightPanel);

    // Set splitter proportions
    m_mainSplitter->setStretchFactor(0, 0); // Left panel - fixed
    m_mainSplitter->setStretchFactor(1, 1); // Center panel - expandable
    m_mainSplitter->setStretchFactor(2, 0); // Right panel - fixed

    // Connect preview signals
    connect(m_tilePreview, &Tile3DPreviewWidget::tileClicked, this, &Tileset3DEditorDialog::onTilePreviewClicked);
    connect(m_loadModelButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onLoadTileModel);
}

void Tileset3DEditorDialog::setupTilesetPropertiesPanel() {
    m_leftLayout = new QVBoxLayout(m_leftPanel);

    // Tileset properties group
    m_tilesetPropertiesGroup = new QGroupBox("Tileset Properties");
    m_tilesetPropertiesLayout = new QVBoxLayout(m_tilesetPropertiesGroup);

    // Name
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Name:"));
    m_tilesetNameEdit = new QLineEdit();
    nameLayout->addWidget(m_tilesetNameEdit);
    m_tilesetPropertiesLayout->addLayout(nameLayout);

    // Description
    m_tilesetPropertiesLayout->addWidget(new QLabel("Description:"));
    m_tilesetDescriptionEdit = new QTextEdit();
    m_tilesetDescriptionEdit->setMaximumHeight(80);
    m_tilesetPropertiesLayout->addWidget(m_tilesetDescriptionEdit);

    m_leftLayout->addWidget(m_tilesetPropertiesGroup);

    // Connect signals
    connect(m_tilesetNameEdit, &QLineEdit::textChanged, this, &Tileset3DEditorDialog::onTilesetNameChanged);
    connect(m_tilesetDescriptionEdit, &QTextEdit::textChanged, this, &Tileset3DEditorDialog::onTilesetDescriptionChanged);
}

void Tileset3DEditorDialog::setupTileManagementPanel() {
    // Tile management group
    m_tileManagementGroup = new QGroupBox("Tile Management");
    m_tileManagementLayout = new QVBoxLayout(m_tileManagementGroup);

    // Tile list
    m_tileList = new QListWidget();
    m_tileManagementLayout->addWidget(m_tileList);

    // Buttons
    m_tileButtonLayout = new QHBoxLayout();
    m_importTileButton = new QPushButton("Import");
    m_removeTileButton = new QPushButton("Remove");
    m_duplicateTileButton = new QPushButton("Duplicate");
    m_tileButtonLayout->addWidget(m_importTileButton);
    m_tileButtonLayout->addWidget(m_removeTileButton);
    m_tileButtonLayout->addWidget(m_duplicateTileButton);
    m_tileManagementLayout->addLayout(m_tileButtonLayout);

    m_leftLayout->addWidget(m_tileManagementGroup);

    // Connect signals
    connect(m_tileList, &QListWidget::currentItemChanged, this, &Tileset3DEditorDialog::onTileSelectionChanged);
    connect(m_tileList, &QListWidget::itemDoubleClicked, this, &Tileset3DEditorDialog::onTileDoubleClicked);
    connect(m_importTileButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onImportTile);
    connect(m_removeTileButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onRemoveTile);
    connect(m_duplicateTileButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onDuplicateTile);
}

void Tileset3DEditorDialog::setupCategoryPanel() {
    // Category management group
    m_categoryGroup = new QGroupBox("Categories");
    m_categoryLayout = new QVBoxLayout(m_categoryGroup);

    // Category list
    m_categoryList = new QListWidget();
    m_categoryLayout->addWidget(m_categoryList);

    // Buttons
    m_categoryButtonLayout = new QHBoxLayout();
    m_addCategoryButton = new QPushButton("Add");
    m_removeCategoryButton = new QPushButton("Remove");
    m_assignToCategoryButton = new QPushButton("Assign");
    m_removeFromCategoryButton = new QPushButton("Unassign");
    m_categoryButtonLayout->addWidget(m_addCategoryButton);
    m_categoryButtonLayout->addWidget(m_removeCategoryButton);
    m_categoryButtonLayout->addWidget(m_assignToCategoryButton);
    m_categoryButtonLayout->addWidget(m_removeFromCategoryButton);
    m_categoryLayout->addLayout(m_categoryButtonLayout);

    m_leftLayout->addWidget(m_categoryGroup);
    m_leftLayout->addStretch();

    // Connect signals
    connect(m_categoryList, &QListWidget::currentItemChanged, this, &Tileset3DEditorDialog::onCategorySelectionChanged);
    connect(m_addCategoryButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onAddCategory);
    connect(m_removeCategoryButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onRemoveCategory);
    connect(m_assignToCategoryButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onAssignTileToCategory);
    connect(m_removeFromCategoryButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onRemoveTileFromCategory);
}

void Tileset3DEditorDialog::setupTilePropertiesPanel() {
    m_rightLayout = new QVBoxLayout(m_rightPanel);

    // Title
    QLabel* titleLabel = new QLabel("Tile Properties");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_rightLayout->addWidget(titleLabel);

    // Properties tab widget
    m_propertiesTab = new QTabWidget();
    m_rightLayout->addWidget(m_propertiesTab);

    // Setup tabs
    setupTransformEditor();
    setupCollisionEditor();
    setupCustomDataEditor();
}

void Tileset3DEditorDialog::setupTransformEditor() {
    // General tab
    m_generalTab = new QWidget();
    m_generalLayout = new QVBoxLayout(m_generalTab);

    // Tile name
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Name:"));
    m_tileNameEdit = new QLineEdit();
    nameLayout->addWidget(m_tileNameEdit);
    m_generalLayout->addLayout(nameLayout);

    // Mesh path
    QHBoxLayout* meshLayout = new QHBoxLayout();
    meshLayout->addWidget(new QLabel("Mesh:"));
    m_tileMeshPathEdit = new QLineEdit();
    m_browseMeshButton = new QPushButton("Browse...");
    meshLayout->addWidget(m_tileMeshPathEdit);
    meshLayout->addWidget(m_browseMeshButton);
    m_generalLayout->addLayout(meshLayout);

    // Preview image path
    QHBoxLayout* previewLayout = new QHBoxLayout();
    previewLayout->addWidget(new QLabel("Preview:"));
    m_tilePreviewImageEdit = new QLineEdit();
    m_browsePreviewButton = new QPushButton("Browse...");
    previewLayout->addWidget(m_tilePreviewImageEdit);
    previewLayout->addWidget(m_browsePreviewButton);
    m_generalLayout->addLayout(previewLayout);

    m_generalLayout->addStretch();
    m_propertiesTab->addTab(m_generalTab, "General");

    // Transform tab
    m_transformTab = new QWidget();
    m_transformLayout = new QVBoxLayout(m_transformTab);

    // Position group
    m_positionGroup = new QGroupBox("Position");
    QGridLayout* posLayout = new QGridLayout(m_positionGroup);
    posLayout->addWidget(new QLabel("X:"), 0, 0);
    m_positionXSpin = new QDoubleSpinBox();
    m_positionXSpin->setRange(-1000.0, 1000.0);
    m_positionXSpin->setDecimals(3);
    posLayout->addWidget(m_positionXSpin, 0, 1);
    posLayout->addWidget(new QLabel("Y:"), 1, 0);
    m_positionYSpin = new QDoubleSpinBox();
    m_positionYSpin->setRange(-1000.0, 1000.0);
    m_positionYSpin->setDecimals(3);
    posLayout->addWidget(m_positionYSpin, 1, 1);
    posLayout->addWidget(new QLabel("Z:"), 2, 0);
    m_positionZSpin = new QDoubleSpinBox();
    m_positionZSpin->setRange(-1000.0, 1000.0);
    m_positionZSpin->setDecimals(3);
    posLayout->addWidget(m_positionZSpin, 2, 1);
    m_transformLayout->addWidget(m_positionGroup);

    // Rotation group
    m_rotationGroup = new QGroupBox("Rotation (Degrees)");
    QGridLayout* rotLayout = new QGridLayout(m_rotationGroup);
    rotLayout->addWidget(new QLabel("X:"), 0, 0);
    m_rotationXSpin = new QDoubleSpinBox();
    m_rotationXSpin->setRange(-360.0, 360.0);
    m_rotationXSpin->setDecimals(1);
    rotLayout->addWidget(m_rotationXSpin, 0, 1);
    rotLayout->addWidget(new QLabel("Y:"), 1, 0);
    m_rotationYSpin = new QDoubleSpinBox();
    m_rotationYSpin->setRange(-360.0, 360.0);
    m_rotationYSpin->setDecimals(1);
    rotLayout->addWidget(m_rotationYSpin, 1, 1);
    rotLayout->addWidget(new QLabel("Z:"), 2, 0);
    m_rotationZSpin = new QDoubleSpinBox();
    m_rotationZSpin->setRange(-360.0, 360.0);
    m_rotationZSpin->setDecimals(1);
    rotLayout->addWidget(m_rotationZSpin, 2, 1);
    m_transformLayout->addWidget(m_rotationGroup);

    // Scale group
    m_scaleGroup = new QGroupBox("Scale");
    QGridLayout* scaleLayout = new QGridLayout(m_scaleGroup);
    scaleLayout->addWidget(new QLabel("X:"), 0, 0);
    m_scaleXSpin = new QDoubleSpinBox();
    m_scaleXSpin->setRange(0.001, 1000.0);
    m_scaleXSpin->setDecimals(3);
    m_scaleXSpin->setValue(1.0);
    scaleLayout->addWidget(m_scaleXSpin, 0, 1);
    scaleLayout->addWidget(new QLabel("Y:"), 1, 0);
    m_scaleYSpin = new QDoubleSpinBox();
    m_scaleYSpin->setRange(0.001, 1000.0);
    m_scaleYSpin->setDecimals(3);
    m_scaleYSpin->setValue(1.0);
    scaleLayout->addWidget(m_scaleYSpin, 1, 1);
    scaleLayout->addWidget(new QLabel("Z:"), 2, 0);
    m_scaleZSpin = new QDoubleSpinBox();
    m_scaleZSpin->setRange(0.001, 1000.0);
    m_scaleZSpin->setDecimals(3);
    m_scaleZSpin->setValue(1.0);
    scaleLayout->addWidget(m_scaleZSpin, 2, 1);
    m_transformLayout->addWidget(m_scaleGroup);

    // Reset button
    m_resetTransformButton = new QPushButton("Reset Transform");
    m_transformLayout->addWidget(m_resetTransformButton);
    m_transformLayout->addStretch();

    m_propertiesTab->addTab(m_transformTab, "Transform");

    // Connect general tab signals
    connect(m_tileNameEdit, &QLineEdit::textChanged, this, &Tileset3DEditorDialog::onTileNameChanged);
    connect(m_tileMeshPathEdit, &QLineEdit::textChanged, this, &Tileset3DEditorDialog::onTileMeshPathChanged);
    connect(m_browseMeshButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onBrowseMeshPath);
    connect(m_tilePreviewImageEdit, &QLineEdit::textChanged, this, &Tileset3DEditorDialog::onTilePreviewImageChanged);
    connect(m_browsePreviewButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onBrowsePreviewImage);

    // Connect transform signals
    connect(m_positionXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onTransformPositionChanged);
    connect(m_positionYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onTransformPositionChanged);
    connect(m_positionZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onTransformPositionChanged);
    connect(m_rotationXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onTransformRotationChanged);
    connect(m_rotationYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onTransformRotationChanged);
    connect(m_rotationZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onTransformRotationChanged);
    connect(m_scaleXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onTransformScaleChanged);
    connect(m_scaleYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onTransformScaleChanged);
    connect(m_scaleZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onTransformScaleChanged);
    connect(m_resetTransformButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onResetTransform);
}

void Tileset3DEditorDialog::setupCollisionEditor() {
    m_collisionTab = new QWidget();
    m_collisionLayout = new QVBoxLayout(m_collisionTab);

    // Collision type
    QHBoxLayout* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Collision Type:"));
    m_collisionTypeCombo = new QComboBox();
    m_collisionTypeCombo->addItems({"None", "Box", "Sphere", "Mesh", "Convex Hull", "Custom"});
    typeLayout->addWidget(m_collisionTypeCombo);
    m_collisionLayout->addLayout(typeLayout);

    // Collision data group
    m_collisionDataGroup = new QGroupBox("Collision Data");
    m_collisionDataLayout = new QGridLayout(m_collisionDataGroup);

    // Offset
    m_collisionDataLayout->addWidget(new QLabel("Offset:"), 0, 0);
    m_collisionOffsetXSpin = new QDoubleSpinBox();
    m_collisionOffsetXSpin->setRange(-1000.0, 1000.0);
    m_collisionOffsetXSpin->setDecimals(3);
    m_collisionOffsetYSpin = new QDoubleSpinBox();
    m_collisionOffsetYSpin->setRange(-1000.0, 1000.0);
    m_collisionOffsetYSpin->setDecimals(3);
    m_collisionOffsetZSpin = new QDoubleSpinBox();
    m_collisionOffsetZSpin->setRange(-1000.0, 1000.0);
    m_collisionOffsetZSpin->setDecimals(3);
    QHBoxLayout* offsetLayout = new QHBoxLayout();
    offsetLayout->addWidget(m_collisionOffsetXSpin);
    offsetLayout->addWidget(m_collisionOffsetYSpin);
    offsetLayout->addWidget(m_collisionOffsetZSpin);
    m_collisionDataLayout->addLayout(offsetLayout, 0, 1);

    // Size
    m_collisionDataLayout->addWidget(new QLabel("Size:"), 1, 0);
    m_collisionSizeXSpin = new QDoubleSpinBox();
    m_collisionSizeXSpin->setRange(0.001, 1000.0);
    m_collisionSizeXSpin->setDecimals(3);
    m_collisionSizeXSpin->setValue(1.0);
    m_collisionSizeYSpin = new QDoubleSpinBox();
    m_collisionSizeYSpin->setRange(0.001, 1000.0);
    m_collisionSizeYSpin->setDecimals(3);
    m_collisionSizeYSpin->setValue(1.0);
    m_collisionSizeZSpin = new QDoubleSpinBox();
    m_collisionSizeZSpin->setRange(0.001, 1000.0);
    m_collisionSizeZSpin->setDecimals(3);
    m_collisionSizeZSpin->setValue(1.0);
    QHBoxLayout* sizeLayout = new QHBoxLayout();
    sizeLayout->addWidget(m_collisionSizeXSpin);
    sizeLayout->addWidget(m_collisionSizeYSpin);
    sizeLayout->addWidget(m_collisionSizeZSpin);
    m_collisionDataLayout->addLayout(sizeLayout, 1, 1);

    // Collision mesh
    m_collisionDataLayout->addWidget(new QLabel("Collision Mesh:"), 2, 0);
    QHBoxLayout* meshLayout = new QHBoxLayout();
    m_collisionMeshEdit = new QLineEdit();
    m_browseCollisionMeshButton = new QPushButton("Browse...");
    meshLayout->addWidget(m_collisionMeshEdit);
    meshLayout->addWidget(m_browseCollisionMeshButton);
    m_collisionDataLayout->addLayout(meshLayout, 2, 1);

    // Margin
    m_collisionDataLayout->addWidget(new QLabel("Margin:"), 3, 0);
    m_collisionMarginSpin = new QDoubleSpinBox();
    m_collisionMarginSpin->setRange(0.0, 10.0);
    m_collisionMarginSpin->setDecimals(3);
    m_collisionDataLayout->addWidget(m_collisionMarginSpin, 3, 1);

    m_collisionLayout->addWidget(m_collisionDataGroup);
    m_collisionLayout->addStretch();

    m_propertiesTab->addTab(m_collisionTab, "Collision");

    // Connect signals
    connect(m_collisionTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Tileset3DEditorDialog::onCollisionTypeChanged);
    connect(m_collisionOffsetXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onCollisionDataChanged);
    connect(m_collisionOffsetYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onCollisionDataChanged);
    connect(m_collisionOffsetZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onCollisionDataChanged);
    connect(m_collisionSizeXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onCollisionDataChanged);
    connect(m_collisionSizeYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onCollisionDataChanged);
    connect(m_collisionSizeZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onCollisionDataChanged);
    connect(m_collisionMarginSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Tileset3DEditorDialog::onCollisionDataChanged);
    connect(m_browseCollisionMeshButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onBrowseCollisionMesh);
}

void Tileset3DEditorDialog::setupCustomDataEditor() {
    m_customDataTab = new QWidget();
    m_customDataLayout = new QVBoxLayout(m_customDataTab);

    // Custom data tree
    m_customDataTree = new QTreeWidget();
    m_customDataTree->setHeaderLabels({"Property", "Type", "Value"});
    m_customDataTree->header()->setStretchLastSection(true);
    m_customDataLayout->addWidget(m_customDataTree);

    // Buttons
    m_customDataButtonLayout = new QHBoxLayout();
    m_addPropertyButton = new QPushButton("Add Property");
    m_removePropertyButton = new QPushButton("Remove Property");
    m_customDataButtonLayout->addWidget(m_addPropertyButton);
    m_customDataButtonLayout->addWidget(m_removePropertyButton);
    m_customDataButtonLayout->addStretch();
    m_customDataLayout->addLayout(m_customDataButtonLayout);

    m_propertiesTab->addTab(m_customDataTab, "Custom Data");

    // Connect signals
    connect(m_addPropertyButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onAddCustomProperty);
    connect(m_removePropertyButton, &QPushButton::clicked, this, &Tileset3DEditorDialog::onRemoveCustomProperty);
}

// Update methods
void Tileset3DEditorDialog::updateWindowTitle() {
    QString title = "Tileset 3D Editor";
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.fileName();
    } else {
        title += " - Untitled";
    }
    if (m_modified) {
        title += "*";
    }
    setWindowTitle(title);
}

void Tileset3DEditorDialog::updateTileList() {
    m_tileList->clear();

    for (const auto& [tileId, tile] : m_tileset->GetTiles()) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(tile.name));
        item->setData(Qt::UserRole, tileId);
        m_tileList->addItem(item);
    }
}

void Tileset3DEditorDialog::updateCategoryList() {
    m_categoryList->clear();

    for (const auto& [categoryName, category] : m_tileset->GetCategories()) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(category.name));
        item->setData(Qt::UserRole, QString::fromStdString(category.name));
        m_categoryList->addItem(item);
    }
}

void Tileset3DEditorDialog::updateTileProperties() {
    if (m_currentTileId < 0) {
        // Disable all property editors
        m_tileNameEdit->clear();
        m_tileMeshPathEdit->clear();
        m_tilePreviewImageEdit->clear();
        m_tileNameEdit->setEnabled(false);
        m_tileMeshPathEdit->setEnabled(false);
        m_tilePreviewImageEdit->setEnabled(false);
        m_browseMeshButton->setEnabled(false);
        m_browsePreviewButton->setEnabled(false);

        updateTransformEditor();
        updateCollisionEditor();
        updateCustomDataEditor();

        m_tileInfoLabel->setText("No tile selected");
        return;
    }

    const Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (!tile) {
        return;
    }

    // Enable and update property editors
    m_tileNameEdit->setEnabled(true);
    m_tileMeshPathEdit->setEnabled(true);
    m_tilePreviewImageEdit->setEnabled(true);
    m_browseMeshButton->setEnabled(true);
    m_browsePreviewButton->setEnabled(true);

    m_tileNameEdit->setText(QString::fromStdString(tile->name));
    m_tileMeshPathEdit->setText(QString::fromStdString(tile->mesh_path));
    m_tilePreviewImageEdit->setText(QString::fromStdString(tile->preview_image_path));

    updateTransformEditor();
    updateCollisionEditor();
    updateCustomDataEditor();

    m_tileInfoLabel->setText(QString("Selected Tile: %1 (ID: %2)")
        .arg(QString::fromStdString(tile->name))
        .arg(tile->id));
}

void Tileset3DEditorDialog::updateTransformEditor() {
    if (m_currentTileId < 0) {
        m_positionXSpin->setEnabled(false);
        m_positionYSpin->setEnabled(false);
        m_positionZSpin->setEnabled(false);
        m_rotationXSpin->setEnabled(false);
        m_rotationYSpin->setEnabled(false);
        m_rotationZSpin->setEnabled(false);
        m_scaleXSpin->setEnabled(false);
        m_scaleYSpin->setEnabled(false);
        m_scaleZSpin->setEnabled(false);
        m_resetTransformButton->setEnabled(false);
        return;
    }

    const Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (!tile) return;

    m_positionXSpin->setEnabled(true);
    m_positionYSpin->setEnabled(true);
    m_positionZSpin->setEnabled(true);
    m_rotationXSpin->setEnabled(true);
    m_rotationYSpin->setEnabled(true);
    m_rotationZSpin->setEnabled(true);
    m_scaleXSpin->setEnabled(true);
    m_scaleYSpin->setEnabled(true);
    m_scaleZSpin->setEnabled(true);
    m_resetTransformButton->setEnabled(true);

    // Update values
    m_positionXSpin->setValue(tile->default_transform.position.x);
    m_positionYSpin->setValue(tile->default_transform.position.y);
    m_positionZSpin->setValue(tile->default_transform.position.z);

    // Convert quaternion to Euler angles for display
    glm::vec3 euler = glm::degrees(glm::eulerAngles(tile->default_transform.rotation));
    m_rotationXSpin->setValue(euler.x);
    m_rotationYSpin->setValue(euler.y);
    m_rotationZSpin->setValue(euler.z);

    m_scaleXSpin->setValue(tile->default_transform.scale.x);
    m_scaleYSpin->setValue(tile->default_transform.scale.y);
    m_scaleZSpin->setValue(tile->default_transform.scale.z);
}

void Tileset3DEditorDialog::updateCollisionEditor() {
    if (m_currentTileId < 0) {
        m_collisionTypeCombo->setEnabled(false);
        m_collisionDataGroup->setEnabled(false);
        return;
    }

    const Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (!tile) return;

    m_collisionTypeCombo->setEnabled(true);
    m_collisionDataGroup->setEnabled(true);

    // Update collision type
    m_collisionTypeCombo->setCurrentIndex(static_cast<int>(tile->collision.type));

    // Update collision data
    m_collisionOffsetXSpin->setValue(tile->collision.offset.x);
    m_collisionOffsetYSpin->setValue(tile->collision.offset.y);
    m_collisionOffsetZSpin->setValue(tile->collision.offset.z);
    m_collisionSizeXSpin->setValue(tile->collision.size.x);
    m_collisionSizeYSpin->setValue(tile->collision.size.y);
    m_collisionSizeZSpin->setValue(tile->collision.size.z);
    m_collisionMeshEdit->setText(QString::fromStdString(tile->collision.collision_mesh_path));
    m_collisionMarginSpin->setValue(tile->collision.margin);

    // Enable/disable collision data based on type
    bool hasCollisionData = (tile->collision.type != Lupine::Tile3DCollisionType::None);
    m_collisionDataGroup->setEnabled(hasCollisionData);
}

void Tileset3DEditorDialog::updateCustomDataEditor() {
    m_customDataTree->clear();

    if (m_currentTileId < 0) {
        m_addPropertyButton->setEnabled(false);
        m_removePropertyButton->setEnabled(false);
        return;
    }

    const Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (!tile) {
        m_addPropertyButton->setEnabled(false);
        m_removePropertyButton->setEnabled(false);
        return;
    }

    m_addPropertyButton->setEnabled(true);
    m_removePropertyButton->setEnabled(true);

    // Populate custom data tree
    for (const auto& pair : tile->custom_data) {
        QTreeWidgetItem* item = new QTreeWidgetItem(m_customDataTree);
        item->setText(0, QString::fromStdString(pair.first));

        const Lupine::Tile3DDataValue& value = pair.second;
        QString typeStr;
        QString valueStr;

        switch (value.type) {
            case Lupine::Tile3DDataType::String:
                typeStr = "String";
                valueStr = QString::fromStdString(value.string_value);
                break;
            case Lupine::Tile3DDataType::Integer:
                typeStr = "Integer";
                valueStr = QString::number(value.int_value);
                break;
            case Lupine::Tile3DDataType::Float:
                typeStr = "Float";
                valueStr = QString::number(value.float_value);
                break;
            case Lupine::Tile3DDataType::Boolean:
                typeStr = "Boolean";
                valueStr = value.bool_value ? "true" : "false";
                break;
            case Lupine::Tile3DDataType::Vector3:
                typeStr = "Vector3";
                valueStr = QString("(%1, %2, %3)")
                    .arg(value.vec3_value.x)
                    .arg(value.vec3_value.y)
                    .arg(value.vec3_value.z);
                break;
            case Lupine::Tile3DDataType::Color:
                typeStr = "Color";
                valueStr = QString("(%1, %2, %3, %4)")
                    .arg(value.color_value.x)
                    .arg(value.color_value.y)
                    .arg(value.color_value.z)
                    .arg(value.color_value.w);
                break;
        }

        item->setText(1, typeStr);
        item->setText(2, valueStr);
    }
}

void Tileset3DEditorDialog::updateTilePreview() {
    if (m_currentTileId < 0) {
        m_tilePreview->ClearTile();
        return;
    }

    Lupine::Tile3DData* tile = m_tileset->GetTile(m_currentTileId);
    if (tile) {
        m_tilePreview->SetTile(tile);
    } else {
        m_tilePreview->ClearTile();
    }
}

// Utility functions
bool Tileset3DEditorDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool Tileset3DEditorDialog::promptSaveChanges() {
    if (!hasUnsavedChanges()) {
        return true;
    }

    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<Tileset3DEditorDialog*>(this),
        "Unsaved Changes",
        "The tileset 3D has unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    switch (result) {
        case QMessageBox::Save:
            SaveTileset();
            return !hasUnsavedChanges(); // Return true if save was successful
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
        default:
            return false;
    }
}

void Tileset3DEditorDialog::setModified(bool modified) {
    if (m_modified != modified) {
        m_modified = modified;
        updateWindowTitle();
    }
}

int Tileset3DEditorDialog::getSelectedTileId() const {
    QListWidgetItem* item = m_tileList->currentItem();
    if (item) {
        return item->data(Qt::UserRole).toInt();
    }
    return -1;
}

QString Tileset3DEditorDialog::getSelectedCategoryName() const {
    QListWidgetItem* item = m_categoryList->currentItem();
    if (item) {
        return item->data(Qt::UserRole).toString();
    }
    return QString();
}


