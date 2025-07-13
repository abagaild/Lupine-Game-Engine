#include "lupine/export/WebExporter.h"
#include "lupine/core/Project.h"
#include "lupine/core/Scene.h"
#include "lupine/resources/ResourceManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <regex>
#include <algorithm>
#include <chrono>
#include <chrono>

namespace Lupine {

WebExporter::WebExporter() = default;

bool WebExporter::IsAvailable() const {
    return IsEmscriptenAvailable();
}

std::string WebExporter::GetAvailabilityError() const {
    if (IsAvailable()) {
        return "";
    }

    auto emcc_path = GetEmscriptenCompilerPath();
    if (emcc_path.empty()) {
        std::filesystem::path emsdk_path = std::filesystem::current_path() / "thirdparty" / "emsdk";
        if (std::filesystem::exists(emsdk_path)) {
            return "Emscripten SDK found at thirdparty/emsdk but emcc compiler not accessible. "
                   "Run 'cd thirdparty/emsdk && ./emsdk activate latest' to activate the environment";
        } else {
            return "Emscripten SDK not found. Install to thirdparty/emsdk or set EMSDK environment variable";
        }
    }

    return "Web export not available";
}

ExportResult WebExporter::Export(const Project* project, const ExportConfig& config,
                                ExportProgressCallback progress_callback) {
    ExportResult result;
    
    try {
        std::filesystem::path output_dir(config.output_directory);
        
        if (progress_callback) {
            progress_callback(0.1f, "Compiling to WebAssembly...");
        }
        
        // Compile to WebAssembly
        if (!CompileToWebAssembly(project, config, output_dir)) {
            result.error_message = "Failed to compile to WebAssembly";
            return result;
        }
        
        if (progress_callback) {
            progress_callback(0.4f, "Generating HTML wrapper...");
        }
        
        // Generate HTML wrapper
        if (!GenerateHTMLWrapper(project, output_dir, config)) {
            result.error_message = "Failed to generate HTML wrapper";
            return result;
        }
        
        if (progress_callback) {
            progress_callback(0.6f, "Generating JavaScript loader...");
        }
        
        // Generate JavaScript loader
        if (!GenerateJavaScriptLoader(output_dir, config)) {
            result.error_message = "Failed to generate JavaScript loader";
            return result;
        }
        
        if (progress_callback) {
            progress_callback(0.7f, "Packaging web assets...");
        }
        
        // Package assets
        if (!PackageWebAssets(project, output_dir)) {
            result.error_message = "Failed to package web assets";
            return result;
        }
        
        if (progress_callback) {
            progress_callback(0.8f, "Creating web manifest...");
        }
        
        // Create web manifest
        if (!CreateWebManifest(output_dir, config)) {
            result.error_message = "Failed to create web manifest";
            return result;
        }
        
        if (progress_callback) {
            progress_callback(0.9f, "Generating service worker...");
        }
        
        // Generate service worker
        if (!GenerateServiceWorker(output_dir, config)) {
            result.error_message = "Failed to generate service worker";
            return result;
        }
        
        result.success = true;
        result.output_path = (output_dir / config.executable_name).string();
        
        // Calculate total size
        size_t total_size = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(output_dir)) {
            if (entry.is_regular_file()) {
                total_size += entry.file_size();
                result.generated_files.push_back(entry.path().string());
            }
        }
        result.total_size_bytes = total_size;
        
    } catch (const std::exception& e) {
        result.error_message = "Export failed: " + std::string(e.what());
    }
    
    return result;
}

bool WebExporter::CompileToWebAssembly(const Project* project,
                                      const ExportConfig& config,
                                      const std::filesystem::path& output_dir) {
    try {
        // Set up Emscripten environment first
        if (!SetupEmscriptenEnvironment()) {
            std::cerr << "Failed to set up Emscripten environment!" << std::endl;
            return false;
        }

        auto emcc_path = GetEmscriptenCompilerPath();
        if (emcc_path.empty()) {
            std::cerr << "Emscripten compiler not found!" << std::endl;
            return false;
        }

        std::cout << "Starting WebAssembly compilation..." << std::endl;
        std::cout << "Using Emscripten compiler: " << emcc_path << std::endl;

        // Get all compilation components
        auto source_files = GetSourceFiles(project);
        auto flags = GetEmscriptenFlags(config);
        auto includes = GetIncludeDirectories();
        auto lib_dirs = GetLibraryDirectories();
        auto libs = GetLibraries();

        // Build compile command
        std::ostringstream command;
        command << emcc_path.string();

        // Add include directories
        for (const auto& include : includes) {
            command << " " << include;
        }

        // Add source files
        for (const auto& source : source_files) {
            if (std::filesystem::exists(source)) {
                command << " \"" << source.string() << "\"";
            } else {
                std::cout << "Warning: Source file not found: " << source << std::endl;
            }
        }

        // Add library directories
        for (const auto& lib_dir : lib_dirs) {
            command << " " << lib_dir;
        }

        // Add libraries
        for (const auto& lib : libs) {
            command << " " << lib;
        }

        // Add compilation flags
        for (const auto& flag : flags) {
            command << " " << flag;
        }

        // Set output files
        auto output_js = output_dir / "game.js";
        command << " -o \"" << output_js.string() << "\"";

        std::cout << "Compilation command:" << std::endl;
        std::cout << command.str() << std::endl;

        // Execute compilation
        std::cout << "Executing compilation..." << std::endl;
        int result = std::system(command.str().c_str());

        if (result == 0) {
            std::cout << "WebAssembly compilation successful!" << std::endl;

            // Verify output files were created
            if (std::filesystem::exists(output_js)) {
                std::cout << "Generated: " << output_js << std::endl;
            }

            auto output_wasm = output_dir / "game.wasm";
            if (std::filesystem::exists(output_wasm)) {
                std::cout << "Generated: " << output_wasm << std::endl;
            }

            return true;
        } else {
            std::cerr << "Compilation failed with exit code: " << result << std::endl;
            return false;
        }

    } catch (const std::exception& e) {
        std::cerr << "Failed to compile to WebAssembly: " << e.what() << std::endl;
        return false;
    }
}

