#include "lupine/core/Engine.h"
#include "lupine/rendering/Renderer.h"
#include "lupine/rendering/Camera.h"
#include "lupine/rendering/ViewportManager.h"
#include "lupine/input/InputManager.h"
#include "lupine/input/ActionMap.h"
#include "lupine/audio/AudioManager.h"
#include "lupine/physics/PhysicsManager.h"
#include "lupine/resources/ResourceManager.h"
#include "lupine/core/GlobalsManager.h"
// #include "lupine/scriptableobjects/ScriptableObjectManager.h"  // Temporarily disabled
#include "lupine/components/Camera3D.h"
#include "lupine/components/Camera2D.h"
#include "lupine/nodes/Node2D.h"
#include "lupine/export/AssetBundler.h"
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <filesystem>

namespace Lupine {

// Static instance for global access
Engine* Engine::s_instance = nullptr;

Engine::Engine()
    : m_initialized(false)
    , m_running(false)
    , m_window(nullptr)
    , m_gl_context(nullptr)
    , m_delta_time(0.0f)
    , m_fps(0.0f)
    , m_last_frame_time(0) {
    s_instance = this;  // Set global instance
}

Engine::~Engine() {
    if (s_instance == this) {
        s_instance = nullptr;  // Clear global instance
    }
    Shutdown();
}

bool Engine::Initialize(int window_width, int window_height, const std::string& window_title) {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing Lupine Engine..." << std::endl;

    if (!InitializeSDL()) {
        return false;
    }

    // Create window with high DPI support by default
    m_window = SDL_CreateWindow(
        window_title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!m_window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!InitializeOpenGL()) {
        return false;
    }

    // Get actual drawable size (may be different from window size on high DPI displays)
    int drawable_width, drawable_height;
    SDL_GL_GetDrawableSize(m_window, &drawable_width, &drawable_height);

    // Set OpenGL viewport to match drawable size
    glViewport(0, 0, drawable_width, drawable_height);
    std::cout << "Window size: " << window_width << "x" << window_height << std::endl;
    std::cout << "Drawable size: " << drawable_width << "x" << drawable_height << std::endl;

    // Update ViewportManager with window size (for basic initialization, use window size as render resolution)
    ViewportManager::SetCurrentBounds(ViewportManager::GetScreenBounds(static_cast<float>(window_width), static_cast<float>(window_height)));
    std::cout << "ViewportManager bounds set to: " << window_width << "x" << window_height << std::endl;

    // Initialize resource manager
    if (!ResourceManager::Initialize()) {
        std::cerr << "Failed to initialize resource manager!" << std::endl;
        return false;
    }

    // Initialize renderer
    if (!Renderer::Initialize()) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return false;
    }

    // Initialize input manager
    if (!InputManager::Initialize()) {
        std::cerr << "Failed to initialize input manager!" << std::endl;
        return false;
    }

    // Register default input actions
    ActionMap defaultActionMap = ActionMap::CreateDefault();
    defaultActionMap.ApplyToInputManager();
    std::cout << "Default input actions registered." << std::endl;

    // Initialize audio manager
    if (!AudioManager::Initialize()) {
        std::cerr << "Failed to initialize audio manager!" << std::endl;
        return false;
    }

    // Initialize physics manager
    if (!PhysicsManager::Initialize()) {
        std::cerr << "Failed to initialize physics manager!" << std::endl;
        return false;
    }

    // Initialize globals manager
    if (!GlobalsManager::Initialize()) {
        std::cerr << "Failed to initialize globals manager!" << std::endl;
        return false;
    }

    // Initialize scriptable objects manager - Temporarily disabled
    // ScriptableObjectManager::Instance().Initialize();
    // std::cout << "Scriptable objects manager initialized." << std::endl;

    // Create default camera
    m_camera = std::make_unique<Camera>(ProjectionType::Perspective);
    m_camera->SetPerspective(glm::radians(45.0f), (float)window_width / (float)window_height, 0.1f, 100.0f);
    m_camera->SetPosition(glm::vec3(0.0f, 0.0f, 3.0f));

    m_initialized = true;
    std::cout << "Engine initialized successfully!" << std::endl;
    return true;
}

