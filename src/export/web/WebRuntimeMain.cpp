#ifdef __EMSCRIPTEN__

#include "lupine/core/Engine.h"
#include "lupine/core/Project.h"
#include "lupine/core/Scene.h"
#include "lupine/core/ComponentRegistration.h"
#include "lupine/assets/AssetBundleReader.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <iostream>
#include <memory>
#include <string>

namespace Lupine {

// Global engine instance for web runtime
static std::unique_ptr<Engine> g_engine;
static std::unique_ptr<Project> g_project;
static std::unique_ptr<AssetBundleReader> g_asset_bundle;
static bool g_initialized = false;
static bool g_running = false;

// Web-specific configuration
struct WebConfig {
    int canvas_width = 1920;
    int canvas_height = 1080;
    std::string project_path = "game.lupine";
    std::string main_scene = "";
    bool debug_mode = false;
};

static WebConfig g_web_config;

// Main loop function called by Emscripten
void MainLoop() {
    if (!g_initialized || !g_running) {
        return;
    }
    
    try {
        // Update engine
        if (g_engine) {
            g_engine->UpdateTiming();
            g_engine->HandleEvents();
            g_engine->Update();
            g_engine->Render();
        }
    } catch (const std::exception& e) {
        std::cerr << "Main loop exception: " << e.what() << std::endl;
        g_running = false;
        emscripten_cancel_main_loop();
    }
}

// Initialize the web runtime
bool InitializeWebRuntime() {
    std::cout << "Initializing Lupine Web Runtime..." << std::endl;
    
    // Initialize component registry
    InitializeComponentRegistry();
    
    // Create engine instance
    g_engine = std::make_unique<Engine>();
    
    // Try to load embedded project
    g_project = std::make_unique<Project>();
    
    // Check if we have an embedded asset bundle
    g_asset_bundle = std::make_unique<AssetBundleReader>();
    if (g_asset_bundle->OpenEmbeddedBundle("game.pck")) {
        std::cout << "Found embedded asset bundle" << std::endl;
        
        // Try to load project from bundle
        std::vector<uint8_t> project_data;
        if (g_asset_bundle->LoadAsset("project.lupine", project_data)) {
            if (g_project->LoadFromMemory(project_data)) {
                std::cout << "Loaded project from embedded bundle" << std::endl;
                g_web_config.project_path = "project.lupine";
            }
        }
    }
    
    // Initialize engine with project settings or defaults
    bool engine_initialized = false;
    if (g_project->IsLoaded()) {
        // Get settings from project - use render resolution for canvas size
        g_web_config.canvas_width = g_project->GetSettingValue<int>("display/render_width", 1920);
        g_web_config.canvas_height = g_project->GetSettingValue<int>("display/render_height", 1080);
        g_web_config.main_scene = g_project->GetSettingValue<std::string>("application/main_scene", "");

        engine_initialized = g_engine->InitializeWithProject(g_project.get());
    } else {
        // Use default settings
        engine_initialized = g_engine->Initialize(
            g_web_config.canvas_width,
            g_web_config.canvas_height,
            "Lupine Game"
        );
    }
    
    if (!engine_initialized) {
        std::cerr << "Failed to initialize engine!" << std::endl;
        return false;
    }
    
    // Load main scene if specified
    if (!g_web_config.main_scene.empty()) {
        if (!g_engine->LoadScene(g_web_config.main_scene)) {
            std::cerr << "Failed to load main scene: " << g_web_config.main_scene << std::endl;
            // Continue anyway with empty scene
        }
    } else if (g_project->IsLoaded()) {
        // Load project normally (which loads main scene)
        if (!g_engine->LoadProject(g_web_config.project_path)) {
            std::cerr << "Failed to load project: " << g_web_config.project_path << std::endl;
            // Continue anyway with empty scene
        }
    } else {
        // Create default empty scene
        auto scene = std::make_unique<Scene>("Default Scene");
        scene->CreateRootNode<Node>("Root");
        // TODO: Set the scene in the engine when that API is available
    }
    
    g_initialized = true;
    std::cout << "Web runtime initialized successfully!" << std::endl;
    return true;
}

// Start the main loop
void StartMainLoop() {
    if (!g_initialized) {
        std::cerr << "Runtime not initialized!" << std::endl;
        return;
    }
    
    g_running = true;
    std::cout << "Starting main loop..." << std::endl;
    
    // Set up Emscripten main loop
    // 0 = use browser's requestAnimationFrame for optimal performance
    // 1 = simulate infinite loop
    emscripten_set_main_loop(MainLoop, 0, 1);
}

// Shutdown the web runtime
void ShutdownWebRuntime() {
    std::cout << "Shutting down web runtime..." << std::endl;
    
    g_running = false;
    
    if (g_engine) {
        g_engine->Shutdown();
        g_engine.reset();
    }
    
    g_project.reset();
    g_asset_bundle.reset();
    g_initialized = false;
    
    std::cout << "Web runtime shutdown complete." << std::endl;
}

} // namespace Lupine

// C-style entry points for JavaScript integration
extern "C" {

// Main entry point called from JavaScript
EMSCRIPTEN_KEEPALIVE
int lupine_main() {
    try {
        if (!Lupine::InitializeWebRuntime()) {
            std::cerr << "Failed to initialize web runtime!" << std::endl;
            return 1;
        }
        
        Lupine::StartMainLoop();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in lupine_main: " << e.what() << std::endl;
        return 1;
    }
}

// Shutdown function callable from JavaScript
EMSCRIPTEN_KEEPALIVE
void lupine_shutdown() {
    Lupine::ShutdownWebRuntime();
}

// Configuration functions callable from JavaScript
EMSCRIPTEN_KEEPALIVE
void lupine_set_canvas_size(int width, int height) {
    Lupine::g_web_config.canvas_width = width;
    Lupine::g_web_config.canvas_height = height;
}

EMSCRIPTEN_KEEPALIVE
void lupine_set_debug_mode(int debug) {
    Lupine::g_web_config.debug_mode = (debug != 0);
}

// Status query functions
EMSCRIPTEN_KEEPALIVE
int lupine_is_initialized() {
    return Lupine::g_initialized ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int lupine_is_running() {
    return Lupine::g_running ? 1 : 0;
}

} // extern "C"

// Traditional main function (fallback)
int main(int argc, char* argv[]) {
    // Parse command line arguments if any
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            Lupine::g_web_config.debug_mode = true;
        } else if (arg.find("--canvas-size=") == 0) {
            std::string size_str = arg.substr(14);
            size_t x_pos = size_str.find('x');
            if (x_pos != std::string::npos) {
                try {
                    Lupine::g_web_config.canvas_width = std::stoi(size_str.substr(0, x_pos));
                    Lupine::g_web_config.canvas_height = std::stoi(size_str.substr(x_pos + 1));
                } catch (const std::exception&) {
                    std::cerr << "Invalid canvas size format: " << size_str << std::endl;
                }
            }
        }
    }
    
    return lupine_main();
}

#endif // __EMSCRIPTEN__
