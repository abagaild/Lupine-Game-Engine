#include "lupine/rendering/DebugRenderer.h"
#include "lupine/rendering/Camera.h"
#include "lupine/rendering/ViewportManager.h"
#include "lupine/components/Camera2D.h"
#include "lupine/components/Camera3D.h"
#include "lupine/components/OmniLight.h"
#include "lupine/components/DirectionalLight.h"
#include "lupine/components/SpotLight.h"
#include "lupine/components/RigidBody2D.h"
#include "lupine/components/KinematicBody2D.h"
#include "lupine/components/Area2D.h"
#include "lupine/components/CollisionShape2D.h"
#include "lupine/components/RigidBody3D.h"
#include "lupine/components/KinematicBody3D.h"
#include "lupine/components/Area3D.h"
#include "lupine/components/CollisionMesh3D.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/nodes/Node3D.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

namespace Lupine {

// Static member definitions
bool DebugRenderer::s_initialized = false;
DebugRenderer::DebugFlags DebugRenderer::s_debug_flags = DebugFlags::None;
unsigned int DebugRenderer::s_shader_program = 0;
unsigned int DebugRenderer::s_line_VAO = 0;
unsigned int DebugRenderer::s_line_VBO = 0;

void DebugRenderer::Initialize() {
    if (s_initialized) return;
    
    CreateShaderProgram();
    CreateLineGeometry();
    
    s_initialized = true;
    std::cout << "DebugRenderer initialized" << std::endl;
}

void DebugRenderer::Shutdown() {
    if (!s_initialized) return;
    
    CleanupResources();
    s_initialized = false;
    std::cout << "DebugRenderer shutdown" << std::endl;
}

void DebugRenderer::EnableDebugFlag(DebugFlags flag) {
    s_debug_flags = s_debug_flags | flag;
}

void DebugRenderer::DisableDebugFlag(DebugFlags flag) {
    s_debug_flags = s_debug_flags & (~flag);
}

bool DebugRenderer::IsDebugFlagEnabled(DebugFlags flag) {
    return (s_debug_flags & flag) != DebugFlags::None;
}

void DebugRenderer::RenderDebugInfo(Scene* scene, Camera* view_camera) {
    if (!s_initialized || !scene || !view_camera) return;

    static int debug_frame_counter = 0;
    debug_frame_counter++;
    if (debug_frame_counter % 60 == 0) { // Print every 60 frames
        std::cout << "DebugRenderer::RenderDebugInfo called (frame " << debug_frame_counter << ")" << std::endl;
    }
    
    // Enable blending for debug lines
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth testing to ensure debug lines are always visible
    glDisable(GL_DEPTH_TEST);

    // Enable line smoothing for better visibility
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    glUseProgram(s_shader_program);
    
    // Render screen bounds if enabled
    if (IsDebugFlagEnabled(DebugFlags::ScreenBounds)) {
        RenderScreenBounds(view_camera);
    }

    // Render game bounds if enabled
    if (IsDebugFlagEnabled(DebugFlags::GameBounds)) {
        RenderGameBounds(view_camera);
    }

    // Find and render camera debug info
    std::vector<Node*> all_nodes = scene->GetAllNodes();
    for (Node* node : all_nodes) {
        if (!node || !node->IsActive()) continue;
        
        // Render Camera2D bounds
        if (IsDebugFlagEnabled(DebugFlags::Camera2DBounds)) {
            if (auto* camera2d = node->GetComponent<Camera2D>()) {
                RenderCamera2DBounds(camera2d, view_camera);
            }
        }
        
        // Render Camera3D frustum
        if (IsDebugFlagEnabled(DebugFlags::Camera3DFrustum)) {
            if (auto* camera3d = node->GetComponent<Camera3D>()) {
                if (debug_frame_counter % 60 == 0) {
                    std::cout << "Rendering Camera3D frustum debug" << std::endl;
                }
                RenderCamera3DFrustum(camera3d, view_camera);
            }
        }
    }

    // Render light debug info
    if (IsDebugFlagEnabled(DebugFlags::Lights)) {
        if (debug_frame_counter % 60 == 0) {
            std::cout << "Rendering light debug info" << std::endl;
        }
        RenderLightDebugInfo(scene, view_camera);
    }

    // Render 2D collision shapes debug info
    if (IsDebugFlagEnabled(DebugFlags::CollisionShapes2D)) {
        RenderCollisionShapes2D(scene, view_camera);
    }

    // Render 3D collision shapes debug info
    if (IsDebugFlagEnabled(DebugFlags::CollisionShapes3D)) {
        if (debug_frame_counter % 60 == 0) {
            std::cout << "Rendering 3D collision shapes debug info" << std::endl;
        }
        RenderCollisionShapes3D(scene, view_camera);
    }

    // Restore state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_BLEND);
    glUseProgram(0);
}

void DebugRenderer::RenderCamera2DBounds(Camera2D* camera2d, Camera* view_camera) {
    if (!camera2d || !camera2d->IsEnabled()) return;

    glm::vec4 bounds = camera2d->GetBounds();
    // bounds format: (left, top, right, bottom)
    glm::vec2 min(bounds.x, bounds.w); // left, bottom
    glm::vec2 max(bounds.z, bounds.y); // right, top

    glm::vec3 color = camera2d->IsCurrent() ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 1.0f);
    glm::mat4 view_projection = view_camera->GetViewProjectionMatrix();