bool Engine::InitializeWithProject(const Project* project) {
    if (m_initialized) {
        return true;
    }

    if (!project) {
        std::cerr << "Cannot initialize engine with null project" << std::endl;
        return false;
    }

    std::cout << "Initializing Lupine Engine with project settings..." << std::endl;

    // Get window settings from project
    int window_width = project->GetSettingValue<int>("display/window_width", 1920);
    int window_height = project->GetSettingValue<int>("display/window_height", 1080);
    bool fullscreen = project->GetSettingValue<bool>("display/fullscreen", false);
    bool enable_high_dpi = project->GetSettingValue<bool>("display/enable_high_dpi", true);
    std::string window_title = project->GetSettingValue<std::string>("display/title", project->GetName());

    // Get render resolution settings (separate from window size)
    int render_width = project->GetSettingValue<int>("display/render_width", 1920);
    int render_height = project->GetSettingValue<int>("display/render_height", 1080);

    // Apply debug window scale if available
    float debug_scale = project->GetSettingValue<float>("display/debug_window_scale", 1.0f);
    if (debug_scale != 1.0f) {
        window_width = static_cast<int>(window_width * debug_scale);
        window_height = static_cast<int>(window_height * debug_scale);

        // Apply debug render resolution if different from default
        int debug_render_width = project->GetSettingValue<int>("display/debug_render_width", render_width);
        int debug_render_height = project->GetSettingValue<int>("display/debug_render_height", render_height);
        if (debug_render_width != render_width || debug_render_height != render_height) {
            render_width = debug_render_width;
            render_height = debug_render_height;
            window_title += " (Debug Scale: " + std::to_string(debug_scale) + "x, Render: " +
                           std::to_string(render_width) + "x" + std::to_string(render_height) + ")";
        } else {
            window_title += " (Debug Scale: " + std::to_string(debug_scale) + "x)";
        }
    }

    if (!InitializeSDL()) {
        return false;
    }

    // Create window with project settings
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    if (fullscreen) {
        window_flags |= SDL_WINDOW_FULLSCREEN;
    }
    if (enable_high_dpi) {
        window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    }

    m_window = SDL_CreateWindow(
        window_title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width,
        window_height,
        window_flags
    );

    if (!m_window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!InitializeOpenGL()) {
        return false;
    }

    // Get actual drawable size (may be different from window size on high DPI displays)
    int drawable_width, drawable_height;
    SDL_GL_GetDrawableSize(m_window, &drawable_width, &drawable_height);

    // Set OpenGL viewport to match drawable size
    glViewport(0, 0, drawable_width, drawable_height);
    std::cout << "Window size: " << window_width << "x" << window_height << std::endl;
    std::cout << "Drawable size: " << drawable_width << "x" << drawable_height << std::endl;
    std::cout << "Render resolution: " << render_width << "x" << render_height << std::endl;

    // Update ViewportManager with render resolution (not window size)
    ViewportManager::SetCurrentBounds(ViewportManager::GetScreenBounds(static_cast<float>(render_width), static_cast<float>(render_height)));
    std::cout << "ViewportManager bounds set to render resolution: " << render_width << "x" << render_height << std::endl;

    // Initialize resource manager
    if (!ResourceManager::Initialize()) {
        std::cerr << "Failed to initialize resource manager!" << std::endl;
        return false;
    }

    // Apply rendering settings from project
    std::string stretchStyleStr = project->GetSettingValue<std::string>("rendering/stretch_style", "bilinear");
    if (stretchStyleStr == "nearest") {
        ResourceManager::SetTextureFilter(TextureFilter::Nearest);
    } else if (stretchStyleStr == "bicubic") {
        ResourceManager::SetTextureFilter(TextureFilter::Bicubic);
    } else {
        ResourceManager::SetTextureFilter(TextureFilter::Bilinear);
    }

    // Initialize renderer
    if (!Renderer::Initialize()) {
        std::cerr << "Failed to initialize renderer!" << std::endl;
        return false;
    }

    // Initialize input manager
    if (!InputManager::Initialize()) {
        std::cerr << "Failed to initialize input manager!" << std::endl;
        return false;
    }

    // Register default input actions
    ActionMap defaultActionMap = ActionMap::CreateDefault();
    defaultActionMap.ApplyToInputManager();
    std::cout << "Default input actions registered." << std::endl;

    // Initialize audio manager
    if (!AudioManager::Initialize()) {
        std::cerr << "Failed to initialize audio manager!" << std::endl;
        return false;
    }

    // Initialize physics manager
    if (!PhysicsManager::Initialize()) {
        std::cerr << "Failed to initialize physics manager!" << std::endl;
        return false;
    }

    // Initialize globals manager
    if (!GlobalsManager::Initialize()) {
        std::cerr << "Failed to initialize globals manager!" << std::endl;
        return false;
    }

    // Initialize scriptable objects manager - Temporarily disabled
    // ScriptableObjectManager::Instance().Initialize();
    // std::cout << "Scriptable objects manager initialized." << std::endl;

    // Load globals from project if available
    if (project) {
        auto globals_setting = project->GetSetting("globals");
        if (globals_setting && std::holds_alternative<std::string>(*globals_setting)) {
            std::string json_str = std::get<std::string>(*globals_setting);
            try {
                auto json = nlohmann::json::parse(json_str);
                GlobalsManager::Instance().DeserializeFromJson(json);
                std::cout << "Loaded globals configuration from project" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Error loading globals from project: " << e.what() << std::endl;
            }
        }

        // Load action map from project if available
        auto action_map_setting = project->GetSetting("input/action_map");
        if (action_map_setting && std::holds_alternative<std::string>(*action_map_setting)) {
            std::string action_map_json = std::get<std::string>(*action_map_setting);
            try {
                auto json = nlohmann::json::parse(action_map_json);
                ActionMap actionMap;
                if (actionMap.LoadFromJson(json)) {
                    actionMap.ApplyToInputManager();
                    std::cout << "Loaded action map configuration from project" << std::endl;
                } else {
                    std::cerr << "Failed to load action map from project JSON" << std::endl;
                    // Fall back to default action map
                    ActionMap fallbackActionMap = ActionMap::CreateDefault();
                    fallbackActionMap.ApplyToInputManager();
                }
            } catch (const std::exception& e) {
                std::cerr << "Error loading action map from project: " << e.what() << std::endl;
                // Fall back to default action map
                ActionMap fallbackActionMap2 = ActionMap::CreateDefault();
                fallbackActionMap2.ApplyToInputManager();
            }
        } else {
            // No action map in project, use default
            ActionMap fallbackActionMap3 = ActionMap::CreateDefault();
            fallbackActionMap3.ApplyToInputManager();
            std::cout << "No action map found in project, using default" << std::endl;
        }
    }

    m_initialized = true;
    std::cout << "Lupine Engine initialized successfully with project settings!" << std::endl;
    return true;
}