bool WebExporter::GenerateHTMLWrapper(const Project* project,
                                     const std::filesystem::path& output_dir,
                                     const ExportConfig& config) {
    try {
        auto html_path = output_dir / config.executable_name;
        std::ofstream html_file(html_path);

        if (!html_file) {
            return false;
        }

        auto canvas_size = ParseCanvasSize(config.web.canvas_size);

        html_file << "<!DOCTYPE html>\n";
        html_file << "<html lang=\"en\">\n";
        html_file << "<head>\n";
        html_file << "    <meta charset=\"utf-8\">\n";
        html_file << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        html_file << "    <meta name=\"theme-color\" content=\"#000000\">\n";
        html_file << "    <title>Lupine Game</title>\n";
        html_file << "    <link rel=\"manifest\" href=\"manifest.json\">\n";
        html_file << "    <link rel=\"icon\" type=\"image/png\" href=\"icon.png\">\n";

        // Add PWA meta tags
        html_file << "    <meta name=\"apple-mobile-web-app-capable\" content=\"yes\">\n";
        html_file << "    <meta name=\"apple-mobile-web-app-status-bar-style\" content=\"black\">\n";
        html_file << "    <meta name=\"apple-mobile-web-app-title\" content=\"Lupine Game\">\n";
        html_file << "    <link rel=\"apple-touch-icon\" href=\"icon.png\">\n";

        // Enhanced CSS styling
        html_file << "    <style>\n";
        html_file << "        * {\n";
        html_file << "            box-sizing: border-box;\n";
        html_file << "        }\n";
        html_file << "        body {\n";
        html_file << "            margin: 0;\n";
        html_file << "            padding: 0;\n";
        html_file << "            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);\n";
        html_file << "            display: flex;\n";
        html_file << "            flex-direction: column;\n";
        html_file << "            justify-content: center;\n";
        html_file << "            align-items: center;\n";
        html_file << "            min-height: 100vh;\n";
        html_file << "            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n";
        html_file << "            overflow: hidden;\n";
        html_file << "        }\n";
        html_file << "        #gameContainer {\n";
        html_file << "            position: relative;\n";
        html_file << "            max-width: 100vw;\n";
        html_file << "            max-height: 100vh;\n";
        html_file << "        }\n";
        html_file << "        #canvas {\n";
        html_file << "            border: none;\n";
        html_file << "            display: block;\n";
        html_file << "            max-width: 100%;\n";
        html_file << "            max-height: 100%;\n";
        html_file << "            box-shadow: 0 4px 20px rgba(0,0,0,0.3);\n";
        html_file << "        }\n";
        html_file << "        #loadingScreen {\n";
        html_file << "            position: absolute;\n";
        html_file << "            top: 0;\n";
        html_file << "            left: 0;\n";
        html_file << "            width: 100%;\n";
        html_file << "            height: 100%;\n";
        html_file << "            background: rgba(0,0,0,0.8);\n";
        html_file << "            display: flex;\n";
        html_file << "            flex-direction: column;\n";
        html_file << "            justify-content: center;\n";
        html_file << "            align-items: center;\n";
        html_file << "            color: white;\n";
        html_file << "            z-index: 1000;\n";
        html_file << "        }\n";
        html_file << "        #loadingText {\n";
        html_file << "            font-size: 24px;\n";
        html_file << "            margin-bottom: 20px;\n";
        html_file << "            text-align: center;\n";
        html_file << "        }\n";
        html_file << "        #progressBar {\n";
        html_file << "            width: 300px;\n";
        html_file << "            height: 6px;\n";
        html_file << "            background: rgba(255,255,255,0.2);\n";
        html_file << "            border-radius: 3px;\n";
        html_file << "            overflow: hidden;\n";
        html_file << "        }\n";
        html_file << "        #progressFill {\n";
        html_file << "            height: 100%;\n";
        html_file << "            background: linear-gradient(90deg, #4CAF50, #45a049);\n";
        html_file << "            width: 0%;\n";
        html_file << "            transition: width 0.3s ease;\n";
        html_file << "        }\n";
        html_file << "        #errorScreen {\n";
        html_file << "            display: none;\n";
        html_file << "            position: absolute;\n";
        html_file << "            top: 0;\n";
        html_file << "            left: 0;\n";
        html_file << "            width: 100%;\n";
        html_file << "            height: 100%;\n";
        html_file << "            background: rgba(139, 0, 0, 0.9);\n";
        html_file << "            color: white;\n";
        html_file << "            padding: 20px;\n";
        html_file << "            text-align: center;\n";
        html_file << "            z-index: 1001;\n";
        html_file << "        }\n";
        html_file << "        .spinner {\n";
        html_file << "            border: 3px solid rgba(255,255,255,0.3);\n";
        html_file << "            border-radius: 50%;\n";
        html_file << "            border-top: 3px solid white;\n";
        html_file << "            width: 40px;\n";
        html_file << "            height: 40px;\n";
        html_file << "            animation: spin 1s linear infinite;\n";
        html_file << "            margin: 20px auto;\n";
        html_file << "        }\n";
        html_file << "        @keyframes spin {\n";
        html_file << "            0% { transform: rotate(0deg); }\n";
        html_file << "            100% { transform: rotate(360deg); }\n";
        html_file << "        }\n";
        html_file << "        @media (max-width: 768px) {\n";
        html_file << "            #loadingText { font-size: 18px; }\n";
        html_file << "            #progressBar { width: 250px; }\n";
        html_file << "        }\n";
        html_file << "    </style>\n";
        html_file << "</head>\n";
        html_file << "<body>\n";
        // Get project title
        std::string game_title = "Lupine Game";
        const Project* project_ptr = project;
        if (project_ptr) {
            game_title = project_ptr->GetName();
        }

        html_file << "    <div id=\"gameContainer\">\n";
        html_file << "        <canvas id=\"canvas\" width=\"" << canvas_size.first << "\" height=\"" << canvas_size.second << "\"></canvas>\n";
        html_file << "        <div id=\"loadingScreen\">\n";
        html_file << "            <div id=\"loadingText\">Loading " << game_title << "...</div>\n";
        html_file << "            <div class=\"spinner\"></div>\n";
        html_file << "            <div id=\"progressContainer\">\n";
        html_file << "                <div id=\"progressBar\">\n";
        html_file << "                    <div id=\"progressFill\"></div>\n";
        html_file << "                </div>\n";
        html_file << "                <div id=\"loadingStatus\">Initializing...</div>\n";
        html_file << "            </div>\n";
        html_file << "        </div>\n";
        html_file << "        <div id=\"errorScreen\">\n";
        html_file << "            <h2>Failed to Load Game</h2>\n";
        html_file << "            <p id=\"errorMessage\">An error occurred while loading the game.</p>\n";
        html_file << "            <button onclick=\"location.reload()\">Retry</button>\n";
        html_file << "        </div>\n";
        html_file << "    </div>\n";
        html_file << "    \n";
        html_file << "    <!-- Game scripts -->\n";
        html_file << "    <script src=\"game.js\"></script>\n";
        html_file << "    <script src=\"loader.js\"></script>\n";
        html_file << "    \n";
        html_file << "    <!-- Service worker registration for PWA -->\n";
        html_file << "    <script>\n";
        html_file << "        if ('serviceWorker' in navigator) {\n";
        html_file << "            window.addEventListener('load', function() {\n";
        html_file << "                navigator.serviceWorker.register('sw.js')\n";
        html_file << "                    .then(function(registration) {\n";
        html_file << "                        console.log('ServiceWorker registration successful');\n";
        html_file << "                    })\n";
        html_file << "                    .catch(function(err) {\n";
        html_file << "                        console.log('ServiceWorker registration failed: ', err);\n";
        html_file << "                    });\n";
        html_file << "            });\n";
        html_file << "        }\n";
        html_file << "    </script>\n";
        html_file << "</body>\n";
        html_file << "</html>\n";

        html_file.close();
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to generate HTML wrapper: " << e.what() << std::endl;
        return false;
    }
}

