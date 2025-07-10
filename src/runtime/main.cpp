#define SDL_MAIN_HANDLED
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include "lupine/Lupine.h"
#include "lupine/core/Engine.h"
#include "lupine/core/ComponentRegistration.h"
#include "lupine/export/AssetBundler.h"
#include "lupine/core/CrashHandler.h"

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

// Global bundle reader for embedded assets
std::unique_ptr<Lupine::AssetBundleReader> g_embedded_bundle;
std::string g_temp_runtime_dir;

#ifdef _WIN32
bool g_attachedToParentConsole = false;

bool attachToParentConsole() {
    // Try to attach to parent console (if launched from command line)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        // Successfully attached to parent console
        // Redirect stdout, stdin, stderr to console
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
        freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

        // Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
        // point to console as well
        std::ios::sync_with_stdio(true);

        return true;
    }
    return false;
}

void setupConsole() {
    // First try to attach to parent console (if launched from command line)
    if (attachToParentConsole()) {
        g_attachedToParentConsole = true;
        std::cout << std::endl; // Add newline after command prompt
        std::cout << "=== Lupine Runtime (Command Line Mode) ===" << std::endl;
        std::cout << "Using parent console for output..." << std::endl;
        std::cout << "=========================================" << std::endl;
    } else {
        // No parent console, allocate a new console for this application
        g_attachedToParentConsole = false;
        if (AllocConsole()) {
            // Redirect stdout, stdin, stderr to console
            freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
            freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
            freopen_s((FILE**)stdin, "CONIN$", "r", stdin);

            // Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
            // point to console as well
            std::ios::sync_with_stdio(true);

            // Set console title
            SetConsoleTitleW(L"Lupine Runtime Debug Console");

            std::cout << "=== Lupine Runtime Debug Console ===" << std::endl;
            std::cout << "Console allocated successfully!" << std::endl;
            std::cout << "Press any key to close console when runtime exits..." << std::endl;
            std::cout << "=====================================" << std::endl;
        }
    }
}
#else
void setupConsole() {
    // On Unix-like systems, console output should work by default
    std::cout << "=== Lupine Runtime ===" << std::endl;
}
#endif

// ExtractRuntimeDLLs function removed - using static linking instead

bool LoadSceneFromEmbeddedBundle(Lupine::Engine& engine, const std::string& scene_path) {
    if (!g_embedded_bundle) {
        return false;
    }

    if (!g_embedded_bundle->HasAsset(scene_path)) {
        std::cerr << "Scene not found in bundle: " << scene_path << std::endl;
        return false;
    }

    // Load scene data from bundle
    std::vector<uint8_t> scene_data;
    if (!g_embedded_bundle->LoadAsset(scene_path, scene_data)) {
        std::cerr << "Failed to load scene data from bundle: " << scene_path << std::endl;
        return false;
    }

    std::cout << "Loaded scene data from bundle: " << scene_data.size() << " bytes" << std::endl;

    // Create temporary scene file
    std::string temp_scene_path = "temp_scene.scene";
    std::ofstream temp_file(temp_scene_path, std::ios::binary);
    if (!temp_file.is_open()) {
        std::cerr << "Failed to create temporary scene file: " << temp_scene_path << std::endl;
        return false;
    }

    temp_file.write(reinterpret_cast<const char*>(scene_data.data()), scene_data.size());
    temp_file.flush();
    temp_file.close();

    // Verify the file was written correctly
    if (!std::filesystem::exists(temp_scene_path)) {
        std::cerr << "Temporary scene file was not created: " << temp_scene_path << std::endl;
        return false;
    }

    size_t file_size = std::filesystem::file_size(temp_scene_path);
    std::cout << "Temporary scene file created: " << temp_scene_path << " (" << file_size << " bytes)" << std::endl;

    // Load scene from temporary file
    bool success = engine.LoadScene(temp_scene_path);

    // Clean up temporary file
    std::filesystem::remove(temp_scene_path);

    return success;
}

bool CheckForEmbeddedBundle() {
    // Get current executable path
    std::string exe_path;

#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    exe_path = buffer;
#else
    exe_path = "/proc/self/exe";
#endif

    // Try to open embedded bundle
    g_embedded_bundle = std::make_unique<Lupine::AssetBundleReader>();
    if (!g_embedded_bundle->OpenEmbeddedBundle(exe_path)) {
        // No embedded bundle found
        g_embedded_bundle.reset();
        return false;
    }

    std::cout << "Found embedded bundle with assets" << std::endl;

    // Skip DLL extraction since we're using static linking
    std::cout << "Using static linking - no DLL extraction needed" << std::endl;

    return true;
}

void PrintUsage() {
    std::cout << "Lupine Runtime v1.0.0" << std::endl;
    std::cout << "Usage: lupine-runtime [options] [file]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --project <file>    Load and run a project (.lupine file)" << std::endl;
    std::cout << "  --scene <file>      Load and run a scene (.scene file)" << std::endl;
    std::cout << "  --width <pixels>    Set window width (default: 1920)" << std::endl;
    std::cout << "  --height <pixels>   Set window height (default: 1080)" << std::endl;
    std::cout << "  --title <string>    Set window title" << std::endl;
    std::cout << "  --help              Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "If no options are specified, the first argument is treated as a project or scene file." << std::endl;
}

