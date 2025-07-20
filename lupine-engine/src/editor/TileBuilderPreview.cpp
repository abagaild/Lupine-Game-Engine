// Include glad before any Qt OpenGL headers to avoid conflicts
#include <glad/glad.h>

#include "lupine/editor/TileBuilderDialog.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QImage>
#include <iostream>

namespace Lupine {

TileBuilderPreview::TileBuilderPreview(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_camera_distance(3.0f)
    , m_camera_rotation_x(20.0f)
    , m_camera_rotation_y(45.0f)
    , m_mouse_pressed(false)
    , m_shader_program(0)
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
    , m_atlas_texture_id(0)
    , m_gl_initialized(false)
    , m_mesh_loaded(false) {

    setFocusPolicy(Qt::StrongFocus);
}

TileBuilderPreview::~TileBuilderPreview() {
    makeCurrent();

    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_ebo) {
        glDeleteBuffers(1, &m_ebo);
    }
    if (m_shader_program) {
        glDeleteProgram(m_shader_program);
    }
    
    // Clean up textures
    for (auto& pair : m_face_textures) {
        if (pair.second) {
            glDeleteTextures(1, &pair.second);
        }
    }

    if (m_atlas_texture_id) {
        glDeleteTextures(1, &m_atlas_texture_id);
    }
    
    doneCurrent();
}

void TileBuilderPreview::SetTileData(const TileBuilderData& tile_data) {
    m_tile_data = tile_data;
    m_mesh_loaded = false;

    if (m_gl_initialized) {
        makeCurrent();
        updateUVCoordinates();
        renderMesh();
        doneCurrent();
    }
}

void TileBuilderPreview::UpdateFaceTexture(MeshFace face, const QString& texture_path) {
    if (!m_gl_initialized) return;
    
    makeCurrent();
    
    // Load texture
    QImage image(texture_path);
    if (!image.isNull()) {
        // Delete old texture if it exists
        if (m_face_textures.find(face) != m_face_textures.end() && m_face_textures[face]) {
            glDeleteTextures(1, &m_face_textures[face]);
        }
        
        // Create new texture
        unsigned int texture_id;
        glGenTextures(1, &texture_id);
        glBindTexture(GL_TEXTURE_2D, texture_id);
        
        QImage gl_image = image.convertToFormat(QImage::Format_RGBA8888);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl_image.width(), gl_image.height(),
                     0, GL_RGBA, GL_UNSIGNED_BYTE, gl_image.bits());
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        m_face_textures[face] = texture_id;
        
        std::cout << "Loaded texture for face " << static_cast<int>(face)
                  << ": " << texture_path.toStdString() << std::endl;
    }

    // Recreate texture atlas with updated textures
    createTextureAtlas();

    doneCurrent();
    update();
}

void TileBuilderPreview::RefreshPreview() {
    update();
}

void TileBuilderPreview::initializeGL() {
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    setupShaders();
    setupCamera();

    m_gl_initialized = true;

    // Load initial mesh
    renderMesh();
}

void TileBuilderPreview::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void TileBuilderPreview::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (!m_mesh_loaded || m_tile_data.mesh_data.vertices.empty()) {
        return;
    }
    
    glUseProgram(m_shader_program);
    
    // Set up matrices
    QMatrix4x4 model, view, projection;
    
    // Model matrix (identity for now)
    model.setToIdentity();
    
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
    
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, model.data());
    glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.data());
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, projection.data());
    
    // Bind texture atlas or default white texture
    glActiveTexture(GL_TEXTURE0);
    if (m_atlas_texture_id != 0) {
        glBindTexture(GL_TEXTURE_2D, m_atlas_texture_id);
    } else if (!m_face_textures.empty()) {
        glBindTexture(GL_TEXTURE_2D, m_face_textures.begin()->second);
    } else {
        // Bind white texture
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    // Draw mesh
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_tile_data.mesh_data.indices.size()), 
                   GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    glUseProgram(0);
}

void TileBuilderPreview::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_mouse_pressed = true;
        m_last_mouse_pos = event->pos();
        
        // Check for face picking
        MeshFace clicked_face = pickFace(event->pos());
        if (clicked_face != MeshFace::Front) { // Default value check
            emit FaceClicked(clicked_face);
        }
    }
}