bool WebExporter::GenerateJavaScriptLoader(const std::filesystem::path& output_dir,
                                          const ExportConfig& config) {
    try {
        auto js_path = output_dir / "loader.js";
        std::ofstream js_file(js_path);

        if (!js_file) {
            return false;
        }

        auto canvas_size = ParseCanvasSize(config.web.canvas_size);

        js_file << "// Lupine Game Loader - Enhanced Web Runtime\n";
        js_file << "(function() {\n";
        js_file << "    'use strict';\n";
        js_file << "    \n";
        js_file << "    // Game state management\n";
        js_file << "    let gameState = {\n";
        js_file << "        loaded: false,\n";
        js_file << "        running: false,\n";
        js_file << "        error: null,\n";
        js_file << "        progress: 0\n";
        js_file << "    };\n";
        js_file << "    \n";
        js_file << "    // DOM elements\n";
        js_file << "    let elements = {};\n";
        js_file << "    \n";
        js_file << "    // Initialize DOM references\n";
        js_file << "    function initializeDOM() {\n";
        js_file << "        elements.canvas = document.getElementById('canvas');\n";
        js_file << "        elements.loadingScreen = document.getElementById('loadingScreen');\n";
        js_file << "        elements.loadingText = document.getElementById('loadingText');\n";
        js_file << "        elements.loadingStatus = document.getElementById('loadingStatus');\n";
        js_file << "        elements.progressFill = document.getElementById('progressFill');\n";
        js_file << "        elements.errorScreen = document.getElementById('errorScreen');\n";
        js_file << "        elements.errorMessage = document.getElementById('errorMessage');\n";
        js_file << "    }\n";
        js_file << "    \n";
        js_file << "    // Update loading progress\n";
        js_file << "    function updateProgress(progress, status) {\n";
        js_file << "        gameState.progress = progress;\n";
        js_file << "        if (elements.progressFill) {\n";
        js_file << "            elements.progressFill.style.width = (progress * 100) + '%';\n";
        js_file << "        }\n";
        js_file << "        if (elements.loadingStatus && status) {\n";
        js_file << "            elements.loadingStatus.textContent = status;\n";
        js_file << "        }\n";
        js_file << "    }\n";
        js_file << "    \n";
        js_file << "    // Show error screen\n";
        js_file << "    function showError(message) {\n";
        js_file << "        gameState.error = message;\n";
        js_file << "        if (elements.loadingScreen) {\n";
        js_file << "            elements.loadingScreen.style.display = 'none';\n";
        js_file << "        }\n";
        js_file << "        if (elements.errorScreen) {\n";
        js_file << "            elements.errorScreen.style.display = 'flex';\n";
        js_file << "        }\n";
        js_file << "        if (elements.errorMessage) {\n";
        js_file << "            elements.errorMessage.textContent = message;\n";
        js_file << "        }\n";
        js_file << "        console.error('Lupine Game Error:', message);\n";
        js_file << "    }\n";
        js_file << "    \n";
        js_file << "    // Hide loading screen and show game\n";
        js_file << "    function showGame() {\n";
        js_file << "        gameState.loaded = true;\n";
        js_file << "        gameState.running = true;\n";
        js_file << "        if (elements.loadingScreen) {\n";
        js_file << "            elements.loadingScreen.style.display = 'none';\n";
        js_file << "        }\n";
        js_file << "        console.log('Lupine Game loaded successfully');\n";
        js_file << "    }\n";
        js_file << "    \n";
        js_file << "    // Handle canvas resize\n";
        js_file << "    function handleResize() {\n";
        js_file << "        if (!elements.canvas) return;\n";
        js_file << "        \n";
        js_file << "        const container = elements.canvas.parentElement;\n";
        js_file << "        const containerRect = container.getBoundingClientRect();\n";
        js_file << "        \n";
        js_file << "        const gameAspect = " << canvas_size.first << " / " << canvas_size.second << ";\n";
        js_file << "        const containerAspect = containerRect.width / containerRect.height;\n";
        js_file << "        \n";
        js_file << "        if (containerAspect > gameAspect) {\n";
        js_file << "            elements.canvas.style.height = containerRect.height + 'px';\n";
        js_file << "            elements.canvas.style.width = (containerRect.height * gameAspect) + 'px';\n";
        js_file << "        } else {\n";
        js_file << "            elements.canvas.style.width = containerRect.width + 'px';\n";
        js_file << "            elements.canvas.style.height = (containerRect.width / gameAspect) + 'px';\n";
        js_file << "        }\n";
        js_file << "    }\n";
        js_file << "    \n";
        js_file << "    // Configure Emscripten Module\n";
        js_file << "    window.Module = {\n";
        js_file << "        canvas: function() { return elements.canvas; },\n";
        js_file << "        \n";
        js_file << "        preRun: [],\n";
        js_file << "        postRun: [],\n";
        js_file << "        \n";
        js_file << "        onRuntimeInitialized: function() {\n";
        js_file << "            updateProgress(1.0, 'Starting game...');\n";
        js_file << "            setTimeout(function() {\n";
        js_file << "                try {\n";
        js_file << "                    // Call the main function\n";
        js_file << "                    if (typeof Module.ccall === 'function') {\n";
        js_file << "                        Module.ccall('lupine_main', 'number', [], []);\n";
        js_file << "                    }\n";
        js_file << "                    showGame();\n";
        js_file << "                } catch (e) {\n";
        js_file << "                    showError('Failed to start game: ' + e.message);\n";
        js_file << "                }\n";
        js_file << "            }, 100);\n";
        js_file << "        },\n";
        js_file << "        \n";
        js_file << "        onAbort: function(what) {\n";
        js_file << "            showError('Game crashed: ' + what);\n";
        js_file << "        },\n";
        js_file << "        \n";
        js_file << "        print: function(text) {\n";
        js_file << "            console.log('[Game]', text);\n";
        js_file << "        },\n";
        js_file << "        \n";
        js_file << "        printErr: function(text) {\n";
        js_file << "            console.error('[Game Error]', text);\n";
        js_file << "        },\n";
        js_file << "        \n";
        js_file << "        setStatus: function(text) {\n";
        js_file << "            if (text) {\n";
        js_file << "                updateProgress(gameState.progress, text);\n";
        js_file << "            }\n";
        js_file << "        },\n";
        js_file << "        \n";
        js_file << "        totalDependencies: 0,\n";
        js_file << "        monitorRunDependencies: function(left) {\n";
        js_file << "            this.totalDependencies = Math.max(this.totalDependencies, left);\n";
        js_file << "            const progress = this.totalDependencies > 0 ? \n";
        js_file << "                (this.totalDependencies - left) / this.totalDependencies : 0;\n";
        js_file << "            updateProgress(progress * 0.9, left > 0 ? 'Loading dependencies...' : 'Initializing...');\n";
        js_file << "        }\n";
        js_file << "    };\n";
        js_file << "    \n";
        js_file << "    // Initialize when DOM is ready\n";
        js_file << "    if (document.readyState === 'loading') {\n";
        js_file << "        document.addEventListener('DOMContentLoaded', initializeDOM);\n";
        js_file << "    } else {\n";
        js_file << "        initializeDOM();\n";
        js_file << "    }\n";
        js_file << "    \n";
        js_file << "    // Handle window resize\n";
        js_file << "    window.addEventListener('resize', handleResize);\n";
        js_file << "    window.addEventListener('orientationchange', function() {\n";
        js_file << "        setTimeout(handleResize, 100);\n";
        js_file << "    });\n";
        js_file << "    \n";
        js_file << "    // Initial resize\n";
        js_file << "    setTimeout(handleResize, 100);\n";
        js_file << "    \n";
        js_file << "    // Expose game state for debugging\n";
        js_file << "    window.LupineGame = {\n";
        js_file << "        getState: function() { return gameState; },\n";
        js_file << "        restart: function() { location.reload(); }\n";
        js_file << "    };\n";
        js_file << "    \n";
        js_file << "})();\n";

        js_file.close();
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to generate JavaScript loader: " << e.what() << std::endl;
        return false;
    }
}