int main(int argc, char* argv[]) {
    // Setup console for debugging output
    setupConsole();

    std::cout << "Starting Lupine Runtime..." << std::endl;

    // Initialize crash handler
    try {
        Lupine::CrashHandler::Initialize("logs", [](const std::string& crash_info) {
            std::cerr << "RUNTIME CRASH DETECTED: " << crash_info << std::endl;
            std::cout << "RUNTIME CRASH DETECTED: " << crash_info << std::endl;
        });
        std::cout << "Crash handler initialized successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize crash handler: " << e.what() << std::endl;
    }

    // Initialize component registry
    try {
        Lupine::InitializeComponentRegistry();
        std::cout << "Component registry initialized successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize component registry: " << e.what() << std::endl;
        return 1;
    }

    // Check for embedded bundle
    bool has_embedded_bundle = CheckForEmbeddedBundle();

    std::string file_to_load;
    std::string scene_override; // For when both project and scene are specified
    bool is_project = false;
    int window_width = 1920;
    int window_height = 1080;
    std::string window_title = "Lupine Runtime";

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            PrintUsage();
            return 0;
        }
        else if (arg == "--project") {
            if (i + 1 < argc) {
                file_to_load = argv[++i];
                is_project = true;
            } else {
                std::cerr << "Error: --project requires a file path" << std::endl;
                return 1;
            }
        }
        else if (arg == "--scene") {
            if (i + 1 < argc) {
                std::string scene_path = argv[++i];
                if (!file_to_load.empty() && is_project) {
                    // Project already specified, use scene as override
                    scene_override = scene_path;
                } else {
                    // No project specified, load scene directly
                    file_to_load = scene_path;
                    is_project = false;
                }
            } else {
                std::cerr << "Error: --scene requires a file path" << std::endl;
                return 1;
            }
        }
        else if (arg == "--width") {
            if (i + 1 < argc) {
                window_width = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --width requires a value" << std::endl;
                return 1;
            }
        }
        else if (arg == "--height") {
            if (i + 1 < argc) {
                window_height = std::stoi(argv[++i]);
            } else {
                std::cerr << "Error: --height requires a value" << std::endl;
                return 1;
            }
        }
        else if (arg == "--title") {
            if (i + 1 < argc) {
                window_title = argv[++i];
            } else {
                std::cerr << "Error: --title requires a value" << std::endl;
                return 1;
            }
        }
        else if (arg[0] != '-') {
            // Assume it's a file to load
            if (file_to_load.empty()) {
                file_to_load = arg;
                // Determine if it's a project or scene based on extension
                if (file_to_load.length() >= 7 &&
                    file_to_load.substr(file_to_load.length() - 7) == ".lupine") {
                    is_project = true;
                } else {
                    is_project = false;
                }
            }
        }
        else {
            std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
            PrintUsage();
            return 1;
        }
    }

    // Initialize component registry
    Lupine::InitializeComponentRegistry();

    // Create engine
    std::cout << "Creating engine instance..." << std::endl;
    Lupine::Engine engine;
    std::cout << "Engine instance created successfully." << std::endl;

    // Load file if specified and initialize engine accordingly
    if (!file_to_load.empty()) {
        bool loaded = false;
        if (is_project) {
            // Load project first to get settings
            auto project = std::make_unique<Lupine::Project>();
            if (project->LoadFromFile(file_to_load)) {
                // Initialize engine with project settings
                if (!engine.InitializeWithProject(project.get())) {
                    std::cerr << "Failed to initialize engine with project settings!" << std::endl;
                    return 1;
                }

                if (!scene_override.empty()) {
                    // Load specific scene instead of main scene
                    loaded = engine.LoadScene(scene_override);
                } else {
                    // Load project normally (which loads main scene)
                    loaded = engine.LoadProject(file_to_load);
                }
            } else {
                std::cerr << "Failed to load project file: " << file_to_load << std::endl;
                return 1;
            }
        } else {
            // Initialize engine with default settings for scene loading
            if (!engine.Initialize(window_width, window_height, window_title)) {
                std::cerr << "Failed to initialize engine!" << std::endl;
                return 1;
            }
            loaded = engine.LoadScene(file_to_load);
        }

        if (!loaded) {
            std::cerr << "Failed to load file: " << file_to_load << std::endl;
            return 1;
        }
    } else {
        // No file specified - try to auto-load a project file
        std::string auto_project_file;

        // First check if we have an embedded bundle with a project file
        if (has_embedded_bundle && g_embedded_bundle) {
            if (g_embedded_bundle->HasAsset("project.lupine")) {
                std::cout << "Loading project from embedded bundle..." << std::endl;

                // Load project data directly from memory
                std::vector<uint8_t> project_data;
                if (g_embedded_bundle->LoadAsset("project.lupine", project_data)) {
                    auto project = std::make_unique<Lupine::Project>();
                    if (project->LoadFromMemory(project_data)) {
                        // Initialize engine with project settings
                        if (!engine.InitializeWithProject(project.get())) {
                            std::cerr << "Failed to initialize engine with project settings!" << std::endl;
                            return 1;
                        }

                        // Load main scene from embedded bundle if available
                        std::string main_scene = project->GetMainScene();
                        if (!main_scene.empty() && g_embedded_bundle->HasAsset(main_scene)) {
                            std::cout << "Loading main scene from bundle: " << main_scene << std::endl;
                            if (!LoadSceneFromEmbeddedBundle(engine, main_scene)) {
                                std::cerr << "Failed to load main scene from bundle: " << main_scene << std::endl;
                                return 1;
                            }
                        } else {
                            std::cout << "No main scene found in embedded bundle" << std::endl;
                        }

                        // Run the engine
                        engine.Run();

                        // Cleanup
                        if (g_embedded_bundle) {
                            g_embedded_bundle.reset();
                        }

                        return 0;
                    }
                }
            }
        }

        // If no embedded project, check for autoload.cfg file
        if (auto_project_file.empty() && std::filesystem::exists("autoload.cfg")) {
            std::ifstream config_file("autoload.cfg");
            std::string line;
            while (std::getline(config_file, line)) {
                if (line.find("project=") == 0) {
                    auto_project_file = line.substr(8); // Remove "project="
                    break;
                }
            }
        }

        // If no config file, look for any .lupine file in current directory
        if (auto_project_file.empty()) {
            try {
                for (const auto& entry : std::filesystem::directory_iterator(".")) {
                    if (entry.is_regular_file() && entry.path().extension() == ".lupine") {
                        auto_project_file = entry.path().filename().string();
                        break;
                    }
                }
            } catch (...) {
                // Ignore filesystem errors
            }
        }

        if (!auto_project_file.empty() && std::filesystem::exists(auto_project_file)) {
            // Found a project file - load it
            std::cout << "Auto-loading project: " << auto_project_file << std::endl;
            auto project = std::make_unique<Lupine::Project>();
            if (project->LoadFromFile(auto_project_file)) {
                // Initialize engine with project settings
                if (!engine.InitializeWithProject(project.get())) {
                    std::cerr << "Failed to initialize engine with project settings!" << std::endl;
                    return 1;
                }

                // Load project normally (which loads main scene)
                if (!engine.LoadProject(auto_project_file)) {
                    std::cerr << "Failed to load auto-detected project: " << auto_project_file << std::endl;
                    return 1;
                }

                // Regular file-based project loading (no embedded bundle handling here)
            } else {
                std::cerr << "Failed to load auto-detected project file: " << auto_project_file << std::endl;
                return 1;
            }
        } else {
            // Initialize engine with default settings
            if (!engine.Initialize(window_width, window_height, window_title)) {
                std::cerr << "Failed to initialize engine!" << std::endl;
                return 1;
            }

            std::cout << "No file specified. Running with empty scene." << std::endl;
            // Create a default empty scene
            auto scene = std::make_unique<Lupine::Scene>("Default Scene");
            scene->CreateRootNode<Lupine::Node>("Root");
            // TODO: Set the scene in the engine when that API is available
        }
    }

    // Run the engine
    std::cout << "Starting engine main loop..." << std::endl;
    try {
        engine.Run();
        std::cout << "Engine main loop completed successfully." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Engine main loop exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown engine main loop exception" << std::endl;
        return 1;
    }

    // Cleanup
    std::cout << "Starting cleanup..." << std::endl;

    if (g_embedded_bundle) {
        g_embedded_bundle.reset();
        std::cout << "Embedded bundle cleaned up" << std::endl;
    }

    // Clean up temporary project file if created
    if (std::filesystem::exists("embedded_project.lupine")) {
        std::filesystem::remove("embedded_project.lupine");
        std::cout << "Temporary project file cleaned up" << std::endl;
    }

    // Clean up temporary runtime directory
    if (!g_temp_runtime_dir.empty() && std::filesystem::exists(g_temp_runtime_dir)) {
        try {
            std::filesystem::remove_all(g_temp_runtime_dir);
            std::cout << "Cleaned up temporary runtime directory" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to clean up temporary directory: " << e.what() << std::endl;
        }
    }

    // Shutdown crash handler
    try {
        Lupine::CrashHandler::Shutdown();
        std::cout << "Crash handler shutdown complete" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to shutdown crash handler: " << e.what() << std::endl;
    }

    std::cout << "Runtime shutdown complete." << std::endl;

#ifdef _WIN32
    // Only wait for input if we allocated our own console (not attached to parent)
    if (!g_attachedToParentConsole) {
        std::cout << "Press any key to close console..." << std::endl;
        std::cin.get();
    } else {
        std::cout << "Exiting (attached to parent console)..." << std::endl;
        // Add a newline to separate from next command prompt
        std::cout << std::endl;
    }
#endif

    return 0;
}