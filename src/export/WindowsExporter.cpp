#include "lupine/export/WindowsExporter.h"
#include "lupine/export/AssetBundler.h"
#include "lupine/export/EmbeddedLibraryManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Lupine {

WindowsExporter::WindowsExporter() = default;

bool WindowsExporter::IsAvailable() const {
#ifdef _WIN32
    // Check if runtime executable exists
    auto runtime_path = GetRuntimeExecutablePath();
    return std::filesystem::exists(runtime_path);
#else
    // Windows export is available on other platforms with embedded libraries
    auto& embedded_manager = GetEmbeddedLibraryManager();
    if (embedded_manager.HasLibrariesForPlatform("windows", "x64")) {
        return true;
    }

    // Fallback: Check if we have Windows static libraries in thirdparty directory
    std::filesystem::path windows_libs = "thirdparty/windows_x64-static";
    if (std::filesystem::exists(windows_libs)) {
        return true;
    }

    return false;
#endif
}

ExportResult WindowsExporter::Export(const Project* project, const ExportConfig& config,
                                    ExportProgressCallback progress_callback) {
    ExportResult result;

    try {
        std::filesystem::path output_dir(config.output_directory);
        std::filesystem::create_directories(output_dir);

        if (progress_callback) {
            progress_callback(0.1f, "Preparing standalone executable...");
        }

        // Get runtime executable path
        auto runtime_path = GetRuntimeExecutablePath();
        if (!std::filesystem::exists(runtime_path)) {
            result.error_message = "Runtime executable not found";
            return result;
        }

        // Create final executable path
        std::filesystem::path final_executable = output_dir / config.executable_name;

        if (progress_callback) {
            progress_callback(0.2f, "Copying runtime executable...");
        }

        // Copy runtime as base
        std::filesystem::copy_file(runtime_path, final_executable,
                                  std::filesystem::copy_options::overwrite_existing);

        if (progress_callback) {
            progress_callback(0.3f, "Creating comprehensive asset bundle...");
        }

        // Create asset bundle with project files AND all runtime dependencies
        AssetBundler bundler;
        bundler.SetOptimizeAssets(config.optimize_assets);

        if (progress_callback) {
            progress_callback(0.4f, "Adding project and all dependencies...");
        }

        // Use the new AddProject method to add everything at once
        if (!bundler.AddProject(project->GetFilePath(), true)) {
            result.error_message = "Failed to add project to bundle";
            return result;
        }

        // Add Qt plugin directories to bundle
        auto runtime_dir = runtime_path.parent_path();
        std::vector<std::string> plugin_dirs = {
            "platforms", "imageformats", "styles", "generic",
            "networkinformation", "sqldrivers", "tls"
        };

        for (const auto& plugin_dir : plugin_dirs) {
            auto source_dir = runtime_dir / plugin_dir;
            if (std::filesystem::exists(source_dir)) {
                bundler.AddAssetsFromDirectory(source_dir, "runtime_plugins/" + plugin_dir);
                std::cout << "Adding plugin directory: " << plugin_dir << std::endl;
            }
        }

        if (progress_callback) {
            progress_callback(0.7f, "Embedding assets in executable...");
        }

        // Create temporary bundle
        auto temp_bundle = output_dir / "temp_assets.bundle";
        if (!bundler.CreateBundle(temp_bundle)) {
            result.error_message = "Failed to create asset bundle";
            return result;
        }

        // Embed bundle in executable
        if (!bundler.EmbedBundleInExecutable(final_executable, temp_bundle)) {
            result.error_message = "Failed to embed assets in executable";
            std::filesystem::remove(temp_bundle);
            return result;
        }

        // Clean up temporary bundle
        std::filesystem::remove(temp_bundle);

        if (progress_callback) {
            progress_callback(0.9f, "Finalizing export...");
        }
        
        // Set executable icon if specified in config or use project icon
        std::filesystem::path icon_to_use;
        if (!config.windows.icon_path.empty()) {
            icon_to_use = config.windows.icon_path;
        } else {
            // Look for icon.ico in project root directory
            std::filesystem::path project_icon = project->GetProjectDirectory() + "/icon.ico";
            if (std::filesystem::exists(project_icon)) {
                icon_to_use = project_icon;
                std::cout << "Using project icon: " << project_icon << std::endl;
            }
        }

        if (!icon_to_use.empty()) {
            std::filesystem::path exe_path = output_dir / config.executable_name;
            SetExecutableIcon(exe_path, icon_to_use);
        }
        
        // Calculate total size
        result.total_size_bytes = CalculateExportSize(project, config);
        result.output_path = (output_dir / config.executable_name).string();
        result.success = true;
        
        // List generated files
        for (const auto& entry : std::filesystem::recursive_directory_iterator(output_dir)) {
            if (entry.is_regular_file()) {
                result.generated_files.push_back(entry.path().string());
            }
        }
        
    } catch (const std::exception& e) {
        result.error_message = "Export failed: " + std::string(e.what());
    }
    
    return result;
}