bool WebExporter::PackageWebAssets(const Project* project,
                                  const std::filesystem::path& output_dir) {
    try {
        if (!project) {
            std::cerr << "No project provided for asset packaging" << std::endl;
            return false;
        }

        std::filesystem::path project_dir = project->GetProjectDirectory();

        // Create assets directory in output
        auto assets_dest = output_dir / "assets";
        std::filesystem::create_directories(assets_dest);

        // Copy main assets directory
        auto assets_src = project_dir / "assets";
        if (std::filesystem::exists(assets_src)) {
            std::cout << "Copying assets from: " << assets_src << std::endl;
            std::filesystem::copy(assets_src, assets_dest,
                                 std::filesystem::copy_options::recursive |
                                 std::filesystem::copy_options::overwrite_existing);
        }

        // Copy scenes
        auto scenes_src = project_dir / "scenes";
        auto scenes_dest = assets_dest / "scenes";
        if (std::filesystem::exists(scenes_src)) {
            std::cout << "Copying scenes from: " << scenes_src << std::endl;
            std::filesystem::copy(scenes_src, scenes_dest,
                                 std::filesystem::copy_options::recursive |
                                 std::filesystem::copy_options::overwrite_existing);
        }

        // Copy scripts
        auto scripts_src = project_dir / "scripts";
        auto scripts_dest = assets_dest / "scripts";
        if (std::filesystem::exists(scripts_src)) {
            std::cout << "Copying scripts from: " << scripts_src << std::endl;
            std::filesystem::copy(scripts_src, scripts_dest,
                                 std::filesystem::copy_options::recursive |
                                 std::filesystem::copy_options::overwrite_existing);
        }

        // Copy project file
        auto project_file_src = project_dir / (project->GetName() + ".lupine");
        auto project_file_dest = assets_dest / "project.lupine";
        if (std::filesystem::exists(project_file_src)) {
            std::cout << "Copying project file: " << project_file_src << std::endl;
            std::filesystem::copy_file(project_file_src, project_file_dest,
                                      std::filesystem::copy_options::overwrite_existing);
        }

        // Copy autoload scripts
        auto autoloads_src = project_dir / "autoloads";
        auto autoloads_dest = assets_dest / "autoloads";
        if (std::filesystem::exists(autoloads_src)) {
            std::cout << "Copying autoloads from: " << autoloads_src << std::endl;
            std::filesystem::copy(autoloads_src, autoloads_dest,
                                 std::filesystem::copy_options::recursive |
                                 std::filesystem::copy_options::overwrite_existing);
        }

        // Create asset manifest for web loading
        if (!CreateAssetManifest(assets_dest)) {
            std::cout << "Warning: Failed to create asset manifest" << std::endl;
        }

        // Generate default icon if not present
        auto icon_path = output_dir / "icon.png";
        if (!std::filesystem::exists(icon_path)) {
            if (!GenerateDefaultIcon(icon_path)) {
                std::cout << "Warning: Failed to generate default icon" << std::endl;
            }
        }

        std::cout << "Asset packaging completed successfully" << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to package web assets: " << e.what() << std::endl;
        return false;
    }
}

