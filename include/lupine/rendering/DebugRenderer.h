#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace Lupine {

class Camera;
class Camera2D;
class Camera3D;
class Scene;

enum class CollisionShapeType;

/**
 * @brief Debug rendering system for visualizing cameras and other debug shapes
 * 
 * DebugRenderer provides functionality to draw debug visualizations like
 * camera bounds, wireframes, and other development aids.
 */
class DebugRenderer {
public:
    /**
     * @brief Debug draw flags
     */
    enum class DebugFlags {
        None = 0,
        Camera2DBounds = 1 << 0,
        Camera3DFrustum = 1 << 1,
        ScreenBounds = 1 << 2,
        GameBounds = 1 << 3,
        Lights = 1 << 4,
        CollisionShapes2D = 1 << 5,
        CollisionShapes3D = 1 << 6,
        All = Camera2DBounds | Camera3DFrustum | ScreenBounds | GameBounds | Lights | CollisionShapes2D | CollisionShapes3D
    };

    /**
     * @brief Initialize debug renderer
     */
    static void Initialize();
    
    /**
     * @brief Shutdown debug renderer
     */
    static void Shutdown();
    
    /**
     * @brief Check if debug renderer is initialized
     * @return True if initialized
     */
    static bool IsInitialized() { return s_initialized; }
    
    /**
     * @brief Set debug flags
     * @param flags Debug flags to enable
     */
    static void SetDebugFlags(DebugFlags flags) { s_debug_flags = flags; }
    
    /**
     * @brief Get current debug flags
     * @return Current debug flags
     */
    static DebugFlags GetDebugFlags() { return s_debug_flags; }
    
    /**
     * @brief Enable specific debug flag
     * @param flag Flag to enable
     */
    static void EnableDebugFlag(DebugFlags flag);
    
    /**
     * @brief Disable specific debug flag
     * @param flag Flag to disable
     */
    static void DisableDebugFlag(DebugFlags flag);
    
    /**
     * @brief Check if debug flag is enabled
     * @param flag Flag to check
     * @return True if enabled
     */
    static bool IsDebugFlagEnabled(DebugFlags flag);
    
    /**
     * @brief Render debug visualizations for a scene
     * @param scene Scene to render debug info for
     * @param view_camera Camera used for viewing (editor camera)
     */
    static void RenderDebugInfo(Scene* scene, Camera* view_camera);
    
    /**
     * @brief Render Camera2D bounds
     * @param camera2d Camera2D component to visualize
     * @param view_camera Camera used for viewing
     */
    static void RenderCamera2DBounds(Camera2D* camera2d, Camera* view_camera);
    
    /**
     * @brief Render Camera3D frustum
     * @param camera3d Camera3D component to visualize
     * @param view_camera Camera used for viewing
     */
    static void RenderCamera3DFrustum(Camera3D* camera3d, Camera* view_camera);
    
    /**
     * @brief Render screen space bounds
     * @param view_camera Camera used for viewing
     */
    static void RenderScreenBounds(Camera* view_camera);

    /**
     * @brief Render game bounds (standard 2D world bounds without camera transform)
     * @param view_camera Camera used for viewing
     */
    static void RenderGameBounds(Camera* view_camera);

    /**
     * @brief Draw a wireframe rectangle
     * @param min Bottom-left corner
     * @param max Top-right corner
     * @param color Line color
     * @param view_projection View-projection matrix
     */
    static void DrawWireframeRect(const glm::vec2& min, const glm::vec2& max, const glm::vec3& color, const glm::mat4& view_projection);

    /**
     * @brief Render light debug visualizations
     * @param scene Scene to render light debug info for
     * @param view_camera Camera used for viewing
     */
    static void RenderLightDebugInfo(Scene* scene, Camera* view_camera);

    /**
     * @brief Render 2D collision shapes debug info for a scene
     * @param scene Scene to render collision shapes for
     * @param view_camera Camera used for viewing
     */
    static void RenderCollisionShapes2D(Scene* scene, Camera* view_camera);

