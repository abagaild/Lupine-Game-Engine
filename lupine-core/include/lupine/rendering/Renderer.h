#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <string>

namespace Lupine {

class Camera;
class Scene;
class Node;
class LightingSystem;
class GraphicsDevice;
class GraphicsShader;
class GraphicsBuffer;
class GraphicsTexture;
class GraphicsVertexArray;
struct Mesh;

/**
 * @brief Rendering context for view mode separation
 */
enum class RenderingContext {
    Runtime,    // All objects render (game runtime)
    Editor2D,   // Only 2D objects render (2D editor view)
    Editor3D    // Only 3D objects render (3D editor view)
};

// Shader functionality is now handled by GraphicsShader through the graphics abstraction

/**
 * @brief Render command for batching
 */
struct RenderCommand {
    glm::mat4 model_matrix;
    std::shared_ptr<GraphicsVertexArray> vertex_array;
    unsigned int vertex_count;
    unsigned int index_count;
    glm::vec4 color;
    std::shared_ptr<GraphicsTexture> texture;
    bool use_indices;

    // Texture region and flipping support
    glm::vec4 texture_region = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); // x, y, width, height in 0-1 range
    bool flip_h = false;
    bool flip_v = false;
    bool use_texture_region = false;

    // Lighting support
    bool use_lighting = true;

    // Text rendering support
    bool use_text_shader = false;
    bool reset_dynamic_quad = false; // Reset dynamic quad to default state after rendering

    // Corner radius support
    float corner_radius = 0.0f;
    glm::vec2 rect_size = glm::vec2(1.0f, 1.0f);
    bool use_corner_radius = false;

    // Skinned mesh support
    bool use_skinned_mesh = false;
    std::vector<glm::mat4> bone_transforms; // Bone transformation matrices (max 100 bones)
    unsigned int bone_count = 0;
};

/**
 * @brief Main renderer class
 */
class Renderer {
public:
    /**
     * @brief Initialize the renderer
     * @return True if successful
     */
    static bool Initialize();

    /**
     * @brief Check if renderer is initialized
     * @return True if initialized
     */
    static bool IsInitialized();

    /**
     * @brief Shutdown the renderer
     */
    static void Shutdown();

    /**
     * @brief Begin frame rendering
     * @param camera Camera to render from
     */
    static void BeginFrame(Camera* camera);

    /**
     * @brief End frame rendering
     */
    static void EndFrame();

    /**
     * @brief Clear the screen
     * @param color Clear color
     */
    static void Clear(const glm::vec4& color = glm::vec4(0.2f, 0.3f, 0.3f, 1.0f));

    /**
     * @brief Render a scene
     * @param scene Scene to render
     * @param camera Camera to render from
     * @param clearScreen Whether to clear the screen before rendering (default: true)
     */
    static void RenderScene(Scene* scene, Camera* camera, bool clearScreen = true);

    /**
     * @brief Render a scene using camera components from the scene
     * @param scene Scene to render
     * @param clearScreen Whether to clear the screen before rendering
     */
    static void RenderSceneWithCameras(Scene* scene, bool clearScreen = true);

    /**
     * @brief Render a scene using camera components with project-based viewport
     * @param scene Scene to render
     * @param project Project for viewport settings (can be nullptr for defaults)
     * @param clearScreen Whether to clear the screen before rendering
     */
    static void RenderSceneWithCameras(Scene* scene, class Project* project, bool clearScreen = true);

    /**
     * @brief Submit a render command
     * @param command Render command
     */
    static void Submit(const RenderCommand& command);

