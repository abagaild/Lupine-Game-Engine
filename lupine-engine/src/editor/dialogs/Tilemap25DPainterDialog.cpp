#include <glad/glad.h>
#include "Tilemap25DPainterDialog.h"
#include "lupine/rendering/Camera.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/GraphicsDevice.h"
#include "lupine/resources/ResourceManager.h"

#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QPushButton>
#include <QToolButton>
#include <QStatusBar>
#include <QProgressBar>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QActionGroup>

// Include glad before any Qt OpenGL headers to avoid conflicts
#include <glad/glad.h>
#include <QOpenGLFunctions>
#include <cmath>
#include <algorithm>
#include <set>
#include <iostream>

// Tilemap25DCanvas Implementation
Tilemap25DCanvas::Tilemap25DCanvas(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_current_tool(Tilemap25DPaintTool::Paint)
    , m_selection_mode(Tilemap25DSelectionMode::Face)
    , m_gizmo_mode(Tilemap25DGizmoMode::None)
    , m_current_tileset_id(-1)
    , m_current_tile_id(-1)
    , m_grid_size(1.0f)
    , m_show_grid(true)
    , m_snap_to_grid(true)
    , m_grid_offset(0.0f, 0.0f)
    , m_snap_x_axis(true)
    , m_snap_y_axis(true)
    , m_snap_z_axis(true)
    , m_grid_size_per_axis(1.0f, 1.0f, 1.0f)
    , m_snap_to_edges(false)
    , m_edge_snap_distance(0.5f)
    , m_show_preview(false)
    , m_preview_position(0.0f)
    , m_tile_rotation_y(0.0f)
    , m_tile_rotation_x(0.0f)
    , m_tile_rotation_z(0.0f)
    , m_camera_distance(10.0f)
    , m_camera_rotation_x(30.0f)
    , m_camera_rotation_y(45.0f)
    , m_camera_target(0.0f, 0.0f, 0.0f)
    , m_mouse_pressed(false)
    , m_dragging_gizmo(false)
    , m_is_panning(false)
    , m_is_orbiting(false)
    , m_pressed_button(Qt::NoButton)
    , m_grid_vao(0)
    , m_grid_vbo(0)
    , m_face_vao(0)
    , m_face_vbo(0)
    , m_face_ebo(0)
    , m_gizmo_vao(0)
    , m_gizmo_vbo(0)
    , m_gizmo_position(0.0f)
    , m_gizmo_transform(1.0f)
    , m_gizmo_axis(0)
    , m_atlas_size(1)
    , m_atlas_texture_scale(1.0f)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

Tilemap25DCanvas::~Tilemap25DCanvas() {
    makeCurrent();
    
    // Cleanup OpenGL resources
    if (m_grid_vao) {
        glDeleteVertexArrays(1, &m_grid_vao);
        glDeleteBuffers(1, &m_grid_vbo);
    }
    if (m_face_vao) {
        glDeleteVertexArrays(1, &m_face_vao);
        glDeleteBuffers(1, &m_face_vbo);
        glDeleteBuffers(1, &m_face_ebo);
    }
    if (m_gizmo_vao) {
        glDeleteVertexArrays(1, &m_gizmo_vao);
        glDeleteBuffers(1, &m_gizmo_vbo);
    }
    
    // Cleanup tileset textures
    for (auto& pair : m_tileset_textures) {
        glDeleteTextures(1, &pair.second);
    }
    
    doneCurrent();
}

void Tilemap25DCanvas::SetCurrentTool(Tilemap25DPaintTool tool) {
    m_current_tool = tool;
    update();
}

void Tilemap25DCanvas::SetSelectionMode(Tilemap25DSelectionMode mode) {
    m_selection_mode = mode;
    ClearSelection();
    update();
}

void Tilemap25DCanvas::SetGizmoMode(Tilemap25DGizmoMode mode) {
    m_gizmo_mode = mode;
    updateGizmoTransform();
    update();
}

void Tilemap25DCanvas::SetCurrentTile(int tileset_id, int tile_id) {
    m_current_tileset_id = tileset_id;
    m_current_tile_id = tile_id;
}

void Tilemap25DCanvas::SetGridSize(float size) {
    m_grid_size = std::max(0.1f, size);
    update();
}

void Tilemap25DCanvas::SetShowGrid(bool show) {
    m_show_grid = show;
    update();
}

void Tilemap25DCanvas::SetSnapToGrid(bool snap) {
    m_snap_to_grid = snap;
}

void Tilemap25DCanvas::ShiftGridHorizontal(float offset) {
    m_grid_offset.x += offset;
    update();
}

void Tilemap25DCanvas::ShiftGridVertical(float offset) {
    m_grid_offset.y += offset;
    update();
}

void Tilemap25DCanvas::SetSnapXAxis(bool snap) {
    m_snap_x_axis = snap;
}

void Tilemap25DCanvas::SetSnapYAxis(bool snap) {
    m_snap_y_axis = snap;
}

void Tilemap25DCanvas::SetSnapZAxis(bool snap) {
    m_snap_z_axis = snap;
}

void Tilemap25DCanvas::SetGridSizePerAxis(const glm::vec3& grid_size) {
    m_grid_size_per_axis = grid_size;
    update();
}

void Tilemap25DCanvas::SetSnapToEdges(bool snap) {
    m_snap_to_edges = snap;
}

void Tilemap25DCanvas::SetEdgeSnapDistance(float distance) {
    m_edge_snap_distance = distance;
}

void Tilemap25DCanvas::LoadTileset(int tileset_id, const QString& tileset_path) {
    auto tileset = std::make_unique<Lupine::Tileset2DResource>();
    if (tileset->LoadFromFile(tileset_path.toStdString())) {
        m_tilesets[tileset_id] = std::move(tileset);

        // Load OpenGL texture
        makeCurrent();
        unsigned int texture_id = 0;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);

        // Load texture data from tileset
        const std::string& texture_path = m_tilesets[tileset_id]->GetTexturePath();
        if (!texture_path.empty()) {
            QImage image(QString::fromStdString(texture_path));
            if (!image.isNull()) {
                // Convert to OpenGL format
                QImage gl_image = image.convertToFormat(QImage::Format_RGBA8888);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl_image.width(), gl_image.height(),
                           0, GL_RGBA, GL_UNSIGNED_BYTE, gl_image.bits());

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

                m_tileset_textures[tileset_id] = texture_id;

                std::cout << "Loaded tileset texture: " << texture_path << " (ID: " << texture_id << ")" << std::endl;
            } else {
                std::cerr << "Failed to load tileset image: " << texture_path << std::endl;
                glDeleteTextures(1, &texture_id);
            }
        } else {
            std::cerr << "Tileset has no texture path" << std::endl;
            glDeleteTextures(1, &texture_id);
        }
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        m_tileset_textures[tileset_id] = texture_id;
        doneCurrent();
    }
}

Lupine::Tileset2DResource* Tilemap25DCanvas::GetTileset(int tileset_id) {
    auto it = m_tilesets.find(tileset_id);
    return (it != m_tilesets.end()) ? it->second.get() : nullptr;
}

void Tilemap25DCanvas::ClearFaces() {
    m_faces.clear();
    ClearSelection();
    emit sceneModified();
    update();
}

void Tilemap25DCanvas::SetFaces(const std::vector<PaintedFace>& faces) {
    m_faces = faces;
    ClearSelection();
    emit sceneModified();
    update();
}

void Tilemap25DCanvas::ClearSelection() {
    m_selected_faces.clear();
    m_selected_vertices.clear();
    m_selected_edges.clear();
    
    // Clear selection flags on faces
    for (auto& face : m_faces) {
        face.selected = false;
    }
    
    emit selectionChanged();
    update();
}

void Tilemap25DCanvas::initializeGL() {
    initializeOpenGLFunctions();
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    
    setupShaders();
    setupBuffers();
    
    // Initialize camera
    m_camera = std::make_unique<Lupine::Camera>();
    updateCamera();
}

void Tilemap25DCanvas::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    updateProjectionMatrix();
}

void Tilemap25DCanvas::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    updateCamera();
    
    if (m_show_grid) {
        renderGrid();
    }
    
    renderFaces();
    renderSelection();
    renderPreview();

    if (m_gizmo_mode != Tilemap25DGizmoMode::None && !m_selected_faces.empty()) {
        renderGizmos();
    }
}

void Tilemap25DCanvas::setupShaders() {
    // Create basic shaders for rendering
    // This is a simplified implementation - in practice you'd load from files
    
    const char* vertex_shader_source = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        layout (location = 2) in vec3 aNormal;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec2 TexCoord;
        out vec3 Normal;
        out vec3 FragPos;
        
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            TexCoord = aTexCoord;
            Normal = mat3(transpose(inverse(model))) * aNormal;
            FragPos = vec3(model * vec4(aPos, 1.0));
        }
    )";
    
    const char* fragment_shader_source = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec2 TexCoord;
        in vec3 Normal;
        in vec3 FragPos;
        
        uniform sampler2D texture1;
        uniform vec4 color;
        uniform bool useTexture;
        uniform bool selected;
        
        void main() {
            vec4 texColor = useTexture ? texture(texture1, TexCoord) : vec4(1.0);
            vec4 finalColor = texColor * color;
            
            if (selected) {
                finalColor = mix(finalColor, vec4(1.0, 1.0, 0.0, 1.0), 0.3);
            }
            
            FragColor = finalColor;
        }
    )";
    
    // Create and compile shaders
    auto graphics_device = Lupine::Renderer::GetGraphicsDevice();
    if (graphics_device) {
        m_face_shader = graphics_device->CreateShader(vertex_shader_source, fragment_shader_source);
        m_grid_shader = graphics_device->CreateShader(vertex_shader_source, fragment_shader_source);
        m_gizmo_shader = graphics_device->CreateShader(vertex_shader_source, fragment_shader_source);
    }
}

void Tilemap25DCanvas::setupBuffers() {
    // Setup grid VAO/VBO
    glGenVertexArrays(1, &m_grid_vao);
    glGenBuffers(1, &m_grid_vbo);

    // Setup face VAO/VBO/EBO
    glGenVertexArrays(1, &m_face_vao);
    glGenBuffers(1, &m_face_vbo);
    glGenBuffers(1, &m_face_ebo);

    // Setup gizmo VAO/VBO
    glGenVertexArrays(1, &m_gizmo_vao);
    glGenBuffers(1, &m_gizmo_vbo);
}

void Tilemap25DCanvas::updateCamera() {
    if (!m_camera) return;

    // Calculate camera position based on rotation and distance
    float rad_x = glm::radians(m_camera_rotation_x);
    float rad_y = glm::radians(m_camera_rotation_y);

    glm::vec3 camera_pos;
    camera_pos.x = m_camera_target.x + m_camera_distance * cos(rad_x) * sin(rad_y);
    camera_pos.y = m_camera_target.y + m_camera_distance * sin(rad_x);
    camera_pos.z = m_camera_target.z + m_camera_distance * cos(rad_x) * cos(rad_y);

    m_camera->SetPosition(camera_pos);
    m_camera->SetTarget(m_camera_target);
    m_camera->UpdateMatrices();
}

void Tilemap25DCanvas::updateProjectionMatrix() {
    if (!m_camera) return;

    float aspect = static_cast<float>(width()) / static_cast<float>(height());
    m_camera->SetPerspective(45.0f, aspect, 0.1f, 1000.0f);
    m_camera->UpdateMatrices();
}

glm::mat4 Tilemap25DCanvas::getViewMatrix() const {
    return m_camera ? m_camera->GetViewMatrix() : glm::mat4(1.0f);
}

glm::mat4 Tilemap25DCanvas::getProjectionMatrix() const {
    return m_camera ? m_camera->GetProjectionMatrix() : glm::mat4(1.0f);
}

void Tilemap25DCanvas::renderGrid() {
    if (!m_show_grid || !m_grid_shader) return;

    // Generate grid lines
    std::vector<glm::vec3> grid_vertices;
    float grid_extent = 50.0f;
    int grid_lines = static_cast<int>(grid_extent / m_grid_size);

    // Horizontal lines
    for (int i = -grid_lines; i <= grid_lines; ++i) {
        float y = i * m_grid_size + m_grid_offset.y;
        grid_vertices.push_back(glm::vec3(-grid_extent + m_grid_offset.x, y, 0.0f));
        grid_vertices.push_back(glm::vec3(grid_extent + m_grid_offset.x, y, 0.0f));
    }

    // Vertical lines
    for (int i = -grid_lines; i <= grid_lines; ++i) {
        float x = i * m_grid_size + m_grid_offset.x;
        grid_vertices.push_back(glm::vec3(x, -grid_extent + m_grid_offset.y, 0.0f));
        grid_vertices.push_back(glm::vec3(x, grid_extent + m_grid_offset.y, 0.0f));
    }

    // Upload grid data
    glBindVertexArray(m_grid_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_grid_vbo);
    glBufferData(GL_ARRAY_BUFFER, grid_vertices.size() * sizeof(glm::vec3),
                 grid_vertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Render grid
    m_grid_shader->Use();
    m_grid_shader->SetMat4("view", getViewMatrix());
    m_grid_shader->SetMat4("projection", getProjectionMatrix());
    m_grid_shader->SetMat4("model", glm::mat4(1.0f));
    m_grid_shader->SetVec4("color", glm::vec4(0.5f, 0.5f, 0.5f, 0.5f));

    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(grid_vertices.size()));

    glBindVertexArray(0);
}

void Tilemap25DCanvas::renderFaces() {
    if (m_faces.empty()) return;

    // Use immediate mode rendering for better compatibility
    renderFacesImmediate();
}