    DrawWireframeRect(min, max, color, view_projection);
}

void DebugRenderer::RenderCamera3DFrustum(Camera3D* camera3d, Camera* view_camera) {
    if (!camera3d || !camera3d->IsEnabled()) return;

    Camera* cam = camera3d->GetCamera();
    if (!cam) return;

    glm::vec3 pos = cam->GetPosition();
    glm::vec3 forward = cam->GetForward();
    glm::vec3 right = cam->GetRight();
    glm::vec3 up = cam->GetUp();
    glm::vec3 color = camera3d->IsCurrent() ? glm::vec3(1.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 1.0f);
    glm::mat4 view_projection = view_camera->GetViewProjectionMatrix();

    // Get camera parameters from the actual camera
    float near_plane = camera3d->GetNearPlane();
    float far_plane = camera3d->GetFarPlane();
    float fov = camera3d->GetFOV();
    float aspect = camera3d->GetAspectRatio();

    // Calculate frustum dimensions at near and far planes
    float near_height = 2.0f * tan(fov * 0.5f) * near_plane;
    float near_width = near_height * aspect;
    float far_height = 2.0f * tan(fov * 0.5f) * far_plane;
    float far_width = far_height * aspect;

    // Calculate frustum corner points
    glm::vec3 near_center = pos + forward * near_plane;
    glm::vec3 far_center = pos + forward * far_plane;

    // Near plane corners
    glm::vec3 near_tl = near_center + up * (near_height * 0.5f) - right * (near_width * 0.5f);
    glm::vec3 near_tr = near_center + up * (near_height * 0.5f) + right * (near_width * 0.5f);
    glm::vec3 near_bl = near_center - up * (near_height * 0.5f) - right * (near_width * 0.5f);
    glm::vec3 near_br = near_center - up * (near_height * 0.5f) + right * (near_width * 0.5f);

    // Far plane corners
    glm::vec3 far_tl = far_center + up * (far_height * 0.5f) - right * (far_width * 0.5f);
    glm::vec3 far_tr = far_center + up * (far_height * 0.5f) + right * (far_width * 0.5f);
    glm::vec3 far_bl = far_center - up * (far_height * 0.5f) - right * (far_width * 0.5f);
    glm::vec3 far_br = far_center - up * (far_height * 0.5f) + right * (far_width * 0.5f);

    // Draw near plane
    DrawLine(near_tl, near_tr, color, view_projection);
    DrawLine(near_tr, near_br, color, view_projection);
    DrawLine(near_br, near_bl, color, view_projection);
    DrawLine(near_bl, near_tl, color, view_projection);

    // Draw far plane
    DrawLine(far_tl, far_tr, color, view_projection);
    DrawLine(far_tr, far_br, color, view_projection);
    DrawLine(far_br, far_bl, color, view_projection);
    DrawLine(far_bl, far_tl, color, view_projection);

    // Draw connecting lines (frustum edges)
    DrawLine(near_tl, far_tl, color, view_projection);
    DrawLine(near_tr, far_tr, color, view_projection);
    DrawLine(near_bl, far_bl, color, view_projection);
    DrawLine(near_br, far_br, color, view_projection);

    // Draw forward direction line from camera position
    DrawLine(pos, pos + forward * 3.0f, color, view_projection);
}

void DebugRenderer::RenderScreenBounds(Camera* view_camera) {
    ViewportManager::ScreenBounds bounds = ViewportManager::GetCurrentBounds();

    // Convert screen bounds to world coordinates for 2D view
    if (view_camera->GetProjectionType() == ProjectionType::Orthographic) {
        float left, right, bottom, top, near_plane, far_plane;
        view_camera->GetOrthographicBounds(left, right, bottom, top, near_plane, far_plane);

        // Calculate screen bounds in world space
        float world_width = bounds.width;
        float world_height = bounds.height;
        glm::vec2 min(-world_width * 0.5f, -world_height * 0.5f);
        glm::vec2 max(world_width * 0.5f, world_height * 0.5f);

        glm::vec3 color(1.0f, 0.5f, 0.0f); // Orange
        glm::mat4 view_projection = view_camera->GetViewProjectionMatrix();

        DrawWireframeRect(min, max, color, view_projection);
    }
}