    /**
     * @brief Render a mesh with transform
     * @param mesh Mesh to render
     * @param transform Transform matrix
     * @param color Color tint
     */
    static void RenderMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color = glm::vec4(1.0f));

    /**
     * @brief Render a mesh with transform and texture
     * @param mesh Mesh to render
     * @param transform Transform matrix
     * @param color Color tint
     * @param texture_id Texture ID to bind
     */
    static void RenderMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color, std::shared_ptr<GraphicsTexture> texture);

    /**
     * @brief Render a mesh with transform, texture, and lighting control
     * @param mesh Mesh to render
     * @param transform Transform matrix
     * @param color Color tint
     * @param texture_id Texture ID
     * @param use_lighting Whether to apply lighting calculations
     */
    static void RenderMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color, std::shared_ptr<GraphicsTexture> texture, bool use_lighting);

    /**
     * @brief Render a skinned mesh with bone transformations
     * @param mesh Mesh to render
     * @param transform Transform matrix
     * @param color Color tint
     * @param texture_id Texture ID
     * @param bone_transforms Array of bone transformation matrices
     * @param use_lighting Whether to apply lighting calculations
     */
    static void RenderSkinnedMesh(const Mesh& mesh, const glm::mat4& transform, const glm::vec4& color,
                                 std::shared_ptr<GraphicsTexture> texture, const std::vector<glm::mat4>& bone_transforms,
                                 bool use_lighting = true);

    /**
     * @brief Render a quad (for 2D sprites)
     * @param transform Transform matrix
     * @param color Color
     * @param texture_id Texture ID (0 for no texture)
     */
    static void RenderQuad(const glm::mat4& transform, const glm::vec4& color, std::shared_ptr<GraphicsTexture> texture = nullptr);

    /**
     * @brief Render a quad with texture region and flipping (for 2D sprites)
     * @param transform Transform matrix
     * @param color Color
     * @param texture_id Texture ID (0 for no texture)
     * @param texture_region Texture region (x, y, width, height) in normalized coordinates (0-1)
     * @param flip_h Flip horizontally
     * @param flip_v Flip vertically
     */
    static void RenderQuad(const glm::mat4& transform, const glm::vec4& color, std::shared_ptr<GraphicsTexture> texture,
                          const glm::vec4& texture_region, bool flip_h = false, bool flip_v = false);

    /**
     * @brief Render a rounded rectangle with corner radius
     * @param transform Transform matrix
     * @param color Color
     * @param corner_radius Corner radius in pixels
     * @param rect_size Rectangle size (width, height)
     * @param texture_id Texture ID (0 for no texture)
     */
    static void RenderRoundedQuad(const glm::mat4& transform, const glm::vec4& color,
                                 float corner_radius, const glm::vec2& rect_size, std::shared_ptr<GraphicsTexture> texture = nullptr);

    /**
     * @brief Render a text glyph quad with high-quality text shader
     * @param transform Transform matrix
     * @param color Text color
     * @param texture_id Glyph texture ID
     */
    static void RenderTextGlyph(const glm::mat4& transform, const glm::vec4& color, std::shared_ptr<GraphicsTexture> texture);

    /**
     * @brief Render text using glyph textures
     * @param text Text to render
     * @param position Position to render at
     * @param scale Scale factor
     * @param color Text color
     * @param font_path Path to font file
     * @param font_size Font size
     */
    static void RenderText(const std::string& text, const glm::vec2& position, float scale,
                          const glm::vec4& color, const std::string& font_path, int font_size);

    /**
     * @brief Get default shader
     * @return Default shader instance
     */
    static GraphicsShader* GetDefaultShader() { return s_default_shader.get(); }

    /**
     * @brief Get 2D shader
     * @return 2D shader instance
     */
    static GraphicsShader* Get2DShader() { return s_2d_shader.get(); }

    /**
     * @brief Get text shader
     * @return Text shader instance
     */
    static GraphicsShader* GetTextShader() { return s_text_shader.get(); }

    /**
     * @brief Get skinned mesh shader
     * @return Skinned mesh shader instance
     */
    static GraphicsShader* GetSkinnedMeshShader() { return s_skinned_mesh_shader.get(); }

    /**
     * @brief Set the current rendering context
     * @param context Rendering context
     */
    static void SetRenderingContext(RenderingContext context);

    /**
     * @brief Get the current rendering context
     * @return Current rendering context
     */
    static RenderingContext GetRenderingContext();

    /**
     * @brief Get the graphics device
     * @return Graphics device instance
     */
    static std::shared_ptr<GraphicsDevice> GetGraphicsDevice() { return s_graphics_device; }

    /**
     * @brief Get the white texture for default rendering
     * @return Shared pointer to white texture
     */
    static std::shared_ptr<GraphicsTexture> GetWhiteTexture();

    /**
     * @brief Get the lighting system instance
     * @return LightingSystem pointer
     */
    static LightingSystem* GetLightingSystem();

    /**
     * @brief Get the current view matrix
     * @return Current view matrix
     */
    static const glm::mat4& GetViewMatrix();

    /**
     * @brief Get the current projection matrix
     * @return Current projection matrix
     */
    static const glm::mat4& GetProjectionMatrix();

    /**
     * @brief Render skyboxes from the scene
     * @param scene Scene to search for skyboxes
     */
    static void RenderSkyboxes(Scene* scene);

    /**
     * @brief Update lighting from scene
     * @param scene Scene to collect lights from
     */
    static void UpdateLighting(Scene* scene);