void Engine::Shutdown() {
    if (!m_initialized) {
        return;
    }

    std::cout << "Shutting down Lupine Engine..." << std::endl;

    // Shutdown scriptable objects manager - Temporarily disabled
    // ScriptableObjectManager::Instance().Shutdown();

    // Shutdown globals manager
    GlobalsManager::Shutdown();

    // Shutdown physics manager
    PhysicsManager::Shutdown();

    // Shutdown audio manager
    AudioManager::Shutdown();

    // Shutdown input manager
    InputManager::Shutdown();

    // Shutdown resource manager
    ResourceManager::Shutdown();

    m_camera.reset();
    m_current_scene.reset();
    m_current_project.reset();

    Renderer::Shutdown();

    if (m_gl_context) {
        SDL_GL_DeleteContext(m_gl_context);
        m_gl_context = nullptr;
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
    m_initialized = false;

    std::cout << "Engine shutdown complete." << std::endl;
}

void Engine::Run() {
    if (!m_initialized) {
        std::cerr << "Engine not initialized!" << std::endl;
        return;
    }

    std::cout << "Starting main loop..." << std::endl;
    m_running = true;
    m_last_frame_time = SDL_GetTicks();

    while (m_running) {
        try {
            UpdateTiming();
            HandleEvents();
            Update();
            Render();
        } catch (const std::exception& e) {
            std::cerr << "Main loop exception: " << e.what() << std::endl;
            m_running = false;
        } catch (...) {
            std::cerr << "Unknown main loop exception" << std::endl;
            m_running = false;
        }
    }

    std::cout << "Main loop ended." << std::endl;
}

bool Engine::LoadProject(const std::string& project_path) {
    std::cout << "Loading project: " << project_path << std::endl;

    auto project = std::make_unique<Project>();
    if (!project->LoadFromFile(project_path)) {
        std::cerr << "Failed to load project: " << project_path << std::endl;
        return false;
    }

    m_current_project = std::move(project);

    // Load main scene if specified
    if (!m_current_project->GetMainScene().empty()) {
        std::string scene_path = m_current_project->GetMainScene();

        // If the scene path is not absolute and we have a project directory, make it relative to project
        if (!std::filesystem::path(scene_path).is_absolute() &&
            !m_current_project->GetProjectDirectory().empty() &&
            m_current_project->GetProjectDirectory() != ".") {
            scene_path = m_current_project->GetProjectDirectory() + "/" + scene_path;
        }

        return LoadScene(scene_path);
    }

    return true;
}

bool Engine::LoadScene(const std::string& scene_path) {
    std::cout << "Loading scene: " << scene_path << std::endl;

    auto scene = std::make_unique<Scene>();
    if (!scene->LoadFromFile(scene_path)) {
        std::cerr << "Failed to load scene: " << scene_path << std::endl;
        return false;
    }

    // Exit current scene if any
    if (m_current_scene) {
        m_current_scene->OnExit();
        // Cleanup autoloads from previous scene
        GlobalsManager::Instance().CleanupAutoloads();
    }

    m_current_scene = std::move(scene);

    // Initialize autoloads for the new scene
    GlobalsManager::Instance().InitializeAutoloads(m_current_scene.get());

    m_current_scene->OnEnter();

    return true;
}

bool Engine::InitializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }

    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    return true;
}