bool WindowsExporter::CopyRuntimeExecutable(const std::filesystem::path& output_dir, 
                                           const std::string& executable_name,
                                           bool include_debug_symbols) {
    try {
        auto runtime_path = GetRuntimeExecutablePath();
        auto output_path = output_dir / executable_name;
        
        std::filesystem::copy_file(runtime_path, output_path, 
                                  std::filesystem::copy_options::overwrite_existing);
        
        // Copy debug symbols if requested
        if (include_debug_symbols) {
            auto pdb_path = runtime_path;
            pdb_path.replace_extension(".pdb");
            if (std::filesystem::exists(pdb_path)) {
                auto output_pdb = output_path;
                output_pdb.replace_extension(".pdb");
                std::filesystem::copy_file(pdb_path, output_pdb,
                                          std::filesystem::copy_options::overwrite_existing);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy runtime executable: " << e.what() << std::endl;
        return false;
    }
}

bool WindowsExporter::CopyRequiredDLLs(const std::filesystem::path& output_dir) {
    try {
        auto dll_list = GetRequiredDLLs();

        // Copy DLL files
        for (const auto& dll_path : dll_list) {
            if (std::filesystem::exists(dll_path)) {
                auto output_path = output_dir / dll_path.filename();
                std::filesystem::copy_file(dll_path, output_path,
                                          std::filesystem::copy_options::overwrite_existing);
            }
        }

        // Copy Qt plugin directories
        auto runtime_dir = GetRuntimeExecutablePath().parent_path();
        std::vector<std::string> plugin_dirs = {
            "platforms",
            "imageformats",
            "styles",
            "generic",
            "networkinformation",
            "sqldrivers",
            "tls"
        };

        for (const auto& plugin_dir : plugin_dirs) {
            auto source_dir = runtime_dir / plugin_dir;
            auto dest_dir = output_dir / plugin_dir;

            if (std::filesystem::exists(source_dir)) {
                std::filesystem::copy(source_dir, dest_dir,
                                    std::filesystem::copy_options::recursive |
                                    std::filesystem::copy_options::overwrite_existing);
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy DLLs: " << e.what() << std::endl;
        return false;
    }
}

bool WindowsExporter::CopyProjectAssets(const Project* project,
                                       const std::filesystem::path& output_dir,
                                       bool embed_assets) {
    try {
        std::filesystem::path project_dir = project->GetProjectDirectory();

        if (embed_assets) {
            // Create asset bundle
            AssetBundler bundler;
            bundler.SetOptimizeAssets(true);

            // Add project assets
            std::vector<std::string> dirs_to_bundle = {"assets", "scenes", "scripts"};

            for (const auto& dir_name : dirs_to_bundle) {
                auto source_dir = project_dir / dir_name;
                if (std::filesystem::exists(source_dir)) {
                    bundler.AddAssetsFromDirectory(source_dir, dir_name);
                }
            }

            // Add project file
            bundler.AddAsset(project->GetFilePath(), "project.lupine");

            // Create bundle file
            auto bundle_path = output_dir / "assets.bundle";
            if (!bundler.CreateBundle(bundle_path)) {
                return false;
            }

            // Embed bundle in executable
            auto runtime_path = GetRuntimeExecutablePath();
            auto output_exe = output_dir / "temp_runtime.exe";

            // First copy the runtime
            std::filesystem::copy_file(runtime_path, output_exe,
                                      std::filesystem::copy_options::overwrite_existing);

            // Then embed the bundle
            if (!bundler.EmbedBundleInExecutable(output_exe, bundle_path)) {
                return false;
            }

            // Remove temporary bundle file
            std::filesystem::remove(bundle_path);

        } else {
            // Copy project file and assets directory
            auto project_file = std::filesystem::path(project->GetFilePath());
            auto output_project = output_dir / project_file.filename();
            std::filesystem::copy_file(project_file, output_project,
                                      std::filesystem::copy_options::overwrite_existing);

            // Copy assets, scenes, and scripts directories
            std::vector<std::string> dirs_to_copy = {"assets", "scenes", "scripts"};

            for (const auto& dir_name : dirs_to_copy) {
                auto source_dir = project_dir / dir_name;
                auto dest_dir = output_dir / dir_name;

                if (std::filesystem::exists(source_dir)) {
                    std::filesystem::copy(source_dir, dest_dir,
                                         std::filesystem::copy_options::recursive |
                                         std::filesystem::copy_options::overwrite_existing);
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy project assets: " << e.what() << std::endl;
        return false;
    }
}

bool WindowsExporter::CreateProjectLauncher(const Project* project,
                                           const std::filesystem::path& output_dir,
                                           const std::string& executable_name) {
    try {
        // Get project file name
        std::filesystem::path project_file_path(project->GetFilePath());
        std::string project_filename = project_file_path.filename().string();

        // Rename the runtime executable to the desired name
        std::filesystem::path runtime_copy = output_dir / "lupine-runtime.exe";
        std::filesystem::path final_executable = output_dir / executable_name;

        if (std::filesystem::exists(runtime_copy)) {
            // Rename runtime to the final executable name
            std::filesystem::rename(runtime_copy, final_executable);

            // Create autoload config file so the runtime knows which project to load
            std::filesystem::path config_file = output_dir / "autoload.cfg";
            std::ofstream config(config_file);
            if (config) {
                config << "project=" << project_filename << "\n";
                config << "title=" << project->GetName() << "\n";
                config.close();
            }
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create project launcher: " << e.what() << std::endl;
        return false;
    }
}



bool WindowsExporter::CreateEmbeddedExecutable(const Project* project,
                                              const std::filesystem::path& runtime_path,
                                              const std::filesystem::path& output_path) {
    // This is a simplified implementation
    // In a full implementation, you would embed the project data as a resource
    // or append it to the executable and modify the runtime to read embedded data
    
    try {
        // For now, just copy the runtime and create a data file alongside it
        std::filesystem::copy_file(runtime_path, output_path,
                                  std::filesystem::copy_options::overwrite_existing);
        
        // Create embedded data file (this would be embedded in the exe in a full implementation)
        auto data_file = output_path;
        data_file.replace_extension(".dat");
        
        std::ofstream data_stream(data_file, std::ios::binary);
        if (!data_stream) {
            return false;
        }
        
        // Write project file path (relative to executable)
        std::string project_file = std::filesystem::path(project->GetFilePath()).filename().string();
        data_stream.write(project_file.c_str(), project_file.size() + 1);
        
        data_stream.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create embedded executable: " << e.what() << std::endl;
        return false;
    }
}

bool WindowsExporter::SetExecutableIcon(const std::filesystem::path& executable_path,
                                       const std::filesystem::path& icon_path) {
#ifdef _WIN32
    if (!std::filesystem::exists(icon_path)) {
        std::cerr << "Icon file does not exist: " << icon_path << std::endl;
        return false;
    }

    // Load the icon file
    HICON hIcon = (HICON)LoadImageA(NULL, icon_path.string().c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (!hIcon) {
        std::cerr << "Failed to load icon file: " << icon_path << std::endl;
        return false;
    }

    // Begin updating the executable resources
    HANDLE hUpdate = BeginUpdateResourceA(executable_path.string().c_str(), FALSE);
    if (!hUpdate) {
        std::cerr << "Failed to begin resource update for: " << executable_path << std::endl;
        DestroyIcon(hIcon);
        return false;
    }

    // Get icon data
    ICONINFO iconInfo;
    if (!GetIconInfo(hIcon, &iconInfo)) {
        std::cerr << "Failed to get icon info" << std::endl;
        EndUpdateResource(hUpdate, TRUE);
        DestroyIcon(hIcon);
        return false;
    }

    // For simplicity, we'll just copy the icon file data directly
    // In a full implementation, you would extract the icon data properly
    std::ifstream icon_file(icon_path, std::ios::binary);
    if (!icon_file) {
        std::cerr << "Failed to open icon file for reading: " << icon_path << std::endl;
        EndUpdateResource(hUpdate, TRUE);
        DestroyIcon(hIcon);
        return false;
    }

    // Get file size
    icon_file.seekg(0, std::ios::end);
    size_t file_size = icon_file.tellg();
    icon_file.seekg(0, std::ios::beg);

    // Read icon data
    std::vector<char> icon_data(file_size);
    icon_file.read(icon_data.data(), file_size);
    icon_file.close();

    // Update the icon resource (ID 1 is typically the main application icon)
    BOOL result = UpdateResourceA(hUpdate, RT_ICON, MAKEINTRESOURCEA(1),
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                                 icon_data.data(), static_cast<DWORD>(file_size));

    if (!result) {
        std::cerr << "Failed to update icon resource" << std::endl;
        EndUpdateResource(hUpdate, TRUE);
        DestroyIcon(hIcon);
        return false;
    }

    // Commit the resource update
    result = EndUpdateResource(hUpdate, FALSE);
    DestroyIcon(hIcon);

    if (result) {
        std::cout << "Successfully updated executable icon: " << executable_path << std::endl;
        return true;
    } else {
        std::cerr << "Failed to commit resource update" << std::endl;
        return false;
    }
#else
    std::cerr << "Icon setting is only supported on Windows" << std::endl;
    return false;
#endif
}

std::vector<std::filesystem::path> WindowsExporter::GetRequiredDLLs() const {
    std::vector<std::filesystem::path> dlls;
    
    // Get the directory where the runtime executable is located
    auto runtime_dir = GetRuntimeExecutablePath().parent_path();
    
    // List of required DLLs (only runtime dependencies, not Qt editor DLLs)
    std::vector<std::string> dll_names = {
        // Core SDL and graphics
        "SDL2.dll", "SDL2_image.dll", "SDL2_ttf.dll",

        // 3D model loading
        "assimp-vc143-mt.dll", "libassimp-5.dll", "libassimp.dll",

        // Scripting
        "lua.dll", "lua51.dll", "python3.dll", "python312.dll",

        // Physics
        "box2d.dll",

        // Image/compression libraries
        "libpng16.dll", "freetype.dll", "harfbuzz.dll",
        "brotlicommon.dll", "brotlidec.dll", "bz2.dll", "zlib1.dll", "zstd.dll",

        // Other dependencies
        "double-conversion.dll", "fmt.dll", "icudt74.dll", "icuin74.dll", "icuuc74.dll",
        "minizip.dll", "pcre2-16.dll", "poly2tri.dll", "pugixml.dll"
    };
    
    for (const auto& dll_name : dll_names) {
        auto dll_path = runtime_dir / dll_name;
        if (std::filesystem::exists(dll_path)) {
            dlls.push_back(dll_path);
        }
    }
    
    return dlls;
}

std::filesystem::path WindowsExporter::GetRuntimeExecutablePath() const {
    // Look for runtime executable in common locations
    std::vector<std::filesystem::path> search_paths = {
        "build/bin/Release/lupine-runtime.exe",
        "build/bin/Debug/lupine-runtime.exe",
        "build/bin/RelWithDebInfo/lupine-runtime.exe",
        "build/bin/MinSizeRel/lupine-runtime.exe",
        "bin/Release/lupine-runtime.exe",
        "bin/Debug/lupine-runtime.exe",
        "bin/lupine-runtime.exe",
        "lupine-runtime.exe"
    };

    for (const auto& path : search_paths) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::absolute(path);
        }
    }

    // Try to find it relative to the current executable (editor)
    try {
        auto current_dir = std::filesystem::current_path();
        auto runtime_path = current_dir / "lupine-runtime.exe";
        if (std::filesystem::exists(runtime_path)) {
            return std::filesystem::absolute(runtime_path);
        }
    } catch (...) {
        // Ignore errors
    }

    return "lupine-runtime.exe"; // Fallback
}

size_t WindowsExporter::CalculateExportSize(const Project* project, const ExportConfig& config) const {
    size_t total_size = 0;
    
    try {
        // Runtime executable size
        auto runtime_path = GetRuntimeExecutablePath();
        if (std::filesystem::exists(runtime_path)) {
            total_size += std::filesystem::file_size(runtime_path);
        }
        
        // DLL sizes
        auto dlls = GetRequiredDLLs();
        for (const auto& dll : dlls) {
            if (std::filesystem::exists(dll)) {
                total_size += std::filesystem::file_size(dll);
            }
        }
        
        // Project assets size
        std::filesystem::path project_dir = project->GetProjectDirectory();
        for (const auto& entry : std::filesystem::recursive_directory_iterator(project_dir)) {
            if (entry.is_regular_file()) {
                total_size += entry.file_size();
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error calculating export size: " << e.what() << std::endl;
    }
    
    return total_size;
}

EmbeddedLibraryManager& WindowsExporter::GetEmbeddedLibraryManager() const {
    static EmbeddedLibraryManager instance;
    return instance;
}

} // namespace Lupine