private:
    static bool s_initialized;
    static std::shared_ptr<GraphicsDevice> s_graphics_device;
    static std::unique_ptr<LightingSystem> s_lighting_system;
    static std::vector<RenderCommand> s_render_commands;
    static glm::mat4 s_view_matrix;
    static glm::mat4 s_projection_matrix;
    static RenderingContext s_rendering_context;

    // Graphics abstraction resources
    static std::shared_ptr<GraphicsShader> s_default_shader;
    static std::shared_ptr<GraphicsShader> s_2d_shader;
    static std::shared_ptr<GraphicsShader> s_text_shader;
    static std::shared_ptr<GraphicsShader> s_skinned_mesh_shader;
    static std::shared_ptr<GraphicsVertexArray> s_quad_vao;
    static std::shared_ptr<GraphicsBuffer> s_quad_vbo;
    static std::shared_ptr<GraphicsTexture> s_white_texture;

    // Dynamic quad for texture regions and flipping
    static std::shared_ptr<GraphicsVertexArray> s_dynamic_quad_vao;
    static std::shared_ptr<GraphicsBuffer> s_dynamic_quad_vbo;
    static std::shared_ptr<GraphicsBuffer> s_dynamic_quad_ebo;

    /**
     * @brief Create default shaders
     */
    static void CreateDefaultShaders();

    /**
     * @brief Create quad geometry for 2D rendering
     */
    static void CreateQuadGeometry();

    /**
     * @brief Create dynamic quad geometry for texture regions and flipping
     */
    static void CreateDynamicQuadGeometry();

    /**
     * @brief Update dynamic quad with texture region and flipping
     * @param texture_region Texture region (x, y, width, height) in 0-1 range
     * @param flip_h Flip horizontally
     * @param flip_v Flip vertically
     */
    static void UpdateDynamicQuad(const glm::vec4& texture_region, bool flip_h, bool flip_v);

    /**
     * @brief Reset dynamic quad to default state (full texture, no flipping)
     */
    static void ResetDynamicQuad();

    /**
     * @brief Set up dynamic quad with correct UV coordinates for text rendering
     */
    static void SetupTextQuad();

    /**
     * @brief Create white texture for untextured rendering
     */
    static void CreateWhiteTexture();

    /**
     * @brief Flush all render commands
     */
    static void Flush();

    /**
     * @brief Render a scene node and its children recursively
     * @param node Node to render
     */
    static void RenderNode(Node* node);

    /**
     * @brief Render nodes by type with specific camera
     * @param scene Scene to render
     * @param camera Camera to use
     * @param node_type Node type to render (2D, 3D, or Control)
     */
    static void RenderNodesByType(Scene* scene, Camera* camera, const std::string& node_type);

    /**
     * @brief Recursively render nodes by type (helper function)
     * @param node Node to start from
     * @param node_type Node type to render (2D, 3D, or Control)
     */
    static void RenderNodesByTypeRecursive(Node* node, const std::string& node_type);
    static void RenderSkyboxesRecursive(Node* node);

    /**
     * @brief Find active camera components in scene
     * @param scene Scene to search
     * @return Pair of Camera2D and Camera3D pointers (either can be nullptr)
     */
    static std::pair<class Camera2D*, class Camera3D*> FindActiveCameras(Scene* scene);
};

} // namespace Lupine