void Tilemap25DCanvas::renderFacesImmediate() {
    // Group faces by tileset for efficient rendering
    std::map<int, std::vector<size_t>> faces_by_tileset;
    for (size_t i = 0; i < m_faces.size(); ++i) {
        const auto& face = m_faces[i];
        faces_by_tileset[face.tileset_id].push_back(i);
    }

    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // Render faces grouped by tileset
    for (const auto& tileset_group : faces_by_tileset) {
        int tileset_id = tileset_group.first;
        const auto& face_indices = tileset_group.second;

        // Bind appropriate texture for this tileset
        auto texture_it = m_tileset_textures.find(tileset_id);
        if (texture_it != m_tileset_textures.end()) {
            glBindTexture(GL_TEXTURE_2D, texture_it->second);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // Render each face in this tileset
        for (size_t face_idx : face_indices) {
            const auto& face = m_faces[face_idx];

            // Render front face
            glBegin(GL_QUADS);
            for (int j = 0; j < 4; ++j) {
                glTexCoord2f(face.uvs[j].x, face.uvs[j].y);
                glVertex3f(face.vertices[j].x, face.vertices[j].y, face.vertices[j].z);
            }
            glEnd();

            // Render back face if double-sided
            if (face.double_sided) {
                glBegin(GL_QUADS);
                for (int j = 3; j >= 0; --j) { // Reverse winding order
                    glTexCoord2f(face.uvs[j].x, face.uvs[j].y);
                    glVertex3f(face.vertices[j].x, face.vertices[j].y, face.vertices[j].z);
                }
                glEnd();
            }
        }
    }

    glDisable(GL_TEXTURE_2D);
}

void Tilemap25DCanvas::renderSelection() {
    // Enable blending for selection highlights
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth testing for selection overlay
    glDisable(GL_DEPTH_TEST);

    // Render face selection highlights
    if (m_selection_mode == Tilemap25DSelectionMode::Face && !m_selected_faces.empty()) {
        for (int face_index : m_selected_faces) {
            if (face_index >= 0 && face_index < static_cast<int>(m_faces.size())) {
                const auto& face = m_faces[face_index];

                // Render selection outline
                glLineWidth(3.0f);
                glColor4f(1.0f, 0.5f, 0.0f, 0.8f); // Orange outline

                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < 4; ++i) {
                    glVertex3f(face.vertices[i].x, face.vertices[i].y, face.vertices[i].z);
                }
                glEnd();

                // Render selection fill
                glColor4f(1.0f, 0.5f, 0.0f, 0.2f); // Semi-transparent orange fill
                glBegin(GL_QUADS);
                for (int i = 0; i < 4; ++i) {
                    glVertex3f(face.vertices[i].x, face.vertices[i].y, face.vertices[i].z);
                }
                glEnd();
            }
        }
    }

    // Render vertex selection
    if (m_selection_mode == Tilemap25DSelectionMode::Vertex && !m_selected_vertices.empty()) {
        glPointSize(8.0f);
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f); // Yellow vertices

        glBegin(GL_POINTS);
        for (const auto& selected_vertex : m_selected_vertices) {
            if (selected_vertex.face_index >= 0 &&
                selected_vertex.face_index < static_cast<int>(m_faces.size()) &&
                selected_vertex.vertex_index >= 0 && selected_vertex.vertex_index < 4) {
                const auto& face = m_faces[selected_vertex.face_index];
                const glm::vec3& vertex = face.vertices[selected_vertex.vertex_index];
                glVertex3f(vertex.x, vertex.y, vertex.z);
            }
        }
        glEnd();
    }

    // Render edge selection
    if (m_selection_mode == Tilemap25DSelectionMode::Edge && !m_selected_edges.empty()) {
        glLineWidth(5.0f);
        glColor4f(0.0f, 1.0f, 1.0f, 1.0f); // Cyan edges

        glBegin(GL_LINES);
        for (const auto& selected_edge : m_selected_edges) {
            if (selected_edge.face_index >= 0 &&
                selected_edge.face_index < static_cast<int>(m_faces.size()) &&
                selected_edge.edge_index >= 0 && selected_edge.edge_index < 4) {
                const auto& face = m_faces[selected_edge.face_index];

                // Get edge vertices
                int v1 = selected_edge.edge_index;
                int v2 = (selected_edge.edge_index + 1) % 4;

                glVertex3f(face.vertices[v1].x, face.vertices[v1].y, face.vertices[v1].z);
                glVertex3f(face.vertices[v2].x, face.vertices[v2].y, face.vertices[v2].z);
            }
        }
        glEnd();
    }

    // Restore OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glLineWidth(1.0f);
    glPointSize(1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void Tilemap25DCanvas::renderPreview() {
    if (!m_show_preview || m_current_tileset_id < 0 || m_current_tile_id < 0) {
        return;
    }

    // Disable depth test for preview rendering
    glDisable(GL_DEPTH_TEST);

    // Render preview as wireframe only (no texture)
    glLineWidth(3.0f);
    glColor4f(0.0f, 1.0f, 1.0f, 0.9f); // Bright cyan wireframe

    // Render wireframe outline
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 4; ++i) {
        glVertex3f(m_preview_face.vertices[i].x, m_preview_face.vertices[i].y, m_preview_face.vertices[i].z);
    }
    glEnd();

    // Render diagonal lines to show face shape more clearly
    glLineWidth(1.5f);
    glColor4f(0.0f, 0.8f, 0.8f, 0.7f); // Slightly dimmer cyan for diagonals

    glBegin(GL_LINES);
    // Diagonal 1: vertex 0 to vertex 2
    glVertex3f(m_preview_face.vertices[0].x, m_preview_face.vertices[0].y, m_preview_face.vertices[0].z);
    glVertex3f(m_preview_face.vertices[2].x, m_preview_face.vertices[2].y, m_preview_face.vertices[2].z);
    // Diagonal 2: vertex 1 to vertex 3
    glVertex3f(m_preview_face.vertices[1].x, m_preview_face.vertices[1].y, m_preview_face.vertices[1].z);
    glVertex3f(m_preview_face.vertices[3].x, m_preview_face.vertices[3].y, m_preview_face.vertices[3].z);
    glEnd();

    // Restore OpenGL state
    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

void Tilemap25DCanvas::renderGizmos() {
    if (m_gizmo_mode == Tilemap25DGizmoMode::None) return;

    // Calculate gizmo position based on selection mode
    glm::vec3 center(0.0f);
    int point_count = 0;
    bool has_selection = false;

    switch (m_selection_mode) {
        case Tilemap25DSelectionMode::Face:
            if (!m_selected_faces.empty()) {
                for (int face_index : m_selected_faces) {
                    if (face_index >= 0 && face_index < static_cast<int>(m_faces.size())) {
                        const auto& face = m_faces[face_index];
                        for (int i = 0; i < 4; ++i) {
                            center += face.vertices[i];
                            point_count++;
                        }
                        has_selection = true;
                    }
                }
            }
            break;

        case Tilemap25DSelectionMode::Vertex:
            if (!m_selected_vertices.empty()) {
                for (const auto& selected_vertex : m_selected_vertices) {
                    if (selected_vertex.face_index >= 0 &&
                        selected_vertex.face_index < static_cast<int>(m_faces.size()) &&
                        selected_vertex.vertex_index >= 0 && selected_vertex.vertex_index < 4) {
                        const auto& face = m_faces[selected_vertex.face_index];
                        center += face.vertices[selected_vertex.vertex_index];
                        point_count++;
                        has_selection = true;
                    }
                }
            }
            break;

        case Tilemap25DSelectionMode::Edge:
            if (!m_selected_edges.empty()) {
                for (const auto& selected_edge : m_selected_edges) {
                    if (selected_edge.face_index >= 0 &&
                        selected_edge.face_index < static_cast<int>(m_faces.size()) &&
                        selected_edge.edge_index >= 0 && selected_edge.edge_index < 4) {
                        const auto& face = m_faces[selected_edge.face_index];

                        // Get edge midpoint
                        int v1 = selected_edge.edge_index;
                        int v2 = (selected_edge.edge_index + 1) % 4;
                        glm::vec3 edge_center = (face.vertices[v1] + face.vertices[v2]) * 0.5f;
                        center += edge_center;
                        point_count++;
                        has_selection = true;
                    }
                }
            }
            break;
    }

    if (!has_selection || point_count == 0) return;

    center /= static_cast<float>(point_count);
    m_gizmo_position = center;

    // Render appropriate gizmo based on mode
    switch (m_gizmo_mode) {
        case Tilemap25DGizmoMode::Move:
            renderMoveGizmo();
            break;
        case Tilemap25DGizmoMode::Rotate:
            renderRotateGizmo();
            break;
        case Tilemap25DGizmoMode::Scale:
            renderScaleGizmo();
            break;
        default:
            break;
    }
}

void Tilemap25DCanvas::renderMoveGizmo() {
    // Render 3D move gizmo (X, Y, Z axes) with improved visibility
    glDisable(GL_DEPTH_TEST);
    glLineWidth(4.0f);

    // Calculate gizmo size based on camera distance for consistent screen size
    float gizmo_size = m_camera_distance * 0.15f;
    float arrow_size = gizmo_size * 0.2f;

    glPushMatrix();
    glTranslatef(m_gizmo_position.x, m_gizmo_position.y, m_gizmo_position.z);

    // X axis (red) with arrow
    glColor3f(1.0f, 0.2f, 0.2f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(gizmo_size, 0.0f, 0.0f);
    // Arrow head
    glVertex3f(gizmo_size, 0.0f, 0.0f);
    glVertex3f(gizmo_size - arrow_size, arrow_size * 0.5f, 0.0f);
    glVertex3f(gizmo_size, 0.0f, 0.0f);
    glVertex3f(gizmo_size - arrow_size, -arrow_size * 0.5f, 0.0f);
    glEnd();

    // Y axis (green) with arrow
    glColor3f(0.2f, 1.0f, 0.2f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, gizmo_size, 0.0f);
    // Arrow head
    glVertex3f(0.0f, gizmo_size, 0.0f);
    glVertex3f(arrow_size * 0.5f, gizmo_size - arrow_size, 0.0f);
    glVertex3f(0.0f, gizmo_size, 0.0f);
    glVertex3f(-arrow_size * 0.5f, gizmo_size - arrow_size, 0.0f);
    glEnd();

    // Z axis (blue) with arrow
    glColor3f(0.2f, 0.2f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, gizmo_size);
    // Arrow head
    glVertex3f(0.0f, 0.0f, gizmo_size);
    glVertex3f(arrow_size * 0.5f, 0.0f, gizmo_size - arrow_size);
    glVertex3f(0.0f, 0.0f, gizmo_size);
    glVertex3f(-arrow_size * 0.5f, 0.0f, gizmo_size - arrow_size);
    glEnd();

    // Center sphere for better visibility
    glColor3f(1.0f, 1.0f, 0.0f);
    glPointSize(8.0f);
    glBegin(GL_POINTS);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();

    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0f);
    glPointSize(1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
}

void Tilemap25DCanvas::renderRotateGizmo() {
    // Render 3D rotation gizmo (colored circles for each axis)
    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0f);

    float gizmo_size = m_camera_distance * 0.08f;
    int segments = 32;

    glPushMatrix();
    glTranslatef(m_gizmo_position.x, m_gizmo_position.y, m_gizmo_position.z);

    // X axis circle (red) - YZ plane
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex3f(0.0f, gizmo_size * cos(angle), gizmo_size * sin(angle));
    }
    glEnd();

    // Y axis circle (green) - XZ plane
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex3f(gizmo_size * cos(angle), 0.0f, gizmo_size * sin(angle));
    }
    glEnd();

    // Z axis circle (blue) - XY plane
    glColor3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex3f(gizmo_size * cos(angle), gizmo_size * sin(angle), 0.0f);
    }
    glEnd();

    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
}