void DebugRenderer::RenderGameBounds(Camera* view_camera) {
    // Get default game bounds (standard 2D world bounds without camera transform)
    ViewportManager::ScreenBounds bounds = ViewportManager::GetDefaultScreenBounds();

    // Only render in 2D view (orthographic projection)
    if (view_camera->GetProjectionType() == ProjectionType::Orthographic) {
        // Game bounds are the standard world space bounds at zoom level 1.0
        float half_width = bounds.width * 0.5f;
        float half_height = bounds.height * 0.5f;

        glm::vec2 min(-half_width, -half_height);
        glm::vec2 max(half_width, half_height);

        glm::vec3 color(0.0f, 1.0f, 0.5f); // Green-cyan for game bounds
        glm::mat4 view_projection = view_camera->GetViewProjectionMatrix();

        DrawWireframeRect(min, max, color, view_projection);
    }
}

void DebugRenderer::DrawWireframeRect(const glm::vec2& min, const glm::vec2& max, const glm::vec3& color, const glm::mat4& view_projection) {
    // Create rectangle vertices
    std::vector<glm::vec3> vertices = {
        glm::vec3(min.x, min.y, 0.0f), // Bottom-left
        glm::vec3(max.x, min.y, 0.0f), // Bottom-right
        glm::vec3(max.x, max.y, 0.0f), // Top-right
        glm::vec3(min.x, max.y, 0.0f)  // Top-left
    };
    
    // Draw lines
    DrawLine(vertices[0], vertices[1], color, view_projection); // Bottom
    DrawLine(vertices[1], vertices[2], color, view_projection); // Right
    DrawLine(vertices[2], vertices[3], color, view_projection); // Top
    DrawLine(vertices[3], vertices[0], color, view_projection); // Left
}

void DebugRenderer::DrawWireframeBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color, const glm::mat4& view_projection) {
    // TODO: Implement 3D wireframe box drawing
    // For now, just draw a simple line
    DrawLine(min, max, color, view_projection);
}

void DebugRenderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, const glm::mat4& view_projection) {
    if (!s_initialized) return;
    
    // Update line vertices
    float vertices[] = {
        start.x, start.y, start.z,
        end.x, end.y, end.z
    };
    
    glBindVertexArray(s_line_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_line_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(s_shader_program, "uMVP"), 1, GL_FALSE, glm::value_ptr(view_projection));
    glUniform3fv(glGetUniformLocation(s_shader_program, "uColor"), 1, glm::value_ptr(color));
    
    // Draw line with thinner width for cleaner debug visualization
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 2);
    glLineWidth(1.0f);
    
    glBindVertexArray(0);
}

void DebugRenderer::CreateShaderProgram() {
    const char* vertex_shader_source = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        
        uniform mat4 uMVP;
        
        void main() {
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )";
    
    const char* fragment_shader_source = R"(
        #version 330 core
        out vec4 FragColor;
        
        uniform vec3 uColor;
        
        void main() {
            FragColor = vec4(uColor, 1.0);
        }
    )";
    
    // Compile vertex shader
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    
    // Compile fragment shader
    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    
    // Create shader program
    s_shader_program = glCreateProgram();
    glAttachShader(s_shader_program, vertex_shader);
    glAttachShader(s_shader_program, fragment_shader);
    glLinkProgram(s_shader_program);
    
    // Clean up shaders
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

void DebugRenderer::CreateLineGeometry() {
    glGenVertexArrays(1, &s_line_VAO);
    glGenBuffers(1, &s_line_VBO);
    
    glBindVertexArray(s_line_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_line_VBO);
    
    // Allocate buffer for 2 vertices (start and end of line)
    glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
}

void DebugRenderer::CleanupResources() {
    if (s_line_VAO) {
        glDeleteVertexArrays(1, &s_line_VAO);
        glDeleteBuffers(1, &s_line_VBO);
        s_line_VAO = 0;
        s_line_VBO = 0;
    }

    if (s_shader_program) {
        glDeleteProgram(s_shader_program);
        s_shader_program = 0;
    }
}

