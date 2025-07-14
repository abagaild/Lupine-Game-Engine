#pragma once

#include "lupine/core/Scene.h"
#include "lupine/core/Project.h"
#include <memory>
#include <string>

struct SDL_Window;
typedef void* SDL_GLContext;

namespace Lupine {

class Camera;
class InputManager;
class PhysicsManager;

/**
 * @brief Main engine class
 *
 * The Engine class manages the main game loop, window, rendering context,
 * and coordinates all engine systems.
 */
class Engine {
public:
    /**
     * @brief Constructor
     */
    Engine();

    /**
     * @brief Destructor
     */
    ~Engine();

    /**
     * @brief Initialize the engine
     * @param window_width Window width
     * @param window_height Window height
     * @param window_title Window title
     * @return True if initialization succeeded
     */
    bool Initialize(int window_width = 1920, int window_height = 1080, const std::string& window_title = "Lupine Engine");

    /**
     * @brief Initialize the engine with project settings
     * @param project Project to get settings from
     * @return True if initialization succeeded
     */
    bool InitializeWithProject(const Project* project);

    /**
     * @brief Shutdown the engine
     */
    void Shutdown();

    /**
     * @brief Run the main game loop
     */
    void Run();

    /**
     * @brief Run a single frame (for editor integration)
     */
    void RunFrame();

    /**
     * @brief Load and run a project
     * @param project_path Path to .lupine project file
     * @return True if project loaded successfully
     */
    bool LoadProject(const std::string& project_path);

    /**
     * @brief Load and run a scene
     * @param scene_path Path to scene file
     * @return True if scene loaded successfully
     */
    bool LoadScene(const std::string& scene_path);

    /**
     * @brief Load a scene from embedded bundle
     * @param scene_bundle_path Path to the scene within the bundle
     * @return True if scene was loaded successfully
     */
    bool LoadSceneFromBundle(const std::string& scene_bundle_path);

    /**
     * @brief Switch to a different scene (tries embedded bundle first, then file system)
     * @param scene_path Path to the scene file or bundle path
     * @return True if scene was switched successfully
     */
    bool SwitchScene(const std::string& scene_path);

    /**
     * @brief Get current scene
     * @return Pointer to current scene
     */
    Scene* GetCurrentScene() const { return m_current_scene.get(); }

    /**
     * @brief Get current project
     * @return Pointer to current project
     */
    Project* GetCurrentProject() const { return m_current_project.get(); }

    /**
     * @brief Get the global engine instance (for scene switching from scripts)
     * @return Pointer to the global engine instance
     */
    static Engine* GetInstance() { return s_instance; }

    /**
     * @brief Check if engine is running
     * @return True if running
     */
    bool IsRunning() const { return m_running; }

    /**
     * @brief Request engine shutdown
     */
    void RequestShutdown() { m_running = false; }

    /**
     * @brief Get delta time
     * @return Delta time in seconds
     */
    float GetDeltaTime() const { return m_delta_time; }

    /**
     * @brief Get frames per second
     * @return Current FPS
     */
    float GetFPS() const { return m_fps; }

private:
    static Engine* s_instance;  // Global engine instance for script access

    bool m_initialized;
    bool m_running;

    // SDL2 objects
    SDL_Window* m_window;
    SDL_GLContext m_gl_context;

    // Timing
    float m_delta_time;
    float m_fps;
    unsigned int m_last_frame_time;

    // Engine systems
    std::unique_ptr<Project> m_current_project;
    std::unique_ptr<Scene> m_current_scene;
    std::unique_ptr<Camera> m_camera;

    /**
     * @brief Initialize SDL2
     * @return True if successful
     */
    bool InitializeSDL();

    /**
     * @brief Initialize OpenGL
     * @return True if successful
     */
    bool InitializeOpenGL();

    /**
     * @brief Handle SDL events
     */
    void HandleEvents();

    /**
     * @brief Handle camera movement for debugging
     */
    void HandleCameraMovement();

    /**
     * @brief Update audio listener position based on active camera
     */
    void UpdateAudioListener();

    /**
     * @brief Update engine systems
     */
    void Update();

    /**
     * @brief Render the current scene
     */
    void Render();

    /**
     * @brief Update timing information
     */
    void UpdateTiming();
};

} // namespace Lupine