void Tilemap25DCanvas::renderScaleGizmo() {
    // Render 3D scale gizmo (colored cubes at the end of axes)
    glDisable(GL_DEPTH_TEST);

    float gizmo_size = m_camera_distance * 0.1f;
    float cube_size = gizmo_size * 0.1f;

    glPushMatrix();
    glTranslatef(m_gizmo_position.x, m_gizmo_position.y, m_gizmo_position.z);

    // X axis (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(gizmo_size, 0.0f, 0.0f);
    glEnd();

    // X cube
    glPushMatrix();
    glTranslatef(gizmo_size, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    // Simple cube faces
    glVertex3f(-cube_size, -cube_size, -cube_size);
    glVertex3f( cube_size, -cube_size, -cube_size);
    glVertex3f( cube_size,  cube_size, -cube_size);
    glVertex3f(-cube_size,  cube_size, -cube_size);
    glEnd();
    glPopMatrix();

    // Y axis (green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, gizmo_size, 0.0f);
    glEnd();

    // Z axis (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, gizmo_size);
    glEnd();

    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glLineWidth(1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
}

void Tilemap25DCanvas::mousePressEvent(QMouseEvent* event) {
    m_last_mouse_pos = event->pos();
    m_mouse_pressed = true;
    m_pressed_button = event->button();

    // Handle camera controls first
    if (event->button() == Qt::MiddleButton) {
        // Start panning
        m_is_panning = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    } else if (event->button() == Qt::RightButton) {
        // Start orbiting
        m_is_orbiting = true;
        setCursor(Qt::SizeAllCursor);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        // Test for gizmo interaction first
        if (m_gizmo_mode != Tilemap25DGizmoMode::None && testGizmoHit(event->pos())) {
            m_dragging_gizmo = true;
            m_drag_start_pos = screenToWorld(event->pos());
            return;
        }

        // Handle tool-specific interactions
        switch (m_current_tool) {
            case Tilemap25DPaintTool::Paint:
                {
                    glm::vec3 world_pos = screenToWorld(event->pos());
                    if (m_snap_to_grid) {
                        world_pos = snapToGrid(world_pos);
                    }
                    if (m_snap_to_edges) {
                        world_pos = snapToEdges(world_pos);
                    }
                    paintFace(world_pos, glm::vec3(0, 0, 1));
                }
                break;

            case Tilemap25DPaintTool::Erase:
                {
                    int face_index = pickFace(event->pos());
                    if (face_index >= 0) {
                        eraseFace(face_index);
                    }
                }
                break;

            case Tilemap25DPaintTool::Select:
                {
                    bool add_to_selection = event->modifiers() & Qt::ControlModifier;

                    switch (m_selection_mode) {
                        case Tilemap25DSelectionMode::Face:
                            {
                                int face_index = pickFace(event->pos());
                                if (face_index >= 0) {
                                    selectFace(face_index, add_to_selection);
                                } else if (!add_to_selection) {
                                    ClearSelection();
                                }
                            }
                            break;

                        case Tilemap25DSelectionMode::Vertex:
                            {
                                int face_index;
                                int vertex_index = pickVertex(event->pos(), face_index);
                                if (vertex_index >= 0) {
                                    selectVertex(face_index, vertex_index, add_to_selection);
                                } else if (!add_to_selection) {
                                    ClearSelection();
                                }
                            }
                            break;

                        case Tilemap25DSelectionMode::Edge:
                            {
                                int face_index;
                                int edge_index = pickEdge(event->pos(), face_index);
                                if (edge_index >= 0) {
                                    selectEdge(face_index, edge_index, add_to_selection);
                                } else if (!add_to_selection) {
                                    ClearSelection();
                                }
                            }
                            break;
                    }
                }
                break;

            case Tilemap25DPaintTool::Eyedropper:
                {
                    int face_index = pickFace(event->pos());
                    if (face_index >= 0 && face_index < static_cast<int>(m_faces.size())) {
                        const auto& face = m_faces[face_index];
                        SetCurrentTile(face.tileset_id, face.tile_id);
                    }
                }
                break;
        }
    }

    update();
}

void Tilemap25DCanvas::mouseMoveEvent(QMouseEvent* event) {
    if (m_mouse_pressed) {
        QPoint delta = event->pos() - m_last_mouse_pos;

        if (m_dragging_gizmo) {
            // Handle gizmo manipulation
            glm::vec3 current_world_pos = screenToWorld(event->pos());
            glm::vec3 delta_pos = current_world_pos - m_drag_start_pos;
            manipulateSelection(delta_pos);
            m_drag_start_pos = current_world_pos;
        } else if (m_is_panning && (event->buttons() & Qt::MiddleButton)) {
            // Pan camera - move camera position and target together
            float pan_scale = m_camera_distance * 0.001f; // Scale by distance for consistent feel

            // Calculate right and up vectors for camera-relative panning
            glm::vec3 forward = glm::normalize(m_camera_target -
                (m_camera_target + glm::vec3(
                    m_camera_distance * cos(glm::radians(m_camera_rotation_x)) * sin(glm::radians(m_camera_rotation_y)),
                    m_camera_distance * sin(glm::radians(m_camera_rotation_x)),
                    m_camera_distance * cos(glm::radians(m_camera_rotation_x)) * cos(glm::radians(m_camera_rotation_y))
                )));
            glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
            glm::vec3 up = glm::normalize(glm::cross(right, forward));

            // Apply panning movement
            glm::vec3 pan_movement = right * (-delta.x() * pan_scale) + up * (delta.y() * pan_scale);
            m_camera_target += pan_movement;

            updateCamera();
        } else if (m_is_orbiting && (event->buttons() & Qt::RightButton)) {
            // Orbit camera around target
            m_camera_rotation_y += delta.x() * 0.5f;
            m_camera_rotation_x -= delta.y() * 0.5f;

            // Clamp vertical rotation
            m_camera_rotation_x = std::max(-89.0f, std::min(89.0f, m_camera_rotation_x));

            updateCamera();
        }

        update();
    } else {
        // Update preview when not dragging
        if (m_current_tool == Tilemap25DPaintTool::Paint || m_current_tool == Tilemap25DPaintTool::Erase) {
            updatePreview(event->pos());
            update();
        }
    }

    m_last_mouse_pos = event->pos();
}

void Tilemap25DCanvas::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging_gizmo = false;
    } else if (event->button() == Qt::MiddleButton && m_is_panning) {
        m_is_panning = false;
        setCursor(Qt::ArrowCursor);
    } else if (event->button() == Qt::RightButton && m_is_orbiting) {
        m_is_orbiting = false;
        setCursor(Qt::ArrowCursor);
    }

    m_mouse_pressed = false;
    m_pressed_button = Qt::NoButton;
}

void Tilemap25DCanvas::wheelEvent(QWheelEvent* event) {
    // Camera zoom - more responsive and smooth
    float delta = event->angleDelta().y() / 120.0f; // Normalize wheel delta
    float zoom_factor = 1.0f - (delta * 0.1f); // 10% zoom per wheel step

    m_camera_distance *= zoom_factor;
    m_camera_distance = std::max(0.5f, std::min(200.0f, m_camera_distance));

    updateCamera();
    update();
}

void Tilemap25DCanvas::keyPressEvent(QKeyEvent* event) {
    // Handle grid shifting with Shift + Arrow keys
    if (event->modifiers() & Qt::ShiftModifier) {
        processGridShift(event);
        return;
    }

    // Handle other keyboard shortcuts
    switch (event->key()) {
        case Qt::Key_Delete:
            // Delete selected faces
            for (int i = static_cast<int>(m_selected_faces.size()) - 1; i >= 0; --i) {
                eraseFace(m_selected_faces[i]);
            }
            break;

        case Qt::Key_Escape:
            ClearSelection();
            break;

        case Qt::Key_A:
            if (event->modifiers() & Qt::ControlModifier) {
                // Select all faces
                m_selected_faces.clear();
                for (size_t i = 0; i < m_faces.size(); ++i) {
                    m_selected_faces.push_back(static_cast<int>(i));
                    m_faces[i].selected = true;
                }
                emit selectionChanged();
            }
            break;

        case Qt::Key_Q:
            // Rotate tile counter-clockwise around Y axis
            m_tile_rotation_y -= 90.0f;
            if (m_tile_rotation_y < 0.0f) m_tile_rotation_y += 360.0f;
            break;

        case Qt::Key_E:
            // Rotate tile clockwise around Y axis
            m_tile_rotation_y += 90.0f;
            if (m_tile_rotation_y >= 360.0f) m_tile_rotation_y -= 360.0f;
            break;

        case Qt::Key_W:
            // Toggle tile standing up (rotate around X axis)
            m_tile_rotation_x = (m_tile_rotation_x == 0.0f) ? 90.0f : 0.0f;
            break;

        case Qt::Key_S:
            // Toggle tile standing up (rotate around Z axis)
            m_tile_rotation_z = (m_tile_rotation_z == 0.0f) ? 90.0f : 0.0f;
            break;
    }

    update();
}

void Tilemap25DCanvas::processGridShift(QKeyEvent* event) {
    float shift_amount = m_grid_size * 0.1f; // Shift by 10% of grid size

    switch (event->key()) {
        case Qt::Key_Left:
            ShiftGridHorizontal(-shift_amount);
            break;
        case Qt::Key_Right:
            ShiftGridHorizontal(shift_amount);
            break;
        case Qt::Key_Up:
            ShiftGridVertical(shift_amount);
            break;
        case Qt::Key_Down:
            ShiftGridVertical(-shift_amount);
            break;
    }
}

void Tilemap25DCanvas::HandleKeyPress(QKeyEvent* event) {
    keyPressEvent(event);
}

glm::vec3 Tilemap25DCanvas::screenToWorld(const QPoint& screen_pos, float depth) {
    // Convert screen coordinates to world coordinates using ray-plane intersection
    glm::mat4 view = getViewMatrix();
    glm::mat4 proj = getProjectionMatrix();
    glm::vec4 viewport = glm::vec4(0, 0, width(), height());

    // Convert screen Y to OpenGL Y (flip)
    float gl_y = height() - screen_pos.y();

    // Create ray from camera through screen point
    glm::vec3 ray_start = glm::unProject(
        glm::vec3(screen_pos.x(), gl_y, 0.0f),
        glm::mat4(1.0f),
        proj * view,
        viewport
    );

    glm::vec3 ray_end = glm::unProject(
        glm::vec3(screen_pos.x(), gl_y, 1.0f),
        glm::mat4(1.0f),
        proj * view,
        viewport
    );

    glm::vec3 ray_dir = glm::normalize(ray_end - ray_start);

    // Intersect ray with a plane at the specified depth
    // Default to Z=0 plane if no depth specified
    glm::vec3 plane_normal(0.0f, 0.0f, 1.0f);
    glm::vec3 plane_point(0.0f, 0.0f, depth);

    // Calculate intersection
    float denom = glm::dot(plane_normal, ray_dir);
    if (abs(denom) > 0.0001f) {
        float t = glm::dot(plane_point - ray_start, plane_normal) / denom;
        if (t >= 0.0f) {
            return ray_start + t * ray_dir;
        }
    }

    // Fallback: project to a reasonable distance from camera
    float fallback_distance = m_camera_distance * 0.5f;
    return ray_start + fallback_distance * ray_dir;
}

glm::vec3 Tilemap25DCanvas::snapToGrid(const glm::vec3& position) {
    if (!m_snap_to_grid) return position;

    glm::vec3 snapped = position;

    // Snap X axis if enabled with improved responsiveness
    if (m_snap_x_axis) {
        float grid_x = m_grid_size_per_axis.x;
        if (grid_x > 0.0f) {
            float offset_x = position.x - m_grid_offset.x;
            snapped.x = std::round(offset_x / grid_x) * grid_x + m_grid_offset.x;
        }
    }

    // Snap Y axis if enabled with improved responsiveness
    if (m_snap_y_axis) {
        float grid_y = m_grid_size_per_axis.y;
        if (grid_y > 0.0f) {
            float offset_y = position.y - m_grid_offset.y;
            snapped.y = std::round(offset_y / grid_y) * grid_y + m_grid_offset.y;
        }
    }

    // Snap Z axis if enabled with improved responsiveness
    if (m_snap_z_axis) {
        float grid_z = m_grid_size_per_axis.z;
        if (grid_z > 0.0f) {
            snapped.z = std::round(position.z / grid_z) * grid_z;
        }
    }

    return snapped;
}

glm::vec3 Tilemap25DCanvas::snapToEdges(const glm::vec3& position) {
    if (!m_snap_to_edges || m_faces.empty()) {
        return position;
    }

    glm::vec3 snapped = position;
    float closest_distance = m_edge_snap_distance;
    bool found_edge_snap = false;
    bool found_vertex_snap = false;

    // Check all edges and vertices of all faces
    for (const auto& face : m_faces) {
        // Check vertices first (higher priority than edges)
        for (int i = 0; i < 4; ++i) {
            glm::vec3 vertex = face.vertices[i];
            float distance = glm::length(vertex - position);

            if (distance < closest_distance * 0.5f) { // Vertices have smaller snap distance
                closest_distance = distance;
                snapped = vertex;
                found_vertex_snap = true;
            }
        }

        // Check edges if no vertex snap found
        if (!found_vertex_snap) {
            for (int i = 0; i < 4; ++i) {
                int next_i = (i + 1) % 4;
                glm::vec3 edge_start = face.vertices[i];
                glm::vec3 edge_end = face.vertices[next_i];

                // Calculate closest point on edge to position
                glm::vec3 edge_dir = edge_end - edge_start;
                float edge_length = glm::length(edge_dir);

                if (edge_length > 0.001f) { // Avoid division by zero
                    edge_dir /= edge_length;

                    float t = glm::dot(position - edge_start, edge_dir);
                    t = std::max(0.0f, std::min(edge_length, t));

                    glm::vec3 closest_point = edge_start + t * edge_dir;
                    float distance = glm::length(closest_point - position);

                    if (distance < closest_distance) {
                        closest_distance = distance;
                        snapped = closest_point;
                        found_edge_snap = true;
                    }
                }
            }
        }
    }

    return (found_vertex_snap || found_edge_snap) ? snapped : position;
}

int Tilemap25DCanvas::pickFace(const QPoint& screen_pos) {
    // Create ray from camera through screen point
    glm::mat4 view = getViewMatrix();
    glm::mat4 proj = getProjectionMatrix();
    glm::vec4 viewport = glm::vec4(0, 0, width(), height());

    float gl_y = height() - screen_pos.y();

    glm::vec3 ray_start = glm::unProject(
        glm::vec3(screen_pos.x(), gl_y, 0.0f),
        glm::mat4(1.0f),
        proj * view,
        viewport
    );

    glm::vec3 ray_end = glm::unProject(
        glm::vec3(screen_pos.x(), gl_y, 1.0f),
        glm::mat4(1.0f),
        proj * view,
        viewport
    );

    glm::vec3 ray_dir = glm::normalize(ray_end - ray_start);

    float closest_distance = std::numeric_limits<float>::max();
    int closest_face = -1;

    for (size_t i = 0; i < m_faces.size(); ++i) {
        const auto& face = m_faces[i];

        // Test ray intersection with face (quad as two triangles)
        float distance;
        if (rayIntersectQuad(ray_start, ray_dir, face.vertices, distance)) {
            if (distance < closest_distance && distance > 0.0f) {
                closest_distance = distance;
                closest_face = static_cast<int>(i);
            }
        }
    }

    return closest_face;
}

int Tilemap25DCanvas::pickVertex(const QPoint& screen_pos, int& face_index) {
    // Pick the closest vertex to the screen position
    glm::vec3 world_pos = screenToWorld(screen_pos);

    float closest_distance = std::numeric_limits<float>::max();
    int closest_vertex = -1;
    face_index = -1;

    for (size_t i = 0; i < m_faces.size(); ++i) {
        const auto& face = m_faces[i];

        for (int j = 0; j < 4; ++j) {
            float distance = glm::length(face.vertices[j] - world_pos);
            if (distance < closest_distance && distance < 0.5f) { // Within selection threshold
                closest_distance = distance;
                closest_vertex = j;
                face_index = static_cast<int>(i);
            }
        }
    }

    return closest_vertex;
}

int Tilemap25DCanvas::pickEdge(const QPoint& screen_pos, int& face_index) {
    // Pick the closest edge to the screen position
    glm::vec3 world_pos = screenToWorld(screen_pos);

    float closest_distance = std::numeric_limits<float>::max();
    int closest_edge = -1;
    face_index = -1;

    for (size_t i = 0; i < m_faces.size(); ++i) {
        const auto& face = m_faces[i];

        for (int j = 0; j < 4; ++j) {
            int next_j = (j + 1) % 4;

            // Calculate distance from point to line segment
            glm::vec3 edge_start = face.vertices[j];
            glm::vec3 edge_end = face.vertices[next_j];
            glm::vec3 edge_dir = edge_end - edge_start;

            float t = glm::dot(world_pos - edge_start, edge_dir) / glm::dot(edge_dir, edge_dir);
            t = std::max(0.0f, std::min(1.0f, t));

            glm::vec3 closest_point = edge_start + t * edge_dir;
            float distance = glm::length(closest_point - world_pos);

            if (distance < closest_distance && distance < 0.3f) { // Within selection threshold
                closest_distance = distance;
                closest_edge = j;
                face_index = static_cast<int>(i);
            }
        }
    }

    return closest_edge;
}