void DebugRenderer::RenderLightDebugInfo(Scene* scene, Camera* view_camera) {
    if (!scene || !view_camera) return;

    static int light_debug_counter = 0;
    light_debug_counter++;
    if (light_debug_counter % 60 == 0) {
        std::cout << "RenderLightDebugInfo called (frame " << light_debug_counter << ")" << std::endl;
    }

    std::vector<Node*> all_nodes = scene->GetAllNodes();
    glm::mat4 view_projection = view_camera->GetViewProjectionMatrix();

    for (Node* node : all_nodes) {
        if (!node || !node->IsActive()) continue;

        // Render OmniLight debug visualization
        if (auto* omni_light = node->GetComponent<OmniLight>()) {
            if (omni_light->IsEnabled()) { // Show debug for all enabled lights when debug flag is on
                if (light_debug_counter % 60 == 0) {
                    std::cout << "Found enabled OmniLight at position: " << omni_light->GetWorldPosition().x
                              << ", " << omni_light->GetWorldPosition().y << ", " << omni_light->GetWorldPosition().z << std::endl;
                }
                glm::vec3 position = omni_light->GetWorldPosition();
                // Use bright, solid color for maximum visibility
                glm::vec3 color(1.0f, 0.0f, 1.0f); // Bright magenta
                float range = std::max(2.0f, omni_light->GetRange()); // Ensure minimum visible size
                DrawWireframeSphere(position, range, color, view_projection);

                // Draw small sphere at light origin
                DrawWireframeSphere(position, 0.2f, color, view_projection);
            }
        }

        // Render DirectionalLight debug visualization
        if (auto* dir_light = node->GetComponent<DirectionalLight>()) {
            if (dir_light->IsEnabled()) { // Show debug for all enabled lights when debug flag is on
                glm::vec3 position = dir_light->GetWorldPosition();
                glm::vec3 direction = dir_light->GetDirection();
                glm::vec3 color = dir_light->GetColor() * dir_light->GetIntensity();
                // Clamp color to prevent overly bright debug visualization and ensure visibility
                color = glm::clamp(color, glm::vec3(0.3f), glm::vec3(1.0f));
                float length = 3.0f; // Make arrow more visible
                DrawDirectionalArrow(position, direction, length, color, view_projection);

                // Draw small sphere at light origin
                DrawWireframeSphere(position, 0.2f, color, view_projection);
            }
        }

        // Render SpotLight debug visualization
        if (auto* spot_light = node->GetComponent<SpotLight>()) {
            if (spot_light->IsEnabled()) { // Show debug for all enabled lights when debug flag is on
                glm::vec3 position = spot_light->GetWorldPosition();
                glm::vec3 direction = spot_light->GetDirection();
                glm::vec3 color = spot_light->GetColor() * spot_light->GetIntensity();
                // Clamp color to prevent overly bright debug visualization and ensure visibility
                color = glm::clamp(color, glm::vec3(0.3f), glm::vec3(1.0f));
                float angle = glm::radians(spot_light->GetOuterConeAngle() * 0.5f);
                float range = std::max(1.0f, spot_light->GetRange()); // Ensure minimum visible size
                DrawWireframeCone(position, direction, range, angle, color, view_projection);

                // Draw small sphere at light origin
                DrawWireframeSphere(position, 0.2f, color, view_projection);
            }
        }
    }
}

void DebugRenderer::DrawWireframeSphere(const glm::vec3& center, float radius, const glm::vec3& color, const glm::mat4& view_projection, int segments) {
    if (!s_initialized) return;

    const float PI = 3.14159265359f;

    // Draw three orthogonal circles to represent the sphere
    // XY plane circle
    for (int i = 0; i < segments; ++i) {
        float angle1 = 2.0f * PI * i / segments;
        float angle2 = 2.0f * PI * (i + 1) / segments;

        glm::vec3 p1 = center + glm::vec3(radius * cos(angle1), radius * sin(angle1), 0.0f);
        glm::vec3 p2 = center + glm::vec3(radius * cos(angle2), radius * sin(angle2), 0.0f);
        DrawLine(p1, p2, color, view_projection);
    }

    // XZ plane circle
    for (int i = 0; i < segments; ++i) {
        float angle1 = 2.0f * PI * i / segments;
        float angle2 = 2.0f * PI * (i + 1) / segments;

        glm::vec3 p1 = center + glm::vec3(radius * cos(angle1), 0.0f, radius * sin(angle1));
        glm::vec3 p2 = center + glm::vec3(radius * cos(angle2), 0.0f, radius * sin(angle2));
        DrawLine(p1, p2, color, view_projection);
    }

    // YZ plane circle
    for (int i = 0; i < segments; ++i) {
        float angle1 = 2.0f * PI * i / segments;
        float angle2 = 2.0f * PI * (i + 1) / segments;

        glm::vec3 p1 = center + glm::vec3(0.0f, radius * cos(angle1), radius * sin(angle1));
        glm::vec3 p2 = center + glm::vec3(0.0f, radius * cos(angle2), radius * sin(angle2));
        DrawLine(p1, p2, color, view_projection);
    }
}