    /**
     * @brief Render a 2D collision shape
     * @param shape_type Type of collision shape
     * @param position World position
     * @param size Shape size
     * @param rotation Shape rotation
     * @param color Debug color
     * @param view_projection View-projection matrix
     */
    static void RenderCollisionShape2D(CollisionShapeType shape_type, const glm::vec2& position,
                                     const glm::vec2& size, float rotation, const glm::vec3& color,
                                     const glm::mat4& view_projection);

    /**
     * @brief Render 3D collision shapes debug info for a scene
     * @param scene Scene to render collision shapes for
     * @param view_camera Camera used for viewing
     */
    static void RenderCollisionShapes3D(Scene* scene, Camera* view_camera);

    /**
     * @brief Render a 3D collision shape
     * @param shape_type Type of collision shape
     * @param position World position
     * @param size Shape size
     * @param rotation Shape rotation (quaternion)
     * @param color Debug color
     * @param view_projection View-projection matrix
     */
    static void RenderCollisionShape3D(CollisionShapeType shape_type, const glm::vec3& position,
                                     const glm::vec3& size, const glm::quat& rotation, const glm::vec3& color,
                                     const glm::mat4& view_projection);

    /**
     * @brief Draw a wireframe sphere
     * @param center Sphere center
     * @param radius Sphere radius
     * @param color Line color
     * @param view_projection View-projection matrix
     * @param segments Number of segments for sphere approximation
     */
    static void DrawWireframeSphere(const glm::vec3& center, float radius, const glm::vec3& color, const glm::mat4& view_projection, int segments = 16);

    /**
     * @brief Draw a wireframe cone
     * @param apex Cone apex position
     * @param direction Cone direction (normalized)
     * @param range Cone range (height)
     * @param angle Cone angle in radians
     * @param color Line color
     * @param view_projection View-projection matrix
     * @param segments Number of segments for cone approximation
     */
    static void DrawWireframeCone(const glm::vec3& apex, const glm::vec3& direction, float range, float angle, const glm::vec3& color, const glm::mat4& view_projection, int segments = 16);

    /**
     * @brief Draw a directional arrow
     * @param position Arrow position
     * @param direction Arrow direction (normalized)
     * @param length Arrow length
     * @param color Line color
     * @param view_projection View-projection matrix
     */
    static void DrawDirectionalArrow(const glm::vec3& position, const glm::vec3& direction, float length, const glm::vec3& color, const glm::mat4& view_projection);
    
    /**
     * @brief Draw a wireframe box
     * @param min Bottom-left-back corner
     * @param max Top-right-front corner
     * @param color Line color
     * @param view_projection View-projection matrix
     */
    static void DrawWireframeBox(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color, const glm::mat4& view_projection);
    
    /**
     * @brief Draw a line
     * @param start Start position
     * @param end End position
     * @param color Line color
     * @param view_projection View-projection matrix
     */
    static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color, const glm::mat4& view_projection);

private:
    static bool s_initialized;
    static DebugFlags s_debug_flags;
    static unsigned int s_shader_program;
    static unsigned int s_line_VAO;
    static unsigned int s_line_VBO;
    
    /**
     * @brief Create debug shader program
     */
    static void CreateShaderProgram();
    
    /**
     * @brief Create line geometry
     */
    static void CreateLineGeometry();
    
    /**
     * @brief Cleanup OpenGL resources
     */
    static void CleanupResources();
};

// Bitwise operators for DebugFlags
inline DebugRenderer::DebugFlags operator|(DebugRenderer::DebugFlags a, DebugRenderer::DebugFlags b) {
    return static_cast<DebugRenderer::DebugFlags>(static_cast<int>(a) | static_cast<int>(b));
}

inline DebugRenderer::DebugFlags operator&(DebugRenderer::DebugFlags a, DebugRenderer::DebugFlags b) {
    return static_cast<DebugRenderer::DebugFlags>(static_cast<int>(a) & static_cast<int>(b));
}

inline DebugRenderer::DebugFlags operator~(DebugRenderer::DebugFlags a) {
    return static_cast<DebugRenderer::DebugFlags>(~static_cast<int>(a));
}

} // namespace Lupine