void Tilemap25DCanvas::paintFace(const glm::vec3& position, const glm::vec3& normal) {
    if (m_current_tileset_id < 0 || m_current_tile_id < 0) return;

    PaintedFace new_face;

    // Create a unit quad at the specified position with rotation around bottom edge
    float half_size = m_grid_size * 0.5f;
    glm::vec3 base_vertices[4] = {
        glm::vec3(-half_size, -half_size, 0.0f),
        glm::vec3( half_size, -half_size, 0.0f),
        glm::vec3( half_size,  half_size, 0.0f),
        glm::vec3(-half_size,  half_size, 0.0f)
    };

    // Apply rotations around bottom edge
    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(m_tile_rotation_x), glm::vec3(1, 0, 0));
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(m_tile_rotation_y), glm::vec3(0, 1, 0));
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(m_tile_rotation_z), glm::vec3(0, 0, 1));

    // Calculate rotation pivot point (bottom edge center)
    glm::vec3 pivot_offset(0.0f, -half_size, 0.0f);

    // Apply rotation around bottom edge and translate to position
    for (int i = 0; i < 4; ++i) {
        // Translate to pivot point (bottom edge)
        glm::vec3 vertex_relative_to_pivot = base_vertices[i] - pivot_offset;

        // Apply rotation
        glm::vec4 rotated_vertex = rotation_matrix * glm::vec4(vertex_relative_to_pivot, 1.0f);

        // Translate back and add world position
        new_face.vertices[i] = position + glm::vec3(rotated_vertex) + pivot_offset;
    }

    new_face.tileset_id = m_current_tileset_id;
    new_face.tile_id = m_current_tile_id;
    new_face.normal = normal;
    new_face.double_sided = true;

    calculateUVs(new_face);

    m_faces.push_back(new_face);

    emit facePainted(static_cast<int>(m_faces.size()) - 1);
    emit sceneModified();
    update(); // Trigger repaint to show the new face
}

void Tilemap25DCanvas::eraseFace(int face_index) {
    if (face_index >= 0 && face_index < static_cast<int>(m_faces.size())) {
        m_faces.erase(m_faces.begin() + face_index);

        // Update selection indices
        for (auto it = m_selected_faces.begin(); it != m_selected_faces.end();) {
            if (*it == face_index) {
                it = m_selected_faces.erase(it);
            } else {
                if (*it > face_index) {
                    (*it)--;
                }
                ++it;
            }
        }

        emit faceErased(face_index);
        emit sceneModified();
        emit selectionChanged();
        update(); // Trigger repaint to show the face removal
    }
}

void Tilemap25DCanvas::selectFace(int face_index, bool add_to_selection) {
    if (face_index < 0 || face_index >= static_cast<int>(m_faces.size())) return;

    if (!add_to_selection) {
        ClearSelection();
    }

    auto it = std::find(m_selected_faces.begin(), m_selected_faces.end(), face_index);
    if (it == m_selected_faces.end()) {
        m_selected_faces.push_back(face_index);
        m_faces[face_index].selected = true;
    }

    updateGizmoTransform();
    emit selectionChanged();
}

void Tilemap25DCanvas::selectVertex(int face_index, int vertex_index, bool add_to_selection) {
    if (face_index < 0 || face_index >= static_cast<int>(m_faces.size()) ||
        vertex_index < 0 || vertex_index >= 4) return;

    if (!add_to_selection) {
        ClearSelection();
    }

    const auto& face = m_faces[face_index];
    SelectedVertex selected_vertex(face_index, vertex_index, face.vertices[vertex_index]);

    m_selected_vertices.push_back(selected_vertex);

    updateGizmoTransform();
    emit selectionChanged();
}

void Tilemap25DCanvas::selectEdge(int face_index, int edge_index, bool add_to_selection) {
    if (face_index < 0 || face_index >= static_cast<int>(m_faces.size()) ||
        edge_index < 0 || edge_index >= 4) return;

    if (!add_to_selection) {
        ClearSelection();
    }

    const auto& face = m_faces[face_index];
    int next_edge = (edge_index + 1) % 4;
    SelectedEdge selected_edge(face_index, edge_index,
                              face.vertices[edge_index], face.vertices[next_edge]);

    m_selected_edges.push_back(selected_edge);

    updateGizmoTransform();
    emit selectionChanged();
}

void Tilemap25DCanvas::updateGizmoTransform() {
    if (m_selected_faces.empty() && m_selected_vertices.empty() && m_selected_edges.empty()) {
        return;
    }

    // Calculate center position for gizmo
    glm::vec3 center(0.0f);
    int count = 0;

    // Include selected faces
    for (int face_index : m_selected_faces) {
        if (face_index >= 0 && face_index < static_cast<int>(m_faces.size())) {
            const auto& face = m_faces[face_index];
            for (int i = 0; i < 4; ++i) {
                center += face.vertices[i];
                count++;
            }
        }
    }

    // Include selected vertices
    for (const auto& selected_vertex : m_selected_vertices) {
        if (selected_vertex.face_index >= 0 &&
            selected_vertex.face_index < static_cast<int>(m_faces.size())) {
            const auto& face = m_faces[selected_vertex.face_index];
            center += face.vertices[selected_vertex.vertex_index];
            count++;
        }
    }

    // Include selected edges
    for (const auto& selected_edge : m_selected_edges) {
        center += selected_edge.start_pos;
        center += selected_edge.end_pos;
        count += 2;
    }

    if (count > 0) {
        center /= static_cast<float>(count);
        m_gizmo_position = center;
        m_gizmo_transform = glm::translate(glm::mat4(1.0f), center);
    }
}

bool Tilemap25DCanvas::testGizmoHit(const QPoint& screen_pos) {
    if (m_gizmo_mode == Tilemap25DGizmoMode::None) {
        return false;
    }

    // Create ray from camera through screen point
    glm::mat4 view = getViewMatrix();
    glm::mat4 proj = getProjectionMatrix();
    glm::vec4 viewport = glm::vec4(0, 0, width(), height());

    float gl_y = height() - screen_pos.y();

    glm::vec3 ray_start = glm::unProject(
        glm::vec3(screen_pos.x(), gl_y, 0.0f),
        glm::mat4(1.0f),
        proj * view,
        viewport
    );

    glm::vec3 ray_end = glm::unProject(
        glm::vec3(screen_pos.x(), gl_y, 1.0f),
        glm::mat4(1.0f),
        proj * view,
        viewport
    );

    glm::vec3 ray_dir = glm::normalize(ray_end - ray_start);

    // Test intersection with gizmo axes
    float gizmo_size = m_camera_distance * 0.15f;
    float hit_radius = gizmo_size * 0.1f; // Hit radius for axes

    // Test X axis (red)
    glm::vec3 x_axis_end = m_gizmo_position + glm::vec3(gizmo_size, 0, 0);
    float x_distance = distancePointToLine(ray_start, ray_dir, m_gizmo_position, x_axis_end);
    if (x_distance < hit_radius) {
        return true;
    }

    // Test Y axis (green)
    glm::vec3 y_axis_end = m_gizmo_position + glm::vec3(0, gizmo_size, 0);
    float y_distance = distancePointToLine(ray_start, ray_dir, m_gizmo_position, y_axis_end);
    if (y_distance < hit_radius) {
        return true;
    }

    // Test Z axis (blue)
    glm::vec3 z_axis_end = m_gizmo_position + glm::vec3(0, 0, gizmo_size);
    float z_distance = distancePointToLine(ray_start, ray_dir, m_gizmo_position, z_axis_end);
    if (z_distance < hit_radius) {
        return true;
    }

    return false;
}

float Tilemap25DCanvas::distancePointToLine(const glm::vec3& point, const glm::vec3& line_dir,
                                           const glm::vec3& line_start, const glm::vec3& line_end) {
    glm::vec3 line_vec = line_end - line_start;
    float line_length = glm::length(line_vec);

    if (line_length < 0.001f) {
        return glm::length(point - line_start);
    }

    line_vec /= line_length;

    // Project point onto line
    float t = glm::dot(point - line_start, line_vec);
    t = std::max(0.0f, std::min(line_length, t));

    glm::vec3 closest_point = line_start + t * line_vec;
    return glm::length(point - closest_point);
}

void Tilemap25DCanvas::manipulateSelection(const glm::vec3& delta) {
    // Apply transformation to selected elements based on gizmo mode
    switch (m_gizmo_mode) {
        case Tilemap25DGizmoMode::Move:
            // Move selected faces/vertices/edges
            for (int face_index : m_selected_faces) {
                if (face_index >= 0 && face_index < static_cast<int>(m_faces.size())) {
                    auto& face = m_faces[face_index];
                    for (int i = 0; i < 4; ++i) {
                        face.vertices[i] += delta;
                    }
                }
            }

            for (const auto& selected_vertex : m_selected_vertices) {
                if (selected_vertex.face_index >= 0 &&
                    selected_vertex.face_index < static_cast<int>(m_faces.size())) {
                    auto& face = m_faces[selected_vertex.face_index];
                    face.vertices[selected_vertex.vertex_index] += delta;
                }
            }
            break;

        case Tilemap25DGizmoMode::Rotate:
            // Implement rotation logic
            break;

        case Tilemap25DGizmoMode::Scale:
            // Implement scaling logic
            break;

        default:
            break;
    }

    updateGizmoTransform();
    emit sceneModified();
}

bool Tilemap25DCanvas::rayIntersectQuad(const glm::vec3& ray_start, const glm::vec3& ray_dir,
                                       const glm::vec3 vertices[4], float& distance) {
    // Test ray intersection with quad as two triangles
    // Triangle 1: vertices[0], vertices[1], vertices[2]
    // Triangle 2: vertices[0], vertices[2], vertices[3]

    float t1, t2;
    bool hit1 = rayIntersectTriangle(ray_start, ray_dir, vertices[0], vertices[1], vertices[2], t1);
    bool hit2 = rayIntersectTriangle(ray_start, ray_dir, vertices[0], vertices[2], vertices[3], t2);

    if (hit1 && hit2) {
        distance = std::min(t1, t2);
        return true;
    } else if (hit1) {
        distance = t1;
        return true;
    } else if (hit2) {
        distance = t2;
        return true;
    }

    return false;
}

bool Tilemap25DCanvas::rayIntersectTriangle(const glm::vec3& ray_start, const glm::vec3& ray_dir,
                                           const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
                                           float& distance) {
    // Möller-Trumbore ray-triangle intersection algorithm
    const float EPSILON = 0.0000001f;

    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(ray_dir, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON) {
        return false; // Ray is parallel to triangle
    }

    float f = 1.0f / a;
    glm::vec3 s = ray_start - v0;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(ray_dir, q);

    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    float t = f * glm::dot(edge2, q);

    if (t > EPSILON) {
        distance = t;
        return true;
    }

    return false;
}

void Tilemap25DCanvas::calculateUVs(PaintedFace& face) {
    // Calculate UV coordinates based on tileset and tile ID
    auto tileset = GetTileset(face.tileset_id);
    if (!tileset) {
        // Default UVs - map entire texture to face
        face.uvs[0] = glm::vec2(0.0f, 0.0f);
        face.uvs[1] = glm::vec2(1.0f, 0.0f);
        face.uvs[2] = glm::vec2(1.0f, 1.0f);
        face.uvs[3] = glm::vec2(0.0f, 1.0f);
        return;
    }

    // Get tile region from tileset
    glm::ivec2 grid_pos = tileset->GetGridPositionFromTileId(face.tile_id);

    // Get texture size from ResourceManager
    auto texture = Lupine::ResourceManager::GetTexture(tileset->GetTexturePath());
    glm::ivec2 texture_size(1, 1); // Default fallback
    if (texture.IsValid()) {
        texture_size = glm::ivec2(texture.width, texture.height);
    }

    // Calculate normalized UV coordinates for the specific tile
    glm::vec4 uv_region = tileset->CalculateNormalizedTextureRegion(grid_pos, texture_size);

    float u1 = uv_region.x;
    float v1 = uv_region.y;
    float u2 = uv_region.x + uv_region.z;
    float v2 = uv_region.y + uv_region.w;

    // Map UV coordinates to face vertices (bottom-left, bottom-right, top-right, top-left)
    face.uvs[0] = glm::vec2(u1, v2); // Bottom-left
    face.uvs[1] = glm::vec2(u2, v2); // Bottom-right
    face.uvs[2] = glm::vec2(u2, v1); // Top-right
    face.uvs[3] = glm::vec2(u1, v1); // Top-left

    // Debug output
    qDebug() << "Tile ID:" << face.tile_id << "Grid pos:" << grid_pos.x << grid_pos.y;
    qDebug() << "UV region:" << u1 << v1 << u2 << v2;
    qDebug() << "Texture size:" << texture_size.x << texture_size.y;
}

void Tilemap25DCanvas::updateFaceTexture(PaintedFace& face) {
    calculateUVs(face);
}