bool Engine::InitializeOpenGL() {
    m_gl_context = SDL_GL_CreateContext(m_window);
    if (!m_gl_context) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        return false;
    }

    // Initialize GLAD
    if (!gladLoadGL()) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        return false;
    }

    // Enable VSync
    SDL_GL_SetSwapInterval(1);

    std::cout << "OpenGL context created successfully" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    return true;
}

void Engine::HandleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Pass event to input manager first
        InputManager::ProcessEvent(event);

        switch (event.type) {
            case SDL_QUIT:
                m_running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    m_running = false;
                }
                break;
            default:
                break;
        }

        // Pass event to current scene
        if (m_current_scene) {
            m_current_scene->OnInput(&event);
        }
    }

    // Add simple camera movement for debugging
    HandleCameraMovement();
}

void Engine::HandleCameraMovement() {
    if (!m_current_scene) return;

    // Find the first Camera3D component in the scene
    auto all_nodes = m_current_scene->GetAllNodes();
    Camera3D* camera3d = nullptr;

    for (auto* node : all_nodes) {
        if (auto* cam = node->GetComponent<Camera3D>()) {
            camera3d = cam;
            break;
        }
    }

    if (!camera3d) return;

    // Get current keyboard state
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    float speed = 5.0f * m_delta_time; // 5 units per second

    // Get camera's current position and rotation
    glm::vec3 pos = camera3d->GetCamera()->GetPosition();
    glm::vec3 forward = camera3d->GetCamera()->GetForward();
    glm::vec3 right = camera3d->GetCamera()->GetRight();
    glm::vec3 up = glm::vec3(0, 1, 0);

    // WASD movement
    if (keys[SDL_SCANCODE_W]) pos += forward * speed;
    if (keys[SDL_SCANCODE_S]) pos -= forward * speed;
    if (keys[SDL_SCANCODE_A]) pos -= right * speed;
    if (keys[SDL_SCANCODE_D]) pos += right * speed;
    if (keys[SDL_SCANCODE_Q]) pos -= up * speed;
    if (keys[SDL_SCANCODE_E]) pos += up * speed;

    // Update camera position
    camera3d->GetCamera()->SetPosition(pos);
}