void DebugRenderer::DrawWireframeCone(const glm::vec3& apex, const glm::vec3& direction, float range, float angle, const glm::vec3& color, const glm::mat4& view_projection, int segments) {
    if (!s_initialized) return;

    const float PI = 3.14159265359f;
    glm::vec3 normalized_dir = glm::normalize(direction);
    float radius = range * tan(angle);

    // Calculate base center
    glm::vec3 base_center = apex + normalized_dir * range;

    // Create orthogonal vectors for the base circle
    glm::vec3 up = (abs(normalized_dir.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(normalized_dir, up));
    up = glm::normalize(glm::cross(right, normalized_dir));

    // Draw base circle
    for (int i = 0; i < segments; ++i) {
        float angle1 = 2.0f * PI * i / segments;
        float angle2 = 2.0f * PI * (i + 1) / segments;

        glm::vec3 p1 = base_center + radius * (cos(angle1) * right + sin(angle1) * up);
        glm::vec3 p2 = base_center + radius * (cos(angle2) * right + sin(angle2) * up);
        DrawLine(p1, p2, color, view_projection);

        // Draw lines from apex to base circle
        if (i % 4 == 0) { // Only draw every 4th line to avoid clutter
            DrawLine(apex, p1, color, view_projection);
        }
    }
}

void DebugRenderer::DrawDirectionalArrow(const glm::vec3& position, const glm::vec3& direction, float length, const glm::vec3& color, const glm::mat4& view_projection) {
    if (!s_initialized) return;

    glm::vec3 normalized_dir = glm::normalize(direction);
    glm::vec3 end = position + normalized_dir * length;

    // Draw main arrow shaft
    DrawLine(position, end, color, view_projection);

    // Draw arrow head
    glm::vec3 up = (abs(normalized_dir.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(normalized_dir, up));
    up = glm::normalize(glm::cross(right, normalized_dir));

    float head_length = length * 0.2f;
    float head_width = length * 0.1f;

    glm::vec3 head_base = end - normalized_dir * head_length;
    glm::vec3 head_p1 = head_base + right * head_width;
    glm::vec3 head_p2 = head_base - right * head_width;
    glm::vec3 head_p3 = head_base + up * head_width;
    glm::vec3 head_p4 = head_base - up * head_width;

    DrawLine(end, head_p1, color, view_projection);
    DrawLine(end, head_p2, color, view_projection);
    DrawLine(end, head_p3, color, view_projection);
    DrawLine(end, head_p4, color, view_projection);
}

void DebugRenderer::RenderCollisionShapes2D(Scene* scene, Camera* view_camera) {
    if (!scene || !view_camera) return;

    std::vector<Node*> all_nodes = scene->GetAllNodes();
    glm::mat4 view_projection = view_camera->GetViewProjectionMatrix();

    for (Node* node : all_nodes) {
        if (!node || !node->IsActive()) continue;

        Node2D* node2d = dynamic_cast<Node2D*>(node);
        if (!node2d) continue;

        glm::vec2 position = node2d->GetPosition();
        float rotation = node2d->GetRotation();

        // Check for RigidBody2D
        if (auto* rigidBody = node->GetComponent<RigidBody2D>()) {
            glm::vec3 color(0.0f, 1.0f, 0.0f); // Green for rigid bodies

            // Get shape info from CollisionShape2D if available, otherwise use RigidBody2D defaults
            CollisionShapeType shape_type = CollisionShapeType::Box;
            glm::vec2 shape_size(1.0f, 1.0f);

            if (auto* collision_shape = node->GetComponent<CollisionShape2D>()) {
                shape_type = collision_shape->GetShapeType();
                shape_size = collision_shape->GetSize();
            }

            RenderCollisionShape2D(shape_type, position, shape_size, rotation, color, view_projection);
        }

        // Check for KinematicBody2D
        if (auto* kinematicBody = node->GetComponent<KinematicBody2D>()) {
            glm::vec3 color(0.0f, 0.0f, 1.0f); // Blue for kinematic bodies

            // Get shape info from CollisionShape2D if available, otherwise use KinematicBody2D defaults
            CollisionShapeType shape_type = kinematicBody->GetShapeType();
            glm::vec2 shape_size = kinematicBody->GetSize();

            if (auto* collision_shape = node->GetComponent<CollisionShape2D>()) {
                shape_type = collision_shape->GetShapeType();
                shape_size = collision_shape->GetSize();
            }

            RenderCollisionShape2D(shape_type, position, shape_size, rotation, color, view_projection);
        }

        // Check for Area2D
        if (auto* area = node->GetComponent<Area2D>()) {
            glm::vec3 color(1.0f, 1.0f, 0.0f); // Yellow for areas

            // Get shape info from CollisionShape2D if available, otherwise use Area2D defaults
            CollisionShapeType shape_type = area->GetShapeType();
            glm::vec2 shape_size = area->GetSize();

            if (auto* collision_shape = node->GetComponent<CollisionShape2D>()) {
                shape_type = collision_shape->GetShapeType();
                shape_size = collision_shape->GetSize();
            }

            RenderCollisionShape2D(shape_type, position, shape_size, rotation, color, view_projection);
        }

        // Check for CollisionShape2D (standalone)
        if (auto* collisionShape = node->GetComponent<CollisionShape2D>()) {
            // Only render if not already rendered by a physics body
            if (!node->GetComponent<RigidBody2D>() && !node->GetComponent<KinematicBody2D>() && !node->GetComponent<Area2D>()) {
                glm::vec3 color(1.0f, 0.0f, 1.0f); // Magenta for standalone collision shapes
                RenderCollisionShape2D(collisionShape->GetShapeType(), position + collisionShape->GetOffset(),
                                     collisionShape->GetSize(), rotation, color, view_projection);
            }
        }
    }
}

void DebugRenderer::RenderCollisionShape2D(CollisionShapeType shape_type, const glm::vec2& position,
                                         const glm::vec2& size, float rotation, const glm::vec3& color,
                                         const glm::mat4& view_projection) {
    if (!s_initialized) return;

    const float PI = 3.14159265359f;

    switch (shape_type) {
        case CollisionShapeType::Box: {
            // Draw wireframe rectangle
            glm::vec2 half_size = size * 0.5f;
            glm::vec2 corners[4] = {
                {-half_size.x, -half_size.y},
                { half_size.x, -half_size.y},
                { half_size.x,  half_size.y},
                {-half_size.x,  half_size.y}
            };

            // Apply rotation and translation
            float cos_r = cos(rotation);
            float sin_r = sin(rotation);

            for (int i = 0; i < 4; ++i) {
                glm::vec2& corner = corners[i];
                float x = corner.x * cos_r - corner.y * sin_r + position.x;
                float y = corner.x * sin_r + corner.y * cos_r + position.y;
                corner = {x, y};
            }

            // Draw the four edges
            for (int i = 0; i < 4; ++i) {
                int next = (i + 1) % 4;
                glm::vec3 start(corners[i].x, corners[i].y, 0.0f);
                glm::vec3 end(corners[next].x, corners[next].y, 0.0f);
                DrawLine(start, end, color, view_projection);
            }
            break;
        }

        case CollisionShapeType::Circle: {
            // Draw wireframe circle
            const int segments = 32;
            float radius = size.x; // Use x component as radius

            for (int i = 0; i < segments; ++i) {
                float angle1 = (float)i / segments * 2.0f * PI;
                float angle2 = (float)(i + 1) / segments * 2.0f * PI;

                glm::vec3 start(position.x + cos(angle1) * radius, position.y + sin(angle1) * radius, 0.0f);
                glm::vec3 end(position.x + cos(angle2) * radius, position.y + sin(angle2) * radius, 0.0f);
                DrawLine(start, end, color, view_projection);
            }
            break;
        }

        case CollisionShapeType::Capsule: {
            // Draw wireframe capsule (rounded rectangle)
            float radius = size.x;
            float height = size.y;
            float half_height = (height - radius) * 0.5f;

            // Draw the two semicircles at top and bottom
            const int segments = 16;

            // Top semicircle
            for (int i = 0; i <= segments; ++i) {
                float angle1 = (float)i / segments * PI;
                float angle2 = (float)(i + 1) / segments * PI;

                glm::vec3 start(position.x + cos(angle1) * radius, position.y + half_height + sin(angle1) * radius, 0.0f);
                glm::vec3 end(position.x + cos(angle2) * radius, position.y + half_height + sin(angle2) * radius, 0.0f);
                DrawLine(start, end, color, view_projection);
            }

            // Bottom semicircle
            for (int i = 0; i <= segments; ++i) {
                float angle1 = PI + (float)i / segments * PI;
                float angle2 = PI + (float)(i + 1) / segments * PI;

                glm::vec3 start(position.x + cos(angle1) * radius, position.y - half_height + sin(angle1) * radius, 0.0f);
                glm::vec3 end(position.x + cos(angle2) * radius, position.y - half_height + sin(angle2) * radius, 0.0f);
                DrawLine(start, end, color, view_projection);
            }

            // Draw the two side lines
            glm::vec3 left_top(position.x - radius, position.y + half_height, 0.0f);
            glm::vec3 left_bottom(position.x - radius, position.y - half_height, 0.0f);
            glm::vec3 right_top(position.x + radius, position.y + half_height, 0.0f);
            glm::vec3 right_bottom(position.x + radius, position.y - half_height, 0.0f);

            DrawLine(left_top, left_bottom, color, view_projection);
            DrawLine(right_top, right_bottom, color, view_projection);
            break;
        }
    }
}

void DebugRenderer::RenderCollisionShapes3D(Scene* scene, Camera* view_camera) {
    if (!scene || !view_camera) return;

    std::vector<Node*> all_nodes = scene->GetAllNodes();
    glm::mat4 view_projection = view_camera->GetViewProjectionMatrix();

    for (Node* node : all_nodes) {
        if (!node || !node->IsActive()) continue;

        // Get node position and rotation for 3D shapes
        glm::vec3 position(0.0f);
        glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);

        if (auto* node3d = dynamic_cast<Node3D*>(node)) {
            position = node3d->GetGlobalPosition();
            rotation = node3d->GetGlobalRotation();
        }

        // Check for RigidBody3D
        if (auto* rigidBody = node->GetComponent<RigidBody3D>()) {
            glm::vec3 color(0.0f, 1.0f, 0.0f); // Bright green for rigid bodies

            static int collision_debug_counter = 0;
            collision_debug_counter++;
            if (collision_debug_counter % 60 == 0) {
                std::cout << "Found RigidBody3D at position: " << position.x << ", " << position.y << ", " << position.z << std::endl;
            }

            // Get shape info from RigidBody3D
            CollisionShapeType shape_type = rigidBody->GetCollisionShapeType();
            glm::vec3 shape_size = rigidBody->GetCollisionShapeSize();
            // Ensure minimum visible size
            shape_size = glm::max(shape_size, glm::vec3(0.5f));

            RenderCollisionShape3D(shape_type, position, shape_size, rotation, color, view_projection);
        }

        // Check for KinematicBody3D
        if (auto* kinematicBody = node->GetComponent<Lupine::KinematicBody3D>()) {
            glm::vec3 color(0.0f, 0.0f, 1.0f); // Blue for kinematic bodies

            // Get shape info from KinematicBody3D
            CollisionShapeType shape_type = kinematicBody->GetShapeType();
            glm::vec3 shape_size = kinematicBody->GetSize();

            RenderCollisionShape3D(shape_type, position, shape_size, rotation, color, view_projection);
        }

        // Check for Area3D (when implemented)
        if (auto* area = node->GetComponent<Lupine::Area3D>()) {
            glm::vec3 color(1.0f, 1.0f, 0.0f); // Yellow for areas
            // TODO: Get shape info from Area3D when implemented
            // For now, render a default box
            glm::vec3 shape_size(1.0f, 1.0f, 1.0f);
            RenderCollisionShape3D(CollisionShapeType::Box, position, shape_size, rotation, color, view_projection);
        }

        // Check for CollisionMesh3D (standalone)
        if (auto* collisionMesh = node->GetComponent<CollisionMesh3D>()) {
            // Only render if not already rendered by a physics body
            if (!node->GetComponent<Lupine::RigidBody3D>() && !node->GetComponent<Lupine::KinematicBody3D>() && !node->GetComponent<Lupine::Area3D>()) {
                glm::vec3 color(1.0f, 0.0f, 1.0f); // Magenta for standalone collision meshes
                // For mesh collision, render a wireframe box representing the bounds
                glm::vec3 shape_size = collisionMesh->GetScale();
                glm::vec3 mesh_position = position + collisionMesh->GetOffset();
                RenderCollisionShape3D(CollisionShapeType::Box, mesh_position, shape_size, rotation, color, view_projection);
            }
        }
    }
}

void DebugRenderer::RenderCollisionShape3D(CollisionShapeType shape_type, const glm::vec3& position,
                                         const glm::vec3& size, const glm::quat& rotation, const glm::vec3& color,
                                         const glm::mat4& view_projection) {
    if (!s_initialized) return;

    const float PI = 3.14159265359f;

    switch (shape_type) {
        case CollisionShapeType::Box: {
            // Draw wireframe box
            glm::vec3 half_size = size * 0.5f;

            // Define box vertices in local space
            glm::vec3 vertices[8] = {
                {-half_size.x, -half_size.y, -half_size.z}, // 0: left-bottom-back
                { half_size.x, -half_size.y, -half_size.z}, // 1: right-bottom-back
                { half_size.x,  half_size.y, -half_size.z}, // 2: right-top-back
                {-half_size.x,  half_size.y, -half_size.z}, // 3: left-top-back
                {-half_size.x, -half_size.y,  half_size.z}, // 4: left-bottom-front
                { half_size.x, -half_size.y,  half_size.z}, // 5: right-bottom-front
                { half_size.x,  half_size.y,  half_size.z}, // 6: right-top-front
                {-half_size.x,  half_size.y,  half_size.z}  // 7: left-top-front
            };

            // Apply rotation and translation to vertices
            for (int i = 0; i < 8; ++i) {
                vertices[i] = position + rotation * vertices[i];
            }

            // Draw the 12 edges of the box
            // Back face
            DrawLine(vertices[0], vertices[1], color, view_projection);
            DrawLine(vertices[1], vertices[2], color, view_projection);
            DrawLine(vertices[2], vertices[3], color, view_projection);
            DrawLine(vertices[3], vertices[0], color, view_projection);

            // Front face
            DrawLine(vertices[4], vertices[5], color, view_projection);
            DrawLine(vertices[5], vertices[6], color, view_projection);
            DrawLine(vertices[6], vertices[7], color, view_projection);
            DrawLine(vertices[7], vertices[4], color, view_projection);

            // Connecting edges
            DrawLine(vertices[0], vertices[4], color, view_projection);
            DrawLine(vertices[1], vertices[5], color, view_projection);
            DrawLine(vertices[2], vertices[6], color, view_projection);
            DrawLine(vertices[3], vertices[7], color, view_projection);
            break;
        }

        case CollisionShapeType::Sphere: {
            // Draw wireframe sphere using three orthogonal circles
            float radius = size.x; // Use x component as radius
            const int segments = 32;

            // Create three circles: XY, XZ, YZ planes
            for (int plane = 0; plane < 3; ++plane) {
                for (int i = 0; i < segments; ++i) {
                    float angle1 = (float)i / segments * 2.0f * PI;
                    float angle2 = (float)(i + 1) / segments * 2.0f * PI;

                    glm::vec3 start, end;

                    if (plane == 0) { // XY plane
                        start = glm::vec3(cos(angle1) * radius, sin(angle1) * radius, 0.0f);
                        end = glm::vec3(cos(angle2) * radius, sin(angle2) * radius, 0.0f);
                    } else if (plane == 1) { // XZ plane
                        start = glm::vec3(cos(angle1) * radius, 0.0f, sin(angle1) * radius);
                        end = glm::vec3(cos(angle2) * radius, 0.0f, sin(angle2) * radius);
                    } else { // YZ plane
                        start = glm::vec3(0.0f, cos(angle1) * radius, sin(angle1) * radius);
                        end = glm::vec3(0.0f, cos(angle2) * radius, sin(angle2) * radius);
                    }

                    // Apply rotation and translation
                    start = position + rotation * start;
                    end = position + rotation * end;

                    DrawLine(start, end, color, view_projection);
                }
            }
            break;
        }

        case CollisionShapeType::Capsule: {
            // Draw wireframe capsule (cylinder with spherical caps)
            float radius = size.x;
            float height = size.y;
            float cylinder_height = height - 2.0f * radius;
            float half_cylinder_height = cylinder_height * 0.5f;

            const int segments = 16;

            // Draw cylinder body (vertical lines and top/bottom circles)
            for (int i = 0; i < segments; ++i) {
                float angle = (float)i / segments * 2.0f * PI;
                float x = cos(angle) * radius;
                float z = sin(angle) * radius;

                // Vertical lines
                glm::vec3 bottom = glm::vec3(x, -half_cylinder_height, z);
                glm::vec3 top = glm::vec3(x, half_cylinder_height, z);

                // Apply rotation and translation
                bottom = position + rotation * bottom;
                top = position + rotation * top;

                DrawLine(bottom, top, color, view_projection);

                // Top and bottom circles
                float next_angle = (float)(i + 1) / segments * 2.0f * PI;
                float next_x = cos(next_angle) * radius;
                float next_z = sin(next_angle) * radius;

                glm::vec3 bottom_next = position + rotation * glm::vec3(next_x, -half_cylinder_height, next_z);
                glm::vec3 top_next = position + rotation * glm::vec3(next_x, half_cylinder_height, next_z);

                DrawLine(bottom, bottom_next, color, view_projection);
                DrawLine(top, top_next, color, view_projection);
            }

            // Draw spherical caps (simplified as semicircles)
            for (int i = 0; i < segments; ++i) {
                float angle1 = (float)i / segments * PI;
                float angle2 = (float)(i + 1) / segments * PI;

                // Top cap
                glm::vec3 top_start = glm::vec3(0.0f, half_cylinder_height + sin(angle1) * radius, cos(angle1) * radius);
                glm::vec3 top_end = glm::vec3(0.0f, half_cylinder_height + sin(angle2) * radius, cos(angle2) * radius);

                // Bottom cap
                glm::vec3 bottom_start = glm::vec3(0.0f, -half_cylinder_height - sin(angle1) * radius, cos(angle1) * radius);
                glm::vec3 bottom_end = glm::vec3(0.0f, -half_cylinder_height - sin(angle2) * radius, cos(angle2) * radius);

                // Apply rotation and translation
                top_start = position + rotation * top_start;
                top_end = position + rotation * top_end;
                bottom_start = position + rotation * bottom_start;
                bottom_end = position + rotation * bottom_end;

                DrawLine(top_start, top_end, color, view_projection);
                DrawLine(bottom_start, bottom_end, color, view_projection);
            }
            break;
        }

        default:
            // For unknown shapes, draw a simple wireframe box
            RenderCollisionShape3D(CollisionShapeType::Box, position, size, rotation, color, view_projection);
            break;
    }
}

} // namespace Lupine