void Tilemap25DCanvas::updatePreview(const QPoint& mouse_pos) {
    if (m_current_tileset_id < 0 || m_current_tile_id < 0) {
        m_show_preview = false;
        return;
    }

    glm::vec3 world_pos = screenToWorld(mouse_pos);

    // Apply snapping
    if (m_snap_to_grid) {
        world_pos = snapToGrid(world_pos);
    }
    if (m_snap_to_edges) {
        world_pos = snapToEdges(world_pos);
    }

    m_preview_position = world_pos;
    m_show_preview = true;

    // Create preview face
    m_preview_face.tileset_id = m_current_tileset_id;
    m_preview_face.tile_id = m_current_tile_id;
    m_preview_face.normal = glm::vec3(0, 0, 1);
    m_preview_face.double_sided = true;

    // Create a unit quad at the preview position with rotation around bottom edge
    float half_size = m_grid_size * 0.5f;
    glm::vec3 base_vertices[4] = {
        glm::vec3(-half_size, -half_size, 0.0f),
        glm::vec3( half_size, -half_size, 0.0f),
        glm::vec3( half_size,  half_size, 0.0f),
        glm::vec3(-half_size,  half_size, 0.0f)
    };

    // Apply rotations around bottom edge
    glm::mat4 rotation_matrix = glm::mat4(1.0f);
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(m_tile_rotation_x), glm::vec3(1, 0, 0));
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(m_tile_rotation_y), glm::vec3(0, 1, 0));
    rotation_matrix = glm::rotate(rotation_matrix, glm::radians(m_tile_rotation_z), glm::vec3(0, 0, 1));

    // Calculate rotation pivot point (bottom edge center)
    glm::vec3 pivot_offset(0.0f, -half_size, 0.0f);

    // Apply rotation around bottom edge and translate to position
    for (int i = 0; i < 4; ++i) {
        // Translate to pivot point (bottom edge)
        glm::vec3 vertex_relative_to_pivot = base_vertices[i] - pivot_offset;

        // Apply rotation
        glm::vec4 rotated_vertex = rotation_matrix * glm::vec4(vertex_relative_to_pivot, 1.0f);

        // Translate back and add world position
        m_preview_face.vertices[i] = world_pos + glm::vec3(rotated_vertex) + pivot_offset;
    }

    calculateUVs(m_preview_face);
}

void Tilemap25DCanvas::clearPreview() {
    m_show_preview = false;
}

bool Tilemap25DCanvas::ExportToOBJ(const QString& filepath, bool generate_texture_atlas) {
    if (m_faces.empty()) {
        return false;
    }

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "# Tilemap 2.5D exported from Lupine Engine\n";
    out << "# Faces: " << m_faces.size() << "\n\n";

    // Generate texture atlas if requested
    std::unordered_map<int, glm::vec2> tileset_uv_offsets;
    QString texture_filename;

    if (generate_texture_atlas) {
        texture_filename = QFileInfo(filepath).baseName() + "_atlas.png";
        if (!generateTextureAtlas(filepath, tileset_uv_offsets)) {
            generate_texture_atlas = false; // Fall back to individual textures
        } else {
            out << "mtllib " << QFileInfo(filepath).baseName() << ".mtl\n\n";
        }
    }

    // Export vertices
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<std::vector<int>> face_indices;

    for (size_t face_idx = 0; face_idx < m_faces.size(); ++face_idx) {
        const auto& face = m_faces[face_idx];

        std::vector<int> indices;

        // Add vertices for this face
        for (int i = 0; i < 4; ++i) {
            vertices.push_back(face.vertices[i]);

            // Calculate UV coordinates
            glm::vec2 uv = face.uvs[i];
            if (generate_texture_atlas) {
                auto it = tileset_uv_offsets.find(face.tileset_id);
                if (it != tileset_uv_offsets.end()) {
                    // Adjust UV coordinates for atlas using dynamic scale
                    uv = it->second + uv * m_atlas_texture_scale;
                }
            }
            uvs.push_back(uv);

            normals.push_back(face.normal);
            indices.push_back(static_cast<int>(vertices.size()));
        }

        face_indices.push_back(indices);
    }

    // Write vertices
    for (const auto& vertex : vertices) {
        out << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
    }
    out << "\n";

    // Write texture coordinates
    for (const auto& uv : uvs) {
        out << "vt " << uv.x << " " << uv.y << "\n";
    }
    out << "\n";

    // Write normals
    for (const auto& normal : normals) {
        out << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
    }
    out << "\n";

    // Write material usage
    if (generate_texture_atlas) {
        out << "usemtl atlas_material\n";
    }

    // Write faces
    for (size_t face_idx = 0; face_idx < face_indices.size(); ++face_idx) {
        const auto& indices = face_indices[face_idx];
        const auto& face = m_faces[face_idx];

        // Write front-facing triangles
        out << "f " << indices[0] << "/" << indices[0] << "/" << indices[0]
            << " " << indices[1] << "/" << indices[1] << "/" << indices[1]
            << " " << indices[2] << "/" << indices[2] << "/" << indices[2] << "\n";

        out << "f " << indices[0] << "/" << indices[0] << "/" << indices[0]
            << " " << indices[2] << "/" << indices[2] << "/" << indices[2]
            << " " << indices[3] << "/" << indices[3] << "/" << indices[3] << "\n";

        // Write back-facing triangles if double-sided
        if (face.double_sided) {
            out << "f " << indices[0] << "/" << indices[0] << "/" << indices[0]
                << " " << indices[2] << "/" << indices[2] << "/" << indices[2]
                << " " << indices[1] << "/" << indices[1] << "/" << indices[1] << "\n";

            out << "f " << indices[0] << "/" << indices[0] << "/" << indices[0]
                << " " << indices[3] << "/" << indices[3] << "/" << indices[3]
                << " " << indices[2] << "/" << indices[2] << "/" << indices[2] << "\n";
        }
    }

    file.close();

    // Generate material file if using texture atlas
    if (generate_texture_atlas) {
        generateMaterialFile(filepath, texture_filename);
    }

    return true;
}

bool Tilemap25DCanvas::generateTextureAtlas(const QString& base_filepath,
                                           std::unordered_map<int, glm::vec2>& tileset_uv_offsets) {
    // Collect unique tilesets used in faces
    std::set<int> used_tilesets;
    for (const auto& face : m_faces) {
        if (face.tileset_id >= 0) {
            used_tilesets.insert(face.tileset_id);
        }
    }

    if (used_tilesets.empty()) {
        return false;
    }

    // Calculate atlas size
    int atlas_size = static_cast<int>(std::ceil(std::sqrt(used_tilesets.size())));
    int texture_size = 256; // Size of each tileset texture in the atlas
    int total_size = atlas_size * texture_size;

    // Store atlas size for UV scaling
    m_atlas_size = atlas_size;
    m_atlas_texture_scale = 1.0f / static_cast<float>(atlas_size);

    // Create atlas image
    QImage atlas_image(total_size, total_size, QImage::Format_RGBA8888);
    atlas_image.fill(Qt::transparent);

    QPainter painter(&atlas_image);

    // Place tilesets in atlas
    int index = 0;
    for (int tileset_id : used_tilesets) {
        int row = index / atlas_size;
        int col = index % atlas_size;

        int x = col * texture_size;
        int y = row * texture_size;

        // Store UV offset for this tileset
        float u = static_cast<float>(x) / total_size;
        float v = static_cast<float>(y) / total_size;
        tileset_uv_offsets[tileset_id] = glm::vec2(u, v);

        // Load and draw tileset texture
        auto tileset = GetTileset(tileset_id);
        if (tileset) {
            // Try to load the actual tileset texture
            QString tileset_path = QString::fromStdString(tileset->GetTexturePath());
            QImage tileset_image;

            if (!tileset_path.isEmpty() && tileset_image.load(tileset_path)) {
                // Scale the tileset image to fit in the atlas slot
                tileset_image = tileset_image.scaled(texture_size, texture_size,
                                                   Qt::KeepAspectRatio, Qt::SmoothTransformation);
            } else {
                // Create a placeholder texture for the tileset
                tileset_image = QImage(texture_size, texture_size, QImage::Format_RGBA8888);
                tileset_image.fill(QColor(128, 128, 128, 255)); // Gray placeholder
            }

            painter.drawImage(x, y, tileset_image);
        }

        index++;
    }

    painter.end();

    // Save atlas image
    QString atlas_path = QFileInfo(base_filepath).path() + "/" +
                        QFileInfo(base_filepath).baseName() + "_atlas.png";
    return atlas_image.save(atlas_path);
}

bool Tilemap25DCanvas::generateMaterialFile(const QString& obj_filepath, const QString& texture_filename) {
    QString mtl_path = QFileInfo(obj_filepath).path() + "/" +
                      QFileInfo(obj_filepath).baseName() + ".mtl";

    QFile file(mtl_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "# Material file for Tilemap 2.5D export\n\n";
    out << "newmtl atlas_material\n";
    out << "Ka 1.0 1.0 1.0\n";
    out << "Kd 1.0 1.0 1.0\n";
    out << "Ks 0.0 0.0 0.0\n";
    out << "Ns 0.0\n";
    out << "map_Kd " << texture_filename << "\n";

    file.close();
    return true;
}

// Tilemap25DPalette Implementation
Tilemap25DPalette::Tilemap25DPalette(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_tileset_item(nullptr)
    , m_selection_rect(nullptr)
    , m_tileset_id(-1)
    , m_tileset(nullptr)
    , m_selected_tile_id(-1)
{
    setupScene();
    setDragMode(QGraphicsView::NoDrag);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
}

Tilemap25DPalette::~Tilemap25DPalette() {
    if (m_scene) {
        delete m_scene;
    }
}

void Tilemap25DPalette::SetTileset(int tileset_id, Lupine::Tileset2DResource* tileset) {
    m_tileset_id = tileset_id;
    m_tileset = tileset;
    m_selected_tile_id = -1;

    updatePalette();
}

void Tilemap25DPalette::ClearTileset() {
    m_tileset_id = -1;
    m_tileset = nullptr;
    m_selected_tile_id = -1;

    if (m_tileset_item) {
        m_scene->removeItem(m_tileset_item);
        m_tileset_item = nullptr;
    }

    if (m_selection_rect) {
        m_scene->removeItem(m_selection_rect);
        m_selection_rect = nullptr;
    }
}

void Tilemap25DPalette::SetSelectedTile(int tile_id) {
    m_selected_tile_id = tile_id;

    if (m_selection_rect && m_tileset) {
        glm::ivec2 grid_pos = m_tileset->GetGridPositionFromTileId(tile_id);
        glm::vec4 tile_region = m_tileset->CalculateTextureRegion(grid_pos);

        m_selection_rect->setRect(tile_region.x, tile_region.y, tile_region.z, tile_region.w);
        m_selection_rect->setVisible(tile_id >= 0);
    }
}

void Tilemap25DPalette::setupScene() {
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    // Create selection rectangle
    m_selection_rect = new QGraphicsRectItem();
    m_selection_rect->setPen(QPen(Qt::yellow, 2));
    m_selection_rect->setBrush(Qt::NoBrush);
    m_selection_rect->setVisible(false);
    m_scene->addItem(m_selection_rect);
}

void Tilemap25DPalette::updatePalette() {
    m_scene->clear();
    m_tileset_item = nullptr;
    m_selection_rect = nullptr;

    if (!m_tileset || m_tileset->GetTexturePath().empty()) {
        return;
    }

    // Load tileset image
    QString imagePath = QString::fromStdString(m_tileset->GetTexturePath());
    m_tileset_pixmap = QPixmap(imagePath);

    if (m_tileset_pixmap.isNull()) {
        std::cerr << "Failed to load tileset texture: " << imagePath.toStdString() << std::endl;
        return;
    }

    // Create pixmap item and add to scene
    m_tileset_item = new QGraphicsPixmapItem(m_tileset_pixmap);
    m_scene->addItem(m_tileset_item);

    // Create selection rectangle (initially hidden)
    m_selection_rect = new QGraphicsRectItem();
    m_selection_rect->setPen(QPen(Qt::red, 2));
    m_selection_rect->setBrush(QBrush(Qt::transparent));
    m_selection_rect->setVisible(false);
    m_scene->addItem(m_selection_rect);

    // Fit the view to the tileset
    fitInView(m_tileset_item, Qt::KeepAspectRatio);
}

void Tilemap25DPalette::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_tileset) {
        QPointF scene_pos = mapToScene(event->pos());
        int tile_id = getTileIdAt(scene_pos);

        if (tile_id >= 0) {
            SetSelectedTile(tile_id);
            emit tileSelected(m_tileset_id, tile_id);
        }
    }

    QGraphicsView::mousePressEvent(event);
}

int Tilemap25DPalette::getTileIdAt(const QPointF& scene_pos) const {
    if (!m_tileset || !m_tileset_item) return -1;

    // Convert scene position to tileset coordinates
    QPointF local_pos = m_tileset_item->mapFromScene(scene_pos);

    if (!m_tileset_item->boundingRect().contains(local_pos)) {
        return -1;
    }

    // Calculate tile ID based on position
    glm::ivec2 tile_size = m_tileset->GetTileSize();
    glm::ivec2 grid_size = m_tileset->GetGridSize();

    int tile_x = static_cast<int>(local_pos.x()) / tile_size.x;
    int tile_y = static_cast<int>(local_pos.y()) / tile_size.y;

    if (tile_x >= 0 && tile_x < grid_size.x && tile_y >= 0 && tile_y < grid_size.y) {
        return tile_y * grid_size.x + tile_x;
    }

    return -1;
}

// Tilemap25DPainterDialog Implementation
Tilemap25DPainterDialog::Tilemap25DPainterDialog(QWidget* parent)
    : QDialog(parent)
    , m_mainLayout(nullptr)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_mainSplitter(nullptr)
    , m_canvas(nullptr)
    , m_toolPanel(nullptr)
    , m_paletteWidget(nullptr)
    , m_tilesetList(nullptr)
    , m_toolCombo(nullptr)
    , m_selectionModeCombo(nullptr)
    , m_gizmoModeCombo(nullptr)
    , m_gridSizeSlider(nullptr)
    , m_gridSizeSpinBox(nullptr)
    , m_showGridCheck(nullptr)
    , m_snapToGridCheck(nullptr)
    , m_snapXAxisCheck(nullptr)
    , m_snapYAxisCheck(nullptr)
    , m_snapZAxisCheck(nullptr)
    , m_gridSizeXSpinBox(nullptr)
    , m_gridSizeYSpinBox(nullptr)
    , m_gridSizeZSpinBox(nullptr)
    , m_snapToEdgesCheck(nullptr)
    , m_edgeSnapDistanceSpinBox(nullptr)
    , m_faceCountLabel(nullptr)
    , m_selectionInfoLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_toolGroup(nullptr)
    , m_selectionModeGroup(nullptr)
    , m_gizmoModeGroup(nullptr)
    , m_modified(false)
    , m_nextTilesetId(1)
{
    setWindowTitle("Tilemap 2.5D Painter");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    setupUI();

    // Initialize with default settings
    m_canvas->SetGridSize(1.0f);
    m_canvas->SetShowGrid(true);
    m_canvas->SetSnapToGrid(true);

    updateWindowTitle();
    updateToolStates();
}