bool WebExporter::CreateWebManifest(const std::filesystem::path& output_dir,
                                   const ExportConfig& config) {
    try {
        auto manifest_path = output_dir / "manifest.json";
        std::ofstream manifest_file(manifest_path);
        
        if (!manifest_file) {
            return false;
        }
        
        manifest_file << "{\n";
        manifest_file << "    \"name\": \"Lupine Game\",\n";
        manifest_file << "    \"short_name\": \"Game\",\n";
        manifest_file << "    \"description\": \"A game made with Lupine Engine\",\n";
        manifest_file << "    \"start_url\": \"./\",\n";
        manifest_file << "    \"display\": \"fullscreen\",\n";
        manifest_file << "    \"orientation\": \"landscape\",\n";
        manifest_file << "    \"theme_color\": \"#000000\",\n";
        manifest_file << "    \"background_color\": \"#000000\",\n";
        manifest_file << "    \"icons\": [\n";
        manifest_file << "        {\n";
        manifest_file << "            \"src\": \"icon.png\",\n";
        manifest_file << "            \"sizes\": \"512x512\",\n";
        manifest_file << "            \"type\": \"image/png\"\n";
        manifest_file << "        }\n";
        manifest_file << "    ]\n";
        manifest_file << "}\n";
        
        manifest_file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create web manifest: " << e.what() << std::endl;
        return false;
    }
}