void TileBuilderPreview::mouseMoveEvent(QMouseEvent* event) {
    if (m_mouse_pressed) {
        QPoint delta = event->pos() - m_last_mouse_pos;
        
        m_camera_rotation_y += delta.x() * 0.5f;
        m_camera_rotation_x += delta.y() * 0.5f;
        
        // Clamp rotation
        m_camera_rotation_x = qBound(-90.0f, m_camera_rotation_x, 90.0f);
        
        m_last_mouse_pos = event->pos();
        update();
    }
}

void TileBuilderPreview::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_mouse_pressed = false;
    }
}

void TileBuilderPreview::wheelEvent(QWheelEvent* event) {
    float delta = event->angleDelta().y() / 120.0f;
    m_camera_distance -= delta * 0.2f;
    m_camera_distance = qBound(0.5f, m_camera_distance, 10.0f);
    update();
}

void TileBuilderPreview::setupCamera() {
    // Camera setup is handled in paintGL
}

void TileBuilderPreview::renderMesh() {
    if (!m_gl_initialized || m_tile_data.mesh_data.vertices.empty()) {
        return;
    }
    
    // Clean up existing buffers
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
    }
    
    // Create VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    
    // Create VBO
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, 
                 m_tile_data.mesh_data.vertices.size() * sizeof(float),
                 m_tile_data.mesh_data.vertices.data(), GL_STATIC_DRAW);
    
    // Create EBO
    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 m_tile_data.mesh_data.indices.size() * sizeof(unsigned int),
                 m_tile_data.mesh_data.indices.data(), GL_STATIC_DRAW);
    
    // Set vertex attributes
    // Position (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // UV (2 floats)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    m_mesh_loaded = true;

    // Create texture atlas if we have face textures
    createTextureAtlas();

    std::cout << "Loaded mesh with " << m_tile_data.mesh_data.vertices.size() / 8
              << " vertices and " << m_tile_data.mesh_data.indices.size() / 3 << " triangles" << std::endl;
}

void TileBuilderPreview::loadTextures() {
    // Load textures for faces
    // This is a placeholder implementation
}

void TileBuilderPreview::updateUVCoordinates() {
    if (m_tile_data.mesh_data.vertices.empty()) return;

    // Create a copy of the original mesh data to modify
    auto& vertices = m_tile_data.mesh_data.vertices;
    const auto& face_vertex_indices = m_tile_data.mesh_data.face_vertex_indices;
    const auto& face_uv_bounds = m_tile_data.mesh_data.face_uv_bounds;

    // Apply texture transforms to UV coordinates for each face
    for (const auto& face_pair : face_vertex_indices) {
        MeshFace face = face_pair.first;
        const auto& vertex_indices = face_pair.second;

        // Get texture transform for this face
        auto transform_it = m_tile_data.face_textures.find(face);
        if (transform_it == m_tile_data.face_textures.end()) {
            continue; // No transform for this face
        }

        const FaceTextureTransform& transform = transform_it->second;

        // Get UV bounds for this face
        auto bounds_it = face_uv_bounds.find(face);
        if (bounds_it == face_uv_bounds.end()) {
            continue; // No UV bounds for this face
        }

        const glm::vec4& uv_bounds = bounds_it->second;

        // Apply transforms to each vertex of this face
        for (int vertex_index : vertex_indices) {
            // vertex_index is the vertex number (0, 1, 2, 3, etc.)
            // Each vertex has 8 components: [x, y, z, nx, ny, nz, u, v]
            int uv_offset = vertex_index * 8 + 6; // UV starts at offset 6

            if (uv_offset + 1 >= vertices.size()) continue;

            // Get current UV coordinates (stored at offset 6 and 7 in vertex data)
            float u = vertices[uv_offset];
            float v = vertices[uv_offset + 1];

            // Convert to face-local UV coordinates (0-1 range within face bounds)
            float local_u = (u - uv_bounds.x) / (uv_bounds.z - uv_bounds.x);
            float local_v = (v - uv_bounds.y) / (uv_bounds.w - uv_bounds.y);

            // Apply texture transforms
            glm::vec2 uv_coord(local_u, local_v);

            // Apply offset
            uv_coord += transform.offset;

            // Apply scale
            uv_coord *= transform.scale;

            // Apply rotation
            if (transform.rotation != 0.0f) {
                float angle = glm::radians(transform.rotation);
                float cos_a = cos(angle);
                float sin_a = sin(angle);

                // Rotate around center (0.5, 0.5)
                uv_coord -= glm::vec2(0.5f);
                glm::vec2 rotated;
                rotated.x = uv_coord.x * cos_a - uv_coord.y * sin_a;
                rotated.y = uv_coord.x * sin_a + uv_coord.y * cos_a;
                uv_coord = rotated + glm::vec2(0.5f);
            }

            // Clamp to 0-1 range if not using full texture
            if (!transform.use_full_texture) {
                uv_coord.x = glm::clamp(uv_coord.x, 0.0f, 1.0f);
                uv_coord.y = glm::clamp(uv_coord.y, 0.0f, 1.0f);
            }

            // Convert back to global UV coordinates
            float final_u = uv_bounds.x + uv_coord.x * (uv_bounds.z - uv_bounds.x);
            float final_v = uv_bounds.y + uv_coord.y * (uv_bounds.w - uv_bounds.y);

            // Update vertex data
            vertices[uv_offset] = final_u;
            vertices[uv_offset + 1] = final_v;
        }
    }
}