Tilemap25DPainterDialog::~Tilemap25DPainterDialog() {
    // Cleanup is handled by Qt's parent-child relationship
}

void Tilemap25DPainterDialog::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    setupMenuBar();
    setupToolBar();
    setupMainPanels();
    setupStatusBar();
}

void Tilemap25DPainterDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    m_mainLayout->addWidget(m_menuBar);

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    m_newAction = fileMenu->addAction("&New Project", this, &Tilemap25DPainterDialog::onNewProject, QKeySequence::New);
    m_openAction = fileMenu->addAction("&Open Project...", this, &Tilemap25DPainterDialog::onOpenProject, QKeySequence::Open);
    fileMenu->addSeparator();
    m_saveAction = fileMenu->addAction("&Save Project", this, &Tilemap25DPainterDialog::onSaveProject, QKeySequence::Save);
    m_saveAsAction = fileMenu->addAction("Save Project &As...", this, &Tilemap25DPainterDialog::onSaveProjectAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    m_exportOBJAction = fileMenu->addAction("&Export OBJ...", this, &Tilemap25DPainterDialog::onExportOBJ);
    fileMenu->addSeparator();
    m_exitAction = fileMenu->addAction("E&xit", this, &Tilemap25DPainterDialog::onExit);

    // Edit menu
    QMenu* editMenu = m_menuBar->addMenu("&Edit");
    m_undoAction = editMenu->addAction("&Undo", this, &Tilemap25DPainterDialog::onUndo, QKeySequence::Undo);
    m_redoAction = editMenu->addAction("&Redo", this, &Tilemap25DPainterDialog::onRedo, QKeySequence::Redo);
    editMenu->addSeparator();
    m_cutAction = editMenu->addAction("Cu&t", this, &Tilemap25DPainterDialog::onCut, QKeySequence::Cut);
    m_copyAction = editMenu->addAction("&Copy", this, &Tilemap25DPainterDialog::onCopy, QKeySequence::Copy);
    m_pasteAction = editMenu->addAction("&Paste", this, &Tilemap25DPainterDialog::onPaste, QKeySequence::Paste);
    m_deleteAction = editMenu->addAction("&Delete", this, &Tilemap25DPainterDialog::onDelete, QKeySequence::Delete);
    editMenu->addSeparator();
    m_selectAllAction = editMenu->addAction("Select &All", this, &Tilemap25DPainterDialog::onSelectAll, QKeySequence::SelectAll);
    m_deselectAllAction = editMenu->addAction("&Deselect All", this, &Tilemap25DPainterDialog::onDeselectAll);

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    m_resetViewAction = viewMenu->addAction("&Reset View", this, &Tilemap25DPainterDialog::onResetView);
    m_fitToViewAction = viewMenu->addAction("&Fit to View", this, &Tilemap25DPainterDialog::onFitToView);
    viewMenu->addSeparator();
    m_zoomInAction = viewMenu->addAction("Zoom &In", this, &Tilemap25DPainterDialog::onZoomIn, QKeySequence::ZoomIn);
    m_zoomOutAction = viewMenu->addAction("Zoom &Out", this, &Tilemap25DPainterDialog::onZoomOut, QKeySequence::ZoomOut);
    viewMenu->addSeparator();
    m_toggleGridAction = viewMenu->addAction("Show &Grid", this, &Tilemap25DPainterDialog::onToggleGrid);
    m_toggleGridAction->setCheckable(true);
    m_toggleGridAction->setChecked(true);
    m_toggleSnapAction = viewMenu->addAction("&Snap to Grid", this, &Tilemap25DPainterDialog::onToggleSnap);
    m_toggleSnapAction->setCheckable(true);
    m_toggleSnapAction->setChecked(true);

    // Tools menu
    QMenu* toolsMenu = m_menuBar->addMenu("&Tools");
    m_paintToolAction = toolsMenu->addAction("&Paint Tool", this, &Tilemap25DPainterDialog::onPaintTool);
    m_paintToolAction->setCheckable(true);
    m_paintToolAction->setChecked(true);
    m_eraseToolAction = toolsMenu->addAction("&Erase Tool", this, &Tilemap25DPainterDialog::onEraseTool);
    m_eraseToolAction->setCheckable(true);
    m_selectToolAction = toolsMenu->addAction("&Select Tool", this, &Tilemap25DPainterDialog::onSelectTool);
    m_selectToolAction->setCheckable(true);
    m_eyedropperToolAction = toolsMenu->addAction("E&yedropper Tool", this, &Tilemap25DPainterDialog::onEyedropperTool);
    m_eyedropperToolAction->setCheckable(true);

    // Create tool action group
    m_toolGroup = new QActionGroup(this);
    m_toolGroup->addAction(m_paintToolAction);
    m_toolGroup->addAction(m_eraseToolAction);
    m_toolGroup->addAction(m_selectToolAction);
    m_toolGroup->addAction(m_eyedropperToolAction);

    toolsMenu->addSeparator();

    // Selection mode submenu
    QMenu* selectionModeMenu = toolsMenu->addMenu("Selection &Mode");
    m_faceSelectionAction = selectionModeMenu->addAction("&Face Selection", this, &Tilemap25DPainterDialog::onFaceSelectionMode);
    m_faceSelectionAction->setCheckable(true);
    m_faceSelectionAction->setChecked(true);
    m_edgeSelectionAction = selectionModeMenu->addAction("&Edge Selection", this, &Tilemap25DPainterDialog::onEdgeSelectionMode);
    m_edgeSelectionAction->setCheckable(true);
    m_vertexSelectionAction = selectionModeMenu->addAction("&Vertex Selection", this, &Tilemap25DPainterDialog::onVertexSelectionMode);
    m_vertexSelectionAction->setCheckable(true);

    // Create selection mode action group
    m_selectionModeGroup = new QActionGroup(this);
    m_selectionModeGroup->addAction(m_faceSelectionAction);
    m_selectionModeGroup->addAction(m_edgeSelectionAction);
    m_selectionModeGroup->addAction(m_vertexSelectionAction);

    // Gizmo mode submenu
    QMenu* gizmoModeMenu = toolsMenu->addMenu("&Gizmo Mode");
    m_moveGizmoAction = gizmoModeMenu->addAction("&Move Gizmo", this, &Tilemap25DPainterDialog::onMoveGizmo);
    m_moveGizmoAction->setCheckable(true);
    m_rotateGizmoAction = gizmoModeMenu->addAction("&Rotate Gizmo", this, &Tilemap25DPainterDialog::onRotateGizmo);
    m_rotateGizmoAction->setCheckable(true);
    m_scaleGizmoAction = gizmoModeMenu->addAction("&Scale Gizmo", this, &Tilemap25DPainterDialog::onScaleGizmo);
    m_scaleGizmoAction->setCheckable(true);

    // Create gizmo mode action group
    m_gizmoModeGroup = new QActionGroup(this);
    m_gizmoModeGroup->addAction(m_moveGizmoAction);
    m_gizmoModeGroup->addAction(m_rotateGizmoAction);
    m_gizmoModeGroup->addAction(m_scaleGizmoAction);
    m_gizmoModeGroup->setExclusive(false); // Allow no gizmo to be selected
}

void Tilemap25DPainterDialog::setupToolBar() {
    m_toolBar = new QToolBar("Main Toolbar", this);
    m_mainLayout->addWidget(m_toolBar);

    // File operations
    m_toolBar->addAction(m_newAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addSeparator();

    // Tools
    m_toolBar->addAction(m_paintToolAction);
    m_toolBar->addAction(m_eraseToolAction);
    m_toolBar->addAction(m_selectToolAction);
    m_toolBar->addAction(m_eyedropperToolAction);
    m_toolBar->addSeparator();

    // Selection modes
    m_toolBar->addAction(m_faceSelectionAction);
    m_toolBar->addAction(m_edgeSelectionAction);
    m_toolBar->addAction(m_vertexSelectionAction);
    m_toolBar->addSeparator();

    // Gizmo modes
    m_toolBar->addAction(m_moveGizmoAction);
    m_toolBar->addAction(m_rotateGizmoAction);
    m_toolBar->addAction(m_scaleGizmoAction);
    m_toolBar->addSeparator();

    // Grid controls
    m_toolBar->addAction(m_toggleGridAction);
    m_toolBar->addAction(m_toggleSnapAction);
}

void Tilemap25DPainterDialog::setupMainPanels() {
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainLayout->addWidget(m_mainSplitter);

    setupToolPanel();
    setupViewportPanel();

    // Set splitter proportions
    m_mainSplitter->setSizes({300, 1100});
}

void Tilemap25DPainterDialog::setupToolPanel() {
    m_toolPanel = new QWidget();
    m_mainSplitter->addWidget(m_toolPanel);

    QVBoxLayout* toolLayout = new QVBoxLayout(m_toolPanel);

    // Tool selection
    QGroupBox* toolGroup = new QGroupBox("Tools");
    QVBoxLayout* toolGroupLayout = new QVBoxLayout(toolGroup);

    m_toolCombo = new QComboBox();
    m_toolCombo->addItem("Paint", static_cast<int>(Tilemap25DPaintTool::Paint));
    m_toolCombo->addItem("Erase", static_cast<int>(Tilemap25DPaintTool::Erase));
    m_toolCombo->addItem("Select", static_cast<int>(Tilemap25DPaintTool::Select));
    m_toolCombo->addItem("Eyedropper", static_cast<int>(Tilemap25DPaintTool::Eyedropper));
    toolGroupLayout->addWidget(m_toolCombo);

    m_selectionModeCombo = new QComboBox();
    m_selectionModeCombo->addItem("Face Selection", static_cast<int>(Tilemap25DSelectionMode::Face));
    m_selectionModeCombo->addItem("Edge Selection", static_cast<int>(Tilemap25DSelectionMode::Edge));
    m_selectionModeCombo->addItem("Vertex Selection", static_cast<int>(Tilemap25DSelectionMode::Vertex));
    toolGroupLayout->addWidget(m_selectionModeCombo);

    m_gizmoModeCombo = new QComboBox();
    m_gizmoModeCombo->addItem("No Gizmo", static_cast<int>(Tilemap25DGizmoMode::None));
    m_gizmoModeCombo->addItem("Move Gizmo", static_cast<int>(Tilemap25DGizmoMode::Move));
    m_gizmoModeCombo->addItem("Rotate Gizmo", static_cast<int>(Tilemap25DGizmoMode::Rotate));
    m_gizmoModeCombo->addItem("Scale Gizmo", static_cast<int>(Tilemap25DGizmoMode::Scale));
    toolGroupLayout->addWidget(m_gizmoModeCombo);

    toolLayout->addWidget(toolGroup);

    // Grid settings
    QGroupBox* gridGroup = new QGroupBox("Grid Settings");
    QVBoxLayout* gridGroupLayout = new QVBoxLayout(gridGroup);

    QHBoxLayout* gridSizeLayout = new QHBoxLayout();
    gridSizeLayout->addWidget(new QLabel("Grid Size:"));
    m_gridSizeSlider = new QSlider(Qt::Horizontal);
    m_gridSizeSlider->setRange(1, 100);
    m_gridSizeSlider->setValue(10);
    m_gridSizeSpinBox = new QDoubleSpinBox();
    m_gridSizeSpinBox->setRange(0.1, 10.0);
    m_gridSizeSpinBox->setSingleStep(0.1);
    m_gridSizeSpinBox->setValue(1.0);
    gridSizeLayout->addWidget(m_gridSizeSlider);
    gridSizeLayout->addWidget(m_gridSizeSpinBox);
    gridGroupLayout->addLayout(gridSizeLayout);

    m_showGridCheck = new QCheckBox("Show Grid");
    m_showGridCheck->setChecked(true);
    gridGroupLayout->addWidget(m_showGridCheck);

    m_snapToGridCheck = new QCheckBox("Snap to Grid");
    m_snapToGridCheck->setChecked(true);
    gridGroupLayout->addWidget(m_snapToGridCheck);

    // Per-axis snapping controls
    QGroupBox* axisSnapGroup = new QGroupBox("Axis Snapping");
    QGridLayout* axisSnapLayout = new QGridLayout(axisSnapGroup);

    m_snapXAxisCheck = new QCheckBox("Snap X");
    m_snapXAxisCheck->setChecked(true);
    m_snapYAxisCheck = new QCheckBox("Snap Y");
    m_snapYAxisCheck->setChecked(true);
    m_snapZAxisCheck = new QCheckBox("Snap Z");
    m_snapZAxisCheck->setChecked(true);

    axisSnapLayout->addWidget(m_snapXAxisCheck, 0, 0);
    axisSnapLayout->addWidget(m_snapYAxisCheck, 0, 1);
    axisSnapLayout->addWidget(m_snapZAxisCheck, 0, 2);

    // Grid size per axis
    axisSnapLayout->addWidget(new QLabel("X Size:"), 1, 0);
    m_gridSizeXSpinBox = new QDoubleSpinBox();
    m_gridSizeXSpinBox->setRange(0.1, 10.0);
    m_gridSizeXSpinBox->setSingleStep(0.1);
    m_gridSizeXSpinBox->setValue(1.0);
    axisSnapLayout->addWidget(m_gridSizeXSpinBox, 2, 0);

    axisSnapLayout->addWidget(new QLabel("Y Size:"), 1, 1);
    m_gridSizeYSpinBox = new QDoubleSpinBox();
    m_gridSizeYSpinBox->setRange(0.1, 10.0);
    m_gridSizeYSpinBox->setSingleStep(0.1);
    m_gridSizeYSpinBox->setValue(1.0);
    axisSnapLayout->addWidget(m_gridSizeYSpinBox, 2, 1);

    axisSnapLayout->addWidget(new QLabel("Z Size:"), 1, 2);
    m_gridSizeZSpinBox = new QDoubleSpinBox();
    m_gridSizeZSpinBox->setRange(0.1, 10.0);
    m_gridSizeZSpinBox->setSingleStep(0.1);
    m_gridSizeZSpinBox->setValue(1.0);
    axisSnapLayout->addWidget(m_gridSizeZSpinBox, 2, 2);

    gridGroupLayout->addWidget(axisSnapGroup);

    // Edge snapping controls
    QGroupBox* edgeSnapGroup = new QGroupBox("Edge Snapping");
    QVBoxLayout* edgeSnapLayout = new QVBoxLayout(edgeSnapGroup);

    m_snapToEdgesCheck = new QCheckBox("Snap to Edges");
    m_snapToEdgesCheck->setChecked(false);
    edgeSnapLayout->addWidget(m_snapToEdgesCheck);

    QHBoxLayout* edgeDistanceLayout = new QHBoxLayout();
    edgeDistanceLayout->addWidget(new QLabel("Snap Distance:"));
    m_edgeSnapDistanceSpinBox = new QDoubleSpinBox();
    m_edgeSnapDistanceSpinBox->setRange(0.1, 5.0);
    m_edgeSnapDistanceSpinBox->setSingleStep(0.1);
    m_edgeSnapDistanceSpinBox->setValue(0.5);
    edgeDistanceLayout->addWidget(m_edgeSnapDistanceSpinBox);
    edgeSnapLayout->addLayout(edgeDistanceLayout);

    gridGroupLayout->addWidget(edgeSnapGroup);

    toolLayout->addWidget(gridGroup);

    // Tileset management
    QGroupBox* tilesetGroup = new QGroupBox("Tilesets");
    QVBoxLayout* tilesetGroupLayout = new QVBoxLayout(tilesetGroup);

    QHBoxLayout* tilesetButtonLayout = new QHBoxLayout();
    QPushButton* loadTilesetBtn = new QPushButton("Load Tileset");
    QPushButton* removeTilesetBtn = new QPushButton("Remove Tileset");
    tilesetButtonLayout->addWidget(loadTilesetBtn);
    tilesetButtonLayout->addWidget(removeTilesetBtn);
    tilesetGroupLayout->addLayout(tilesetButtonLayout);

    m_tilesetList = new QListWidget();
    tilesetGroupLayout->addWidget(m_tilesetList);

    toolLayout->addWidget(tilesetGroup);

    // Tileset palette
    QGroupBox* paletteGroup = new QGroupBox("Tile Palette");
    QVBoxLayout* paletteGroupLayout = new QVBoxLayout(paletteGroup);

    m_paletteWidget = new Tilemap25DPalette();
    m_paletteWidget->setMinimumHeight(200);
    paletteGroupLayout->addWidget(m_paletteWidget);

    toolLayout->addWidget(paletteGroup);

    toolLayout->addStretch();

    // Connect signals
    connect(m_toolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                Tilemap25DPaintTool tool = static_cast<Tilemap25DPaintTool>(m_toolCombo->itemData(index).toInt());
                m_canvas->SetCurrentTool(tool);
            });

    connect(m_selectionModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                Tilemap25DSelectionMode mode = static_cast<Tilemap25DSelectionMode>(m_selectionModeCombo->itemData(index).toInt());
                m_canvas->SetSelectionMode(mode);
            });

    connect(m_gizmoModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int index) {
                Tilemap25DGizmoMode mode = static_cast<Tilemap25DGizmoMode>(m_gizmoModeCombo->itemData(index).toInt());
                m_canvas->SetGizmoMode(mode);
            });

    connect(m_gridSizeSlider, &QSlider::valueChanged, this, &Tilemap25DPainterDialog::onGridSizeChanged);
    connect(m_gridSizeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &Tilemap25DPainterDialog::onGridSizeChanged);
    connect(m_showGridCheck, &QCheckBox::toggled, this, &Tilemap25DPainterDialog::onShowGridChanged);
    connect(m_snapToGridCheck, &QCheckBox::toggled, this, &Tilemap25DPainterDialog::onSnapToGridChanged);

    // Per-axis snapping connections
    connect(m_snapXAxisCheck, &QCheckBox::toggled, this, &Tilemap25DPainterDialog::onSnapXAxisChanged);
    connect(m_snapYAxisCheck, &QCheckBox::toggled, this, &Tilemap25DPainterDialog::onSnapYAxisChanged);
    connect(m_snapZAxisCheck, &QCheckBox::toggled, this, &Tilemap25DPainterDialog::onSnapZAxisChanged);
    connect(m_gridSizeXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &Tilemap25DPainterDialog::onGridSizePerAxisChanged);
    connect(m_gridSizeYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &Tilemap25DPainterDialog::onGridSizePerAxisChanged);
    connect(m_gridSizeZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &Tilemap25DPainterDialog::onGridSizePerAxisChanged);

    // Edge snapping connections
    connect(m_snapToEdgesCheck, &QCheckBox::toggled, this, &Tilemap25DPainterDialog::onSnapToEdgesChanged);
    connect(m_edgeSnapDistanceSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &Tilemap25DPainterDialog::onEdgeSnapDistanceChanged);

    connect(loadTilesetBtn, &QPushButton::clicked, this, &Tilemap25DPainterDialog::onLoadTileset);
    connect(removeTilesetBtn, &QPushButton::clicked, this, &Tilemap25DPainterDialog::onRemoveTileset);
    connect(m_tilesetList, &QListWidget::currentRowChanged, this, &Tilemap25DPainterDialog::onTilesetSelectionChanged);

    connect(m_paletteWidget, &Tilemap25DPalette::tileSelected, this, &Tilemap25DPainterDialog::onTileSelected);
}