bool WebExporter::GenerateServiceWorker(const std::filesystem::path& output_dir,
                                       const ExportConfig& config) {
    try {
        auto sw_path = output_dir / "sw.js";
        std::ofstream sw_file(sw_path);

        if (!sw_file) {
            return false;
        }

        // Generate cache version based on current time
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::string cache_version = "v" + std::to_string(time_t);

        sw_file << "// Lupine Game Service Worker - Enhanced PWA Support\n";
        sw_file << "const CACHE_NAME = 'lupine-game-" << cache_version << "';\n";
        sw_file << "const GAME_CACHE = 'lupine-game-core';\n";
        sw_file << "const ASSETS_CACHE = 'lupine-game-assets';\n";
        sw_file << "\n";
        sw_file << "// Core game files that should be cached\n";
        sw_file << "const coreFiles = [\n";
        sw_file << "    './',\n";
        sw_file << "    './" << config.executable_name << "',\n";
        sw_file << "    './game.js',\n";
        sw_file << "    './game.wasm',\n";
        sw_file << "    './loader.js',\n";
        sw_file << "    './manifest.json',\n";
        sw_file << "    './icon.png'\n";
        sw_file << "];\n";
        sw_file << "\n";
        sw_file << "// Asset files that can be cached on demand\n";
        sw_file << "const assetPatterns = [\n";
        sw_file << "    /\\/assets\\//,\n";
        sw_file << "    /\\.(png|jpg|jpeg|gif|svg|webp)$/,\n";
        sw_file << "    /\\.(mp3|wav|ogg|m4a)$/,\n";
        sw_file << "    /\\.(json|xml|txt)$/\n";
        sw_file << "];\n";
        sw_file << "\n";
        sw_file << "// Install event - cache core files\n";
        sw_file << "self.addEventListener('install', function(event) {\n";
        sw_file << "    console.log('[SW] Installing service worker');\n";
        sw_file << "    event.waitUntil(\n";
        sw_file << "        caches.open(GAME_CACHE)\n";
        sw_file << "            .then(function(cache) {\n";
        sw_file << "                console.log('[SW] Caching core files');\n";
        sw_file << "                return cache.addAll(coreFiles);\n";
        sw_file << "            })\n";
        sw_file << "            .then(function() {\n";
        sw_file << "                console.log('[SW] Core files cached successfully');\n";
        sw_file << "                return self.skipWaiting();\n";
        sw_file << "            })\n";
        sw_file << "            .catch(function(error) {\n";
        sw_file << "                console.error('[SW] Failed to cache core files:', error);\n";
        sw_file << "            })\n";
        sw_file << "    );\n";
        sw_file << "});\n";
        sw_file << "\n";
        sw_file << "// Activate event - clean up old caches\n";
        sw_file << "self.addEventListener('activate', function(event) {\n";
        sw_file << "    console.log('[SW] Activating service worker');\n";
        sw_file << "    event.waitUntil(\n";
        sw_file << "        caches.keys().then(function(cacheNames) {\n";
        sw_file << "            return Promise.all(\n";
        sw_file << "                cacheNames.map(function(cacheName) {\n";
        sw_file << "                    if (cacheName !== GAME_CACHE && cacheName !== ASSETS_CACHE) {\n";
        sw_file << "                        console.log('[SW] Deleting old cache:', cacheName);\n";
        sw_file << "                        return caches.delete(cacheName);\n";
        sw_file << "                    }\n";
        sw_file << "                })\n";
        sw_file << "            );\n";
        sw_file << "        })\n";
        sw_file << "        .then(function() {\n";
        sw_file << "            return self.clients.claim();\n";
        sw_file << "        })\n";
        sw_file << "    );\n";
        sw_file << "});\n";
        sw_file << "\n";
        sw_file << "// Fetch event - serve from cache with fallback\n";
        sw_file << "self.addEventListener('fetch', function(event) {\n";
        sw_file << "    const url = new URL(event.request.url);\n";
        sw_file << "    \n";
        sw_file << "    // Skip cross-origin requests\n";
        sw_file << "    if (url.origin !== location.origin) {\n";
        sw_file << "        return;\n";
        sw_file << "    }\n";
        sw_file << "    \n";
        sw_file << "    event.respondWith(\n";
        sw_file << "        caches.match(event.request)\n";
        sw_file << "            .then(function(response) {\n";
        sw_file << "                if (response) {\n";
        sw_file << "                    return response;\n";
        sw_file << "                }\n";
        sw_file << "                \n";
        sw_file << "                // Check if this is an asset that should be cached\n";
        sw_file << "                const isAsset = assetPatterns.some(pattern => \n";
        sw_file << "                    pattern.test(event.request.url)\n";
        sw_file << "                );\n";
        sw_file << "                \n";
        sw_file << "                if (isAsset) {\n";
        sw_file << "                    return fetch(event.request)\n";
        sw_file << "                        .then(function(response) {\n";
        sw_file << "                            if (response.status === 200) {\n";
        sw_file << "                                const responseClone = response.clone();\n";
        sw_file << "                                caches.open(ASSETS_CACHE)\n";
        sw_file << "                                    .then(function(cache) {\n";
        sw_file << "                                        cache.put(event.request, responseClone);\n";
        sw_file << "                                    });\n";
        sw_file << "                            }\n";
        sw_file << "                            return response;\n";
        sw_file << "                        })\n";
        sw_file << "                        .catch(function() {\n";
        sw_file << "                            console.warn('[SW] Failed to fetch asset:', event.request.url);\n";
        sw_file << "                            return new Response('Asset not available offline', {\n";
        sw_file << "                                status: 503,\n";
        sw_file << "                                statusText: 'Service Unavailable'\n";
        sw_file << "                            });\n";
        sw_file << "                        });\n";
        sw_file << "                }\n";
        sw_file << "                \n";
        sw_file << "                return fetch(event.request);\n";
        sw_file << "            })\n";
        sw_file << "    );\n";
        sw_file << "});\n";
        sw_file << "\n";
        sw_file << "// Message handling for cache updates\n";
        sw_file << "self.addEventListener('message', function(event) {\n";
        sw_file << "    if (event.data && event.data.type === 'SKIP_WAITING') {\n";
        sw_file << "        self.skipWaiting();\n";
        sw_file << "    }\n";
        sw_file << "});\n";

        sw_file.close();
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to generate service worker: " << e.what() << std::endl;
        return false;
    }
}

bool WebExporter::IsEmscriptenAvailable() const {
    auto emcc_path = GetEmscriptenCompilerPath();
    bool available = !emcc_path.empty();

    // If we can't find emcc, check if the emsdk directory exists at least
    if (!available) {
        std::filesystem::path emsdk_path = std::filesystem::current_path() / "thirdparty" / "emsdk";
        if (std::filesystem::exists(emsdk_path)) {
            // Emsdk exists, so web export should be available even if emcc isn't immediately found
            available = true;
        }
    }

    return available;
}

std::filesystem::path WebExporter::GetEmscriptenCompilerPath() const {
    // First try to use bundled Emscripten from thirdparty directory (development)
    std::filesystem::path bundled_emsdk = std::filesystem::current_path() / "thirdparty" / "emsdk";

    if (std::filesystem::exists(bundled_emsdk)) {
        // Look for emcc in the bundled emsdk
        std::filesystem::path emcc_path;

#ifdef _WIN32
        emcc_path = bundled_emsdk / "upstream" / "emscripten" / "emcc.bat";
        if (std::filesystem::exists(emcc_path)) {
            return emcc_path;
        }

        // Try alternative path structure
        emcc_path = bundled_emsdk / "emscripten" / "emcc.bat";
        if (std::filesystem::exists(emcc_path)) {
            return emcc_path;
        }
#else
        emcc_path = bundled_emsdk / "upstream" / "emscripten" / "emcc";
        if (std::filesystem::exists(emcc_path)) {
            return emcc_path;
        }

        // Try alternative path structure
        emcc_path = bundled_emsdk / "emscripten" / "emcc";
        if (std::filesystem::exists(emcc_path)) {
            return emcc_path;
        }
#endif
    }

    // Try installed Emscripten (when engine is distributed)
    std::filesystem::path exe_dir = std::filesystem::current_path();
    std::filesystem::path installed_emsdk = exe_dir / "share" / "lupine" / "emsdk";

    if (std::filesystem::exists(installed_emsdk)) {
        std::filesystem::path emcc_path;

#ifdef _WIN32
        emcc_path = installed_emsdk / "upstream" / "emscripten" / "emcc.bat";
        if (std::filesystem::exists(emcc_path)) {
            return emcc_path;
        }

        emcc_path = installed_emsdk / "emscripten" / "emcc.bat";
        if (std::filesystem::exists(emcc_path)) {
            return emcc_path;
        }
#else
        emcc_path = installed_emsdk / "upstream" / "emscripten" / "emcc";
        if (std::filesystem::exists(emcc_path)) {
            return emcc_path;
        }

        emcc_path = installed_emsdk / "emscripten" / "emcc";
        if (std::filesystem::exists(emcc_path)) {
            return emcc_path;
        }
#endif
    }

    // Fallback: Check if emcc is available in PATH
    if (std::system("emcc --version > /dev/null 2>&1") == 0) {
        return "emcc";
    }

    return {};
}