void Engine::UpdateAudioListener() {
    if (!m_current_scene || !AudioManager::IsInitialized()) {
        return;
    }

    // Find active cameras in the scene
    auto all_nodes = m_current_scene->GetAllNodes();
    Camera3D* active_camera3d = nullptr;
    Camera2D* active_camera2d = nullptr;

    // Look for active cameras
    for (auto* node : all_nodes) {
        if (!node || !node->IsActive()) continue;

        // Check for Camera3D (prioritize 3D for spatial audio)
        if (auto* cam3d = node->GetComponent<Camera3D>()) {
            if (cam3d->IsEnabled() && (cam3d->IsCurrent() || !active_camera3d)) {
                active_camera3d = cam3d;
                if (cam3d->IsCurrent()) break; // Use explicitly current camera
            }
        }

        // Check for Camera2D as fallback
        if (!active_camera3d) {
            if (auto* cam2d = node->GetComponent<Camera2D>()) {
                if (cam2d->IsEnabled() && (cam2d->IsCurrent() || !active_camera2d)) {
                    active_camera2d = cam2d;
                }
            }
        }
    }

    // Update audio listener position and orientation
    if (active_camera3d) {
        // Use 3D camera for full spatial audio
        Camera* camera = active_camera3d->GetCamera();
        if (camera) {
            AudioManager::SetListenerPosition(camera->GetPosition());
            AudioManager::SetListenerOrientation(camera->GetForward(), camera->GetUp());
        }
    } else if (active_camera2d) {
        // Use 2D camera position (z=0) for 2D spatial audio
        if (auto* node2d = dynamic_cast<Node2D*>(active_camera2d->GetOwner())) {
            glm::vec2 pos2d = node2d->GetGlobalPosition();
            glm::vec3 pos3d(pos2d.x, pos2d.y, 0.0f);
            AudioManager::SetListenerPosition(pos3d);
            // Keep default forward/up for 2D
            AudioManager::SetListenerOrientation(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
}

bool Engine::LoadSceneFromBundle(const std::string& scene_bundle_path) {
    std::cout << "Loading scene from bundle: " << scene_bundle_path << std::endl;

    // This method is primarily for runtime use with embedded bundles
    // For now, try to load from file system as fallback
    return LoadScene(scene_bundle_path);
}

bool Engine::SwitchScene(const std::string& scene_path) {
    std::cout << "Switching to scene: " << scene_path << std::endl;

    // Try file system paths
    std::vector<std::string> file_paths = {
        scene_path,                                    // Direct path
        m_current_project ? m_current_project->GetProjectDirectory() + "/" + scene_path : scene_path,
        scene_path + ".scene"                          // Add .scene extension if missing
    };

    for (const auto& file_path : file_paths) {
        if (std::filesystem::exists(file_path)) {
            std::cout << "Loading scene from file: " << file_path << std::endl;
            return LoadScene(file_path);
        }
    }

    std::cerr << "Scene not found: " << scene_path << std::endl;
    return false;
}

void Engine::Update() {
    // Update input manager
    InputManager::Update(m_delta_time);

    // Update audio manager
    AudioManager::Update(m_delta_time);

    // Update audio listener position based on active camera
    UpdateAudioListener();

    // Update physics manager
    PhysicsManager::Update(m_delta_time);

    if (m_current_scene) {
        m_current_scene->OnUpdate(m_delta_time);
        m_current_scene->OnPhysicsProcess(m_delta_time);
    }
}

void Engine::Render() {
    // Set runtime rendering context (all objects should render)
    Renderer::SetRenderingContext(RenderingContext::Runtime);

    // Render current scene using camera components
    if (m_current_scene) {
        Renderer::RenderSceneWithCameras(m_current_scene.get(), m_current_project.get());
    }

    // Swap buffers
    SDL_GL_SwapWindow(m_window);
}

void Engine::UpdateTiming() {
    unsigned int current_time = SDL_GetTicks();
    m_delta_time = (current_time - m_last_frame_time) / 1000.0f;
    m_last_frame_time = current_time;

    // Calculate FPS
    if (m_delta_time > 0.0f) {
        m_fps = 1.0f / m_delta_time;
    }
}

} // namespace Lupine