void Tilemap25DPainterDialog::setupViewportPanel() {
    m_canvas = new Tilemap25DCanvas();
    m_mainSplitter->addWidget(m_canvas);

    // Connect canvas signals
    connect(m_canvas, &Tilemap25DCanvas::facePainted, this, &Tilemap25DPainterDialog::onFacePainted);
    connect(m_canvas, &Tilemap25DCanvas::faceErased, this, &Tilemap25DPainterDialog::onFaceErased);
    connect(m_canvas, &Tilemap25DCanvas::selectionChanged, this, &Tilemap25DPainterDialog::onSelectionChanged);
    connect(m_canvas, &Tilemap25DCanvas::sceneModified, this, &Tilemap25DPainterDialog::onSceneModified);
}

void Tilemap25DPainterDialog::setupStatusBar() {
    QWidget* statusWidget = new QWidget();
    QHBoxLayout* statusLayout = new QHBoxLayout(statusWidget);
    statusLayout->setContentsMargins(5, 2, 5, 2);

    m_faceCountLabel = new QLabel("Faces: 0");
    m_selectionInfoLabel = new QLabel("Selection: None");
    m_statusLabel = new QLabel("Ready");

    statusLayout->addWidget(m_faceCountLabel);
    statusLayout->addWidget(new QLabel("|"));
    statusLayout->addWidget(m_selectionInfoLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_statusLabel);

    m_mainLayout->addWidget(statusWidget);
}

void Tilemap25DPainterDialog::updateWindowTitle() {
    QString title = "Tilemap 2.5D Painter";

    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.baseName();
    } else {
        title += " - Untitled";
    }

    if (m_modified) {
        title += "*";
    }

    setWindowTitle(title);
}

void Tilemap25DPainterDialog::updateToolStates() {
    // Update tool action states based on current tool
    // This is handled by the action groups

    // Update combo boxes to match action states
    if (m_paintToolAction->isChecked()) {
        m_toolCombo->setCurrentIndex(0);
    } else if (m_eraseToolAction->isChecked()) {
        m_toolCombo->setCurrentIndex(1);
    } else if (m_selectToolAction->isChecked()) {
        m_toolCombo->setCurrentIndex(2);
    } else if (m_eyedropperToolAction->isChecked()) {
        m_toolCombo->setCurrentIndex(3);
    }

    // Update selection mode combo
    if (m_faceSelectionAction->isChecked()) {
        m_selectionModeCombo->setCurrentIndex(0);
    } else if (m_edgeSelectionAction->isChecked()) {
        m_selectionModeCombo->setCurrentIndex(1);
    } else if (m_vertexSelectionAction->isChecked()) {
        m_selectionModeCombo->setCurrentIndex(2);
    }

    // Update gizmo mode combo
    if (m_moveGizmoAction->isChecked()) {
        m_gizmoModeCombo->setCurrentIndex(1);
    } else if (m_rotateGizmoAction->isChecked()) {
        m_gizmoModeCombo->setCurrentIndex(2);
    } else if (m_scaleGizmoAction->isChecked()) {
        m_gizmoModeCombo->setCurrentIndex(3);
    } else {
        m_gizmoModeCombo->setCurrentIndex(0);
    }
}

void Tilemap25DPainterDialog::updateSelectionInfo() {
    if (!m_canvas) return;

    const auto& selected_faces = m_canvas->GetSelectedFaces();
    const auto& selected_vertices = m_canvas->GetSelectedVertices();
    const auto& selected_edges = m_canvas->GetSelectedEdges();

    QString info;
    if (!selected_faces.empty()) {
        info = QString("Faces: %1").arg(selected_faces.size());
    } else if (!selected_vertices.empty()) {
        info = QString("Vertices: %1").arg(selected_vertices.size());
    } else if (!selected_edges.empty()) {
        info = QString("Edges: %1").arg(selected_edges.size());
    } else {
        info = "None";
    }

    m_selectionInfoLabel->setText("Selection: " + info);

    // Update face count
    const auto& faces = m_canvas->GetFaces();
    m_faceCountLabel->setText(QString("Faces: %1").arg(faces.size()));
}

bool Tilemap25DPainterDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool Tilemap25DPainterDialog::promptSaveChanges() {
    if (!hasUnsavedChanges()) {
        return true;
    }

    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<Tilemap25DPainterDialog*>(this),
        "Unsaved Changes",
        "The project has unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save
    );

    switch (result) {
        case QMessageBox::Save:
            if (m_currentFilePath.isEmpty()) {
                return onSaveProjectAs();
            } else {
                return saveProject(m_currentFilePath);
            }
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
        default:
            return false;
    }
}

void Tilemap25DPainterDialog::setModified(bool modified) {
    if (m_modified != modified) {
        m_modified = modified;
        updateWindowTitle();
    }
}

void Tilemap25DPainterDialog::closeEvent(QCloseEvent* event) {
    if (promptSaveChanges()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void Tilemap25DPainterDialog::keyPressEvent(QKeyEvent* event) {
    // Forward key events to canvas for grid manipulation
    if (m_canvas && (event->modifiers() & Qt::ShiftModifier) &&
        (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
         event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)) {
        m_canvas->HandleKeyPress(event);
        return;
    }

    QDialog::keyPressEvent(event);
}

// Slot implementations
void Tilemap25DPainterDialog::onNewProject() {
    if (promptSaveChanges()) {
        newProject();
    }
}

void Tilemap25DPainterDialog::onOpenProject() {
    if (!promptSaveChanges()) {
        return;
    }

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Open Tilemap 2.5D Project",
        QString(),
        "Tilemap 2.5D Projects (*.tm25d);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        loadProject(filepath);
    }
}

void Tilemap25DPainterDialog::onSaveProject() {
    if (m_currentFilePath.isEmpty()) {
        onSaveProjectAs();
    } else {
        saveProject(m_currentFilePath);
    }
}

bool Tilemap25DPainterDialog::onSaveProjectAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Tilemap 2.5D Project",
        QString(),
        "Tilemap 2.5D Projects (*.tm25d);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        return saveProject(filepath);
    }

    return false;
}

void Tilemap25DPainterDialog::onExportOBJ() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Export OBJ",
        QString(),
        "Wavefront OBJ (*.obj);;All Files (*)"
    );

    if (!filepath.isEmpty() && m_canvas) {
        QProgressDialog progress("Exporting OBJ...", "Cancel", 0, 100, this);
        progress.setWindowModality(Qt::WindowModal);
        progress.show();

        QApplication::processEvents();

        bool success = m_canvas->ExportToOBJ(filepath, true);

        progress.close();

        if (success) {
            QMessageBox::information(this, "Export Complete",
                                   "OBJ file exported successfully!");
        } else {
            QMessageBox::warning(this, "Export Failed",
                               "Failed to export OBJ file.");
        }
    }
}

void Tilemap25DPainterDialog::onExit() {
    close();
}

// Edit menu slots
void Tilemap25DPainterDialog::onUndo() {
    // TODO: Implement undo system
    m_statusLabel->setText("Undo not implemented yet");
}

void Tilemap25DPainterDialog::onRedo() {
    // TODO: Implement redo system
    m_statusLabel->setText("Redo not implemented yet");
}

void Tilemap25DPainterDialog::onCut() {
    // TODO: Implement cut operation
    m_statusLabel->setText("Cut not implemented yet");
}

void Tilemap25DPainterDialog::onCopy() {
    // TODO: Implement copy operation
    m_statusLabel->setText("Copy not implemented yet");
}

void Tilemap25DPainterDialog::onPaste() {
    // TODO: Implement paste operation
    m_statusLabel->setText("Paste not implemented yet");
}

void Tilemap25DPainterDialog::onDelete() {
    if (m_canvas) {
        // Delete selected faces
        const auto& selected_faces = m_canvas->GetSelectedFaces();
        for (int i = static_cast<int>(selected_faces.size()) - 1; i >= 0; --i) {
            // The canvas will handle updating selection indices
        }
        setModified(true);
    }
}

void Tilemap25DPainterDialog::onSelectAll() {
    if (m_canvas) {
        // Select all faces
        const auto& faces = m_canvas->GetFaces();
        for (size_t i = 0; i < faces.size(); ++i) {
            // Implementation would need to be added to canvas
        }
    }
}

void Tilemap25DPainterDialog::onDeselectAll() {
    if (m_canvas) {
        m_canvas->ClearSelection();
    }
}

// View menu slots
void Tilemap25DPainterDialog::onResetView() {
    // TODO: Reset camera to default position
    m_statusLabel->setText("View reset");
}

void Tilemap25DPainterDialog::onFitToView() {
    // TODO: Fit all faces in view
    m_statusLabel->setText("Fit to view");
}

void Tilemap25DPainterDialog::onZoomIn() {
    // TODO: Zoom in camera
    m_statusLabel->setText("Zoom in");
}

void Tilemap25DPainterDialog::onZoomOut() {
    // TODO: Zoom out camera
    m_statusLabel->setText("Zoom out");
}

void Tilemap25DPainterDialog::onToggleGrid() {
    bool show_grid = m_toggleGridAction->isChecked();
    if (m_canvas) {
        m_canvas->SetShowGrid(show_grid);
    }
    if (m_showGridCheck) {
        m_showGridCheck->setChecked(show_grid);
    }
}