bool WebExporter::SetupEmscriptenEnvironment() const {
    std::filesystem::path emsdk_path;

    // Try bundled Emscripten first (development)
    std::filesystem::path bundled_emsdk = std::filesystem::current_path() / "thirdparty" / "emsdk";
    if (std::filesystem::exists(bundled_emsdk)) {
        emsdk_path = bundled_emsdk;
    } else {
        // Try installed Emscripten (distributed engine)
        std::filesystem::path exe_dir = std::filesystem::current_path();
        std::filesystem::path installed_emsdk = exe_dir / "share" / "lupine" / "emsdk";
        if (std::filesystem::exists(installed_emsdk)) {
            emsdk_path = installed_emsdk;
        }
    }

    if (emsdk_path.empty()) {
        std::cerr << "Emscripten SDK not found in bundled or installed locations" << std::endl;
        return false;
    }

    // Set up environment variables for Emscripten
    std::string emsdk_path_str = emsdk_path.string();

#ifdef _WIN32
    // On Windows, set environment variables
    std::string env_command = "set EMSDK=" + emsdk_path_str + " && ";
    env_command += "set EM_CONFIG=" + emsdk_path_str + "\\.emscripten && ";
    env_command += "set EMSDK_NODE=" + emsdk_path_str + "\\node\\22.16.0_64bit\\bin\\node.exe && ";
    env_command += "set EMSDK_PYTHON=" + emsdk_path_str + "\\python\\3.13.3_64bit\\python.exe";

    // Execute environment setup
    int result = std::system(env_command.c_str());
    if (result != 0) {
        std::cerr << "Failed to set up Emscripten environment" << std::endl;
        return false;
    }
#else
    // On Unix-like systems, source the environment script
    std::string env_script = emsdk_path_str + "/emsdk_env.sh";
    if (std::filesystem::exists(env_script)) {
        std::string source_command = "source " + env_script;
        int result = std::system(source_command.c_str());
        if (result != 0) {
            std::cerr << "Failed to source Emscripten environment script" << std::endl;
            return false;
        }
    }
#endif

    return true;
}

std::vector<std::string> WebExporter::GetEmscriptenFlags(const ExportConfig& config) const {
    std::vector<std::string> flags;

    // Basic SDL flags
    flags.push_back("-s USE_SDL=2");
    flags.push_back("-s USE_SDL_IMAGE=2");
    flags.push_back("-s USE_SDL_TTF=2");
    flags.push_back("-s USE_SDL_MIXER=2");

    // WebAssembly settings
    flags.push_back("-s WASM=1");
    flags.push_back("-s ALLOW_MEMORY_GROWTH=1");
    flags.push_back("-s MAXIMUM_MEMORY=2GB");

    // Memory size
    flags.push_back("-s INITIAL_MEMORY=" + std::to_string(config.web.memory_size_mb * 1024 * 1024));

    // Export runtime functions for script integration
    flags.push_back("-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue','UTF8ToString','stringToUTF8']");
    flags.push_back("-s EXPORTED_FUNCTIONS=['_main','_malloc','_free']");

    // File system support for assets
    flags.push_back("-s FORCE_FILESYSTEM=1");

    // Check if assets directory exists before preloading
    std::filesystem::path assets_path = std::filesystem::current_path() / "assets";
    if (std::filesystem::exists(assets_path)) {
        flags.push_back("--preload-file assets@/assets");
    } else {
        std::cout << "Warning: Assets directory not found, skipping asset preloading" << std::endl;
    }

    // Threading support
    if (config.web.enable_threads) {
        flags.push_back("-s USE_PTHREADS=1");
        flags.push_back("-s PTHREAD_POOL_SIZE=4");
        flags.push_back("-s SHARED_MEMORY=1");
    }

    // SIMD support
    if (config.web.enable_simd) {
        flags.push_back("-msimd128");
        flags.push_back("-s SIMD=1");
    }

    // Audio worklet support for better audio performance
    flags.push_back("-s AUDIO_WORKLET=1");
    flags.push_back("-s WEBAUDIO_DEBUG=0");

    // OpenGL settings
    flags.push_back("-s FULL_ES2=1");
    flags.push_back("-s FULL_ES3=1");
    flags.push_back("-s USE_WEBGL2=1");

    // Optimization settings
    if (config.optimize_assets) {
        flags.push_back("-O3");
        flags.push_back("-s ASSERTIONS=0");
        flags.push_back("--closure 1");
    } else {
        flags.push_back("-O1");
        flags.push_back("-s ASSERTIONS=1");
        flags.push_back("-g");
    }

    // Link libraries
    flags.push_back("-lGL");
    flags.push_back("-lal");
    flags.push_back("-lidbfs.js");

    // C++ standard
    flags.push_back("-std=c++17");

    // Graphics backend flags
    flags.push_back("-DLUPINE_WEBGL_BACKEND");

    return flags;
}