void TileBuilderPreview::createTextureAtlas() {
    // Clean up existing atlas
    if (m_atlas_texture_id) {
        glDeleteTextures(1, &m_atlas_texture_id);
        m_atlas_texture_id = 0;
    }

    // If no face textures, don't create atlas
    if (m_face_textures.empty()) {
        return;
    }

    // For simplicity, create a 2x3 atlas layout for cube faces
    // In a full implementation, you'd use a more sophisticated packing algorithm
    const int atlas_width = 512;
    const int atlas_height = 512;
    const int face_width = atlas_width / 3;
    const int face_height = atlas_height / 2;

    // Create atlas image
    QImage atlas_image(atlas_width, atlas_height, QImage::Format_RGBA8888);
    atlas_image.fill(QColor(255, 255, 255, 255)); // White background

    QPainter painter(&atlas_image);

    // Map faces to atlas positions
    std::map<MeshFace, QRect> face_positions = {
        {MeshFace::Front,  QRect(face_width, 0, face_width, face_height)},
        {MeshFace::Back,   QRect(0, face_height, face_width, face_height)},
        {MeshFace::Left,   QRect(0, 0, face_width, face_height)},
        {MeshFace::Right,  QRect(face_width * 2, 0, face_width, face_height)},
        {MeshFace::Top,    QRect(face_width * 2, face_height, face_width, face_height)},
        {MeshFace::Bottom, QRect(face_width, face_height, face_width, face_height)}
    };

    // Copy face textures to atlas
    for (const auto& face_pair : m_tile_data.face_textures) {
        MeshFace face = face_pair.first;
        const FaceTextureTransform& transform = face_pair.second;

        if (transform.texture_path.empty()) continue;

        // Load face texture
        QImage face_image(QString::fromStdString(transform.texture_path));
        if (face_image.isNull()) continue;

        // Get atlas position for this face
        auto pos_it = face_positions.find(face);
        if (pos_it == face_positions.end()) continue;

        const QRect& face_rect = pos_it->second;

        // Scale and draw face texture to atlas
        QImage scaled_face = face_image.scaled(face_rect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        painter.drawImage(face_rect, scaled_face);
    }

    painter.end();

    // Create OpenGL texture from atlas
    glGenTextures(1, &m_atlas_texture_id);
    glBindTexture(GL_TEXTURE_2D, m_atlas_texture_id);

    QImage gl_image = atlas_image.convertToFormat(QImage::Format_RGBA8888);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl_image.width(), gl_image.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, gl_image.bits());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Created texture atlas: " << atlas_width << "x" << atlas_height << std::endl;
}

void TileBuilderPreview::setupShaders() {
    // Simple vertex shader
    const char* vertex_shader_source = R"(
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
    
    // Simple fragment shader
    const char* fragment_shader_source = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;
        
        uniform sampler2D texture1;
        
        void main() {
            vec3 lightColor = vec3(1.0, 1.0, 1.0);
            vec3 lightPos = vec3(2.0, 2.0, 2.0);
            
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * lightColor;
            
            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            
            vec3 result = (ambient + diffuse) * texture(texture1, TexCoord).rgb;
            FragColor = vec4(result, 1.0);
        }
    )";
    
    // Compile shaders
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    
    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    
    // Create program
    m_shader_program = glCreateProgram();
    glAttachShader(m_shader_program, vertex_shader);
    glAttachShader(m_shader_program, fragment_shader);
    glLinkProgram(m_shader_program);
    
    // Clean up
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

MeshFace TileBuilderPreview::pickFace(const QPoint& mouse_pos) {
    // Simple face picking - for now just return Front face
    // This would need proper ray-triangle intersection in a full implementation
    return MeshFace::Front;
}

} // namespace Lupine