void Tilemap25DPainterDialog::onToggleSnap() {
    bool snap_to_grid = m_toggleSnapAction->isChecked();
    if (m_canvas) {
        m_canvas->SetSnapToGrid(snap_to_grid);
    }
    if (m_snapToGridCheck) {
        m_snapToGridCheck->setChecked(snap_to_grid);
    }
}

// Tool menu slots
void Tilemap25DPainterDialog::onPaintTool() {
    if (m_canvas) {
        m_canvas->SetCurrentTool(Tilemap25DPaintTool::Paint);
    }
    updateToolStates();
}

void Tilemap25DPainterDialog::onEraseTool() {
    if (m_canvas) {
        m_canvas->SetCurrentTool(Tilemap25DPaintTool::Erase);
    }
    updateToolStates();
}

void Tilemap25DPainterDialog::onSelectTool() {
    if (m_canvas) {
        m_canvas->SetCurrentTool(Tilemap25DPaintTool::Select);
    }
    updateToolStates();
}

void Tilemap25DPainterDialog::onEyedropperTool() {
    if (m_canvas) {
        m_canvas->SetCurrentTool(Tilemap25DPaintTool::Eyedropper);
    }
    updateToolStates();
}

// Selection mode slots
void Tilemap25DPainterDialog::onFaceSelectionMode() {
    if (m_canvas) {
        m_canvas->SetSelectionMode(Tilemap25DSelectionMode::Face);
    }
    updateToolStates();
}

void Tilemap25DPainterDialog::onEdgeSelectionMode() {
    if (m_canvas) {
        m_canvas->SetSelectionMode(Tilemap25DSelectionMode::Edge);
    }
    updateToolStates();
}

void Tilemap25DPainterDialog::onVertexSelectionMode() {
    if (m_canvas) {
        m_canvas->SetSelectionMode(Tilemap25DSelectionMode::Vertex);
    }
    updateToolStates();
}

// Gizmo mode slots
void Tilemap25DPainterDialog::onMoveGizmo() {
    if (m_canvas) {
        m_canvas->SetGizmoMode(m_moveGizmoAction->isChecked() ? Tilemap25DGizmoMode::Move : Tilemap25DGizmoMode::None);
    }
    updateToolStates();
}

void Tilemap25DPainterDialog::onRotateGizmo() {
    if (m_canvas) {
        m_canvas->SetGizmoMode(m_rotateGizmoAction->isChecked() ? Tilemap25DGizmoMode::Rotate : Tilemap25DGizmoMode::None);
    }
    updateToolStates();
}

void Tilemap25DPainterDialog::onScaleGizmo() {
    if (m_canvas) {
        m_canvas->SetGizmoMode(m_scaleGizmoAction->isChecked() ? Tilemap25DGizmoMode::Scale : Tilemap25DGizmoMode::None);
    }
    updateToolStates();
}

// Tileset management slots
void Tilemap25DPainterDialog::onLoadTileset() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Load Tileset",
        QString(),
        "Tileset Files (*.tileset);;All Files (*)"
    );

    if (!filepath.isEmpty()) {
        int tileset_id = m_nextTilesetId++;

        if (m_canvas) {
            m_canvas->LoadTileset(tileset_id, filepath);
        }

        // Add to tileset list
        QFileInfo fileInfo(filepath);
        QListWidgetItem* item = new QListWidgetItem(fileInfo.baseName());
        item->setData(Qt::UserRole, tileset_id);
        m_tilesetList->addItem(item);

        // Store tileset info
        m_tilesetPaths[tileset_id] = filepath;

        // Select the new tileset
        m_tilesetList->setCurrentItem(item);

        setModified(true);
    }
}

void Tilemap25DPainterDialog::onRemoveTileset() {
    QListWidgetItem* current_item = m_tilesetList->currentItem();
    if (!current_item) return;

    int tileset_id = current_item->data(Qt::UserRole).toInt();

    // Remove from canvas
    // TODO: Implement tileset removal in canvas

    // Remove from lists
    m_tilesetPaths.erase(tileset_id);
    m_tilesets.erase(tileset_id);

    // Remove from UI
    delete current_item;

    // Clear palette if this was the selected tileset
    if (m_paletteWidget) {
        m_paletteWidget->ClearTileset();
    }

    setModified(true);
}

void Tilemap25DPainterDialog::onTilesetSelectionChanged() {
    QListWidgetItem* current_item = m_tilesetList->currentItem();
    if (!current_item || !m_paletteWidget || !m_canvas) return;

    int tileset_id = current_item->data(Qt::UserRole).toInt();
    auto tileset = m_canvas->GetTileset(tileset_id);

    if (tileset) {
        m_paletteWidget->SetTileset(tileset_id, tileset);
    }
}

// Canvas interaction slots
void Tilemap25DPainterDialog::onFacePainted(int face_index) {
    setModified(true);
    updateSelectionInfo();
    m_statusLabel->setText(QString("Face painted at index %1").arg(face_index));
}

void Tilemap25DPainterDialog::onFaceErased(int face_index) {
    setModified(true);
    updateSelectionInfo();
    m_statusLabel->setText(QString("Face erased at index %1").arg(face_index));
}

void Tilemap25DPainterDialog::onSelectionChanged() {
    updateSelectionInfo();
}

void Tilemap25DPainterDialog::onSceneModified() {
    setModified(true);
}

void Tilemap25DPainterDialog::onTileSelected(int tileset_id, int tile_id) {
    if (m_canvas) {
        m_canvas->SetCurrentTile(tileset_id, tile_id);
    }
    m_statusLabel->setText(QString("Selected tile %1 from tileset %2").arg(tile_id).arg(tileset_id));
}

// Grid control slots
void Tilemap25DPainterDialog::onGridSizeChanged() {
    float grid_size = static_cast<float>(m_gridSizeSpinBox->value());

    // Sync slider and spinbox
    m_gridSizeSlider->blockSignals(true);
    m_gridSizeSlider->setValue(static_cast<int>(grid_size * 10));
    m_gridSizeSlider->blockSignals(false);

    m_gridSizeSpinBox->blockSignals(true);
    m_gridSizeSpinBox->setValue(grid_size);
    m_gridSizeSpinBox->blockSignals(false);

    if (m_canvas) {
        m_canvas->SetGridSize(grid_size);
    }
}

void Tilemap25DPainterDialog::onShowGridChanged() {
    bool show_grid = m_showGridCheck->isChecked();
    if (m_canvas) {
        m_canvas->SetShowGrid(show_grid);
    }
    if (m_toggleGridAction) {
        m_toggleGridAction->setChecked(show_grid);
    }
}

void Tilemap25DPainterDialog::onSnapToGridChanged() {
    bool snap_to_grid = m_snapToGridCheck->isChecked();
    if (m_canvas) {
        m_canvas->SetSnapToGrid(snap_to_grid);
    }
    if (m_toggleSnapAction) {
        m_toggleSnapAction->setChecked(snap_to_grid);
    }
}

void Tilemap25DPainterDialog::onSnapXAxisChanged() {
    bool snap_x = m_snapXAxisCheck->isChecked();
    if (m_canvas) {
        m_canvas->SetSnapXAxis(snap_x);
    }
}

void Tilemap25DPainterDialog::onSnapYAxisChanged() {
    bool snap_y = m_snapYAxisCheck->isChecked();
    if (m_canvas) {
        m_canvas->SetSnapYAxis(snap_y);
    }
}

void Tilemap25DPainterDialog::onSnapZAxisChanged() {
    bool snap_z = m_snapZAxisCheck->isChecked();
    if (m_canvas) {
        m_canvas->SetSnapZAxis(snap_z);
    }
}

void Tilemap25DPainterDialog::onGridSizePerAxisChanged() {
    glm::vec3 grid_size(
        static_cast<float>(m_gridSizeXSpinBox->value()),
        static_cast<float>(m_gridSizeYSpinBox->value()),
        static_cast<float>(m_gridSizeZSpinBox->value())
    );

    if (m_canvas) {
        m_canvas->SetGridSizePerAxis(grid_size);
    }
}

void Tilemap25DPainterDialog::onSnapToEdgesChanged() {
    bool snap_to_edges = m_snapToEdgesCheck->isChecked();
    if (m_canvas) {
        m_canvas->SetSnapToEdges(snap_to_edges);
    }
}

void Tilemap25DPainterDialog::onEdgeSnapDistanceChanged() {
    float distance = static_cast<float>(m_edgeSnapDistanceSpinBox->value());
    if (m_canvas) {
        m_canvas->SetEdgeSnapDistance(distance);
    }
}

// Project management methods
void Tilemap25DPainterDialog::newProject() {
    m_currentFilePath.clear();
    m_modified = false;

    if (m_canvas) {
        m_canvas->ClearFaces();
    }

    // Clear tilesets
    m_tilesetList->clear();
    m_tilesetPaths.clear();
    m_tilesets.clear();
    m_nextTilesetId = 1;

    if (m_paletteWidget) {
        m_paletteWidget->ClearTileset();
    }

    updateWindowTitle();
    updateSelectionInfo();
    m_statusLabel->setText("New project created");
}

bool Tilemap25DPainterDialog::saveProject(const QString& filepath) {
    // TODO: Implement project saving
    // This would save faces, tilesets, and other project data to a JSON file

    QJsonObject project;
    project["version"] = "1.0";
    project["face_count"] = static_cast<int>(m_canvas ? m_canvas->GetFaces().size() : 0);

    // Save faces
    QJsonArray faces_array;
    if (m_canvas) {
        const auto& faces = m_canvas->GetFaces();
        for (const auto& face : faces) {
            QJsonObject face_obj;
            face_obj["tileset_id"] = face.tileset_id;
            face_obj["tile_id"] = face.tile_id;
            face_obj["double_sided"] = face.double_sided;

            // Save vertices
            QJsonArray vertices_array;
            for (int i = 0; i < 4; ++i) {
                QJsonArray vertex;
                vertex.append(face.vertices[i].x);
                vertex.append(face.vertices[i].y);
                vertex.append(face.vertices[i].z);
                vertices_array.append(vertex);
            }
            face_obj["vertices"] = vertices_array;

            // Save UVs
            QJsonArray uvs_array;
            for (int i = 0; i < 4; ++i) {
                QJsonArray uv;
                uv.append(face.uvs[i].x);
                uv.append(face.uvs[i].y);
                uvs_array.append(uv);
            }
            face_obj["uvs"] = uvs_array;

            // Save normal
            QJsonArray normal;
            normal.append(face.normal.x);
            normal.append(face.normal.y);
            normal.append(face.normal.z);
            face_obj["normal"] = normal;

            faces_array.append(face_obj);
        }
    }
    project["faces"] = faces_array;

    // Save tilesets
    QJsonArray tilesets_array;
    for (const auto& pair : m_tilesetPaths) {
        QJsonObject tileset_obj;
        tileset_obj["id"] = pair.first;
        tileset_obj["path"] = pair.second;
        tilesets_array.append(tileset_obj);
    }
    project["tilesets"] = tilesets_array;

    // Write to file
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Save Error", "Could not open file for writing.");
        return false;
    }

    QJsonDocument doc(project);
    file.write(doc.toJson());
    file.close();

    m_currentFilePath = filepath;
    setModified(false);
    m_statusLabel->setText("Project saved");

    return true;
}

bool Tilemap25DPainterDialog::loadProject(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Load Error", "Could not open file for reading.");
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::warning(this, "Load Error", "Invalid project file format.");
        return false;
    }

    QJsonObject project = doc.object();

    // Clear current project
    newProject();

    // Load tilesets first
    QJsonArray tilesets_array = project["tilesets"].toArray();
    for (const auto& tileset_value : tilesets_array) {
        QJsonObject tileset_obj = tileset_value.toObject();
        int tileset_id = tileset_obj["id"].toInt();
        QString tileset_path = tileset_obj["path"].toString();

        if (m_canvas) {
            m_canvas->LoadTileset(tileset_id, tileset_path);
        }

        // Add to UI
        QFileInfo fileInfo(tileset_path);
        QListWidgetItem* item = new QListWidgetItem(fileInfo.baseName());
        item->setData(Qt::UserRole, tileset_id);
        m_tilesetList->addItem(item);

        m_tilesetPaths[tileset_id] = tileset_path;
        m_nextTilesetId = std::max(m_nextTilesetId, tileset_id + 1);
    }

    // Load faces
    std::vector<PaintedFace> faces;
    QJsonArray faces_array = project["faces"].toArray();
    for (const auto& face_value : faces_array) {
        QJsonObject face_obj = face_value.toObject();

        PaintedFace face;
        face.tileset_id = face_obj["tileset_id"].toInt();
        face.tile_id = face_obj["tile_id"].toInt();
        face.double_sided = face_obj["double_sided"].toBool();

        // Load vertices
        QJsonArray vertices_array = face_obj["vertices"].toArray();
        for (int i = 0; i < 4 && i < vertices_array.size(); ++i) {
            QJsonArray vertex = vertices_array[i].toArray();
            if (vertex.size() >= 3) {
                face.vertices[i] = glm::vec3(
                    vertex[0].toDouble(),
                    vertex[1].toDouble(),
                    vertex[2].toDouble()
                );
            }
        }

        // Load UVs
        QJsonArray uvs_array = face_obj["uvs"].toArray();
        for (int i = 0; i < 4 && i < uvs_array.size(); ++i) {
            QJsonArray uv = uvs_array[i].toArray();
            if (uv.size() >= 2) {
                face.uvs[i] = glm::vec2(
                    uv[0].toDouble(),
                    uv[1].toDouble()
                );
            }
        }

        // Load normal
        QJsonArray normal = face_obj["normal"].toArray();
        if (normal.size() >= 3) {
            face.normal = glm::vec3(
                normal[0].toDouble(),
                normal[1].toDouble(),
                normal[2].toDouble()
            );
        }

        faces.push_back(face);
    }

    if (m_canvas) {
        m_canvas->SetFaces(faces);
    }

    m_currentFilePath = filepath;
    setModified(false);
    updateWindowTitle();
    updateSelectionInfo();
    m_statusLabel->setText("Project loaded");

    return true;
}