std::vector<std::filesystem::path> WebExporter::GetSourceFiles(const Project* project) const {
    std::vector<std::filesystem::path> sources;

    // Add web-specific main entry point
    sources.push_back("src/export/web/WebRuntimeMain.cpp");

    // Core engine sources
    sources.push_back("src/core/Engine.cpp");
    sources.push_back("src/core/Node.cpp");
    sources.push_back("src/core/Scene.cpp");
    sources.push_back("src/core/Component.cpp");
    sources.push_back("src/core/ResourceManager.cpp");
    sources.push_back("src/core/GlobalsManager.cpp");
    sources.push_back("src/core/Project.cpp");

    // Rendering system
    sources.push_back("src/rendering/Renderer.cpp");
    sources.push_back("src/rendering/Camera.cpp");
    sources.push_back("src/rendering/ViewportManager.cpp");
    sources.push_back("src/rendering/LightingSystem.cpp");
    sources.push_back("src/rendering/TextRenderer.cpp");

    // Graphics abstraction layer
    sources.push_back("src/rendering/WebGLDevice.cpp");
    sources.push_back("src/rendering/GraphicsDeviceFactory.cpp");
    sources.push_back("src/rendering/ShaderManager.cpp");

    // Component system
    sources.push_back("src/components/Transform.cpp");
    sources.push_back("src/components/MeshRenderer.cpp");
    sources.push_back("src/components/AudioSource.cpp");
    sources.push_back("src/components/RigidBody2D.cpp");
    sources.push_back("src/components/RigidBody3D.cpp");
    sources.push_back("src/components/CollisionShape2D.cpp");
    sources.push_back("src/components/CollisionShape3D.cpp");

    // Physics system
    sources.push_back("src/physics/PhysicsManager.cpp");
    sources.push_back("src/physics/PhysicsBody2D.cpp");
    sources.push_back("src/physics/PhysicsBody3D.cpp");

    // Audio system
    sources.push_back("src/audio/AudioManager.cpp");

    // Input system
    sources.push_back("src/input/InputManager.cpp");

    // Scripting system
    sources.push_back("src/scripting/LuaScriptComponent.cpp");
    sources.push_back("src/scripting/PythonScriptComponent.cpp");
    sources.push_back("src/scripting/VisualScriptComponent.cpp");
    sources.push_back("src/export/web/WebScriptBridge.cpp");

    // Asset loading
    sources.push_back("src/assets/AssetLoader.cpp");
    sources.push_back("src/assets/AssetBundleReader.cpp");

    // Math utilities
    sources.push_back("src/math/Math.cpp");

    // Add project-specific script files if any
    if (project) {
        auto project_dir = project->GetProjectDirectory();
        std::filesystem::path scripts_dir = std::filesystem::path(project_dir) / "scripts";

        if (std::filesystem::exists(scripts_dir)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(scripts_dir)) {
                if (entry.is_regular_file()) {
                    auto ext = entry.path().extension().string();
                    if (ext == ".cpp" || ext == ".c") {
                        sources.push_back(entry.path());
                    }
                }
            }
        }
    }

    return sources;
}

std::pair<int, int> WebExporter::ParseCanvasSize(const std::string& canvas_size) const {
    size_t x_pos = canvas_size.find('x');
    if (x_pos != std::string::npos) {
        try {
            int width = std::stoi(canvas_size.substr(0, x_pos));
            int height = std::stoi(canvas_size.substr(x_pos + 1));
            return {width, height};
        } catch (const std::exception&) {
            // Fall through to default
        }
    }

    return {1920, 1080}; // Default size
}

bool WebExporter::CreateAssetManifest(const std::filesystem::path& assets_dir) const {
    try {
        auto manifest_path = assets_dir / "manifest.json";
        std::ofstream manifest_file(manifest_path);

        if (!manifest_file) {
            return false;
        }

        manifest_file << "{\n";
        manifest_file << "    \"version\": \"1.0\",\n";
        manifest_file << "    \"assets\": [\n";

        bool first = true;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(assets_dir)) {
            if (entry.is_regular_file() && entry.path().filename() != "manifest.json") {
                if (!first) {
                    manifest_file << ",\n";
                }

                auto relative_path = std::filesystem::relative(entry.path(), assets_dir);
                manifest_file << "        {\n";
                manifest_file << "            \"path\": \"" << relative_path.generic_string() << "\",\n";
                manifest_file << "            \"size\": " << entry.file_size() << "\n";
                manifest_file << "        }";
                first = false;
            }
        }

        manifest_file << "\n    ]\n";
        manifest_file << "}\n";

        manifest_file.close();
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to create asset manifest: " << e.what() << std::endl;
        return false;
    }
}

bool WebExporter::GenerateDefaultIcon(const std::filesystem::path& icon_path) const {
    try {
        // Create a simple 512x512 PNG icon
        // This is a placeholder - in a real implementation, you'd use a proper image library
        std::ofstream icon_file(icon_path, std::ios::binary);

        if (!icon_file) {
            return false;
        }

        // Simple 1x1 transparent PNG (placeholder)
        const unsigned char png_data[] = {
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
            0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
            0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
            0x89, 0x00, 0x00, 0x00, 0x0B, 0x49, 0x44, 0x41,
            0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
            0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
            0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
            0x42, 0x60, 0x82
        };

        icon_file.write(reinterpret_cast<const char*>(png_data), sizeof(png_data));
        icon_file.close();

        std::cout << "Generated default icon: " << icon_path << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to generate default icon: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> WebExporter::GetIncludeDirectories() const {
    std::vector<std::string> includes;

    // Engine include directories
    includes.push_back("-Iinclude");
    includes.push_back("-Isrc");
    includes.push_back("-Ithirdparty/lua/include");
    includes.push_back("-Ithirdparty/pybind11/include");
    includes.push_back("-Ithirdparty/glm");
    includes.push_back("-Ithirdparty/glad/include");
    includes.push_back("-Ithirdparty/box2d/include");
    includes.push_back("-Ithirdparty/bullet3/src");
    includes.push_back("-Ithirdparty/yaml-cpp/include");
    includes.push_back("-Ithirdparty/spdlog/include");
    includes.push_back("-Ithirdparty/assimp/include");

    return includes;
}

std::vector<std::string> WebExporter::GetLibraryDirectories() const {
    std::vector<std::string> lib_dirs;

    // These would be web-specific library paths
    lib_dirs.push_back("-Lthirdparty/web/lib");

    return lib_dirs;
}

std::vector<std::string> WebExporter::GetLibraries() const {
    std::vector<std::string> libs;

    // Web-specific libraries
    libs.push_back("-lGL");
    libs.push_back("-lal");
    libs.push_back("-lidbfs.js");

    return libs;
}

} // namespace Lupine
