#include "lupine/export/LinuxExporter.h"
#include "lupine/export/LinuxPackageBuilder.h"
#include "lupine/export/AssetBundler.h"
#include "lupine/export/EmbeddedLibraryManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace Lupine {

LinuxExporter::LinuxExporter() = default;

bool LinuxExporter::IsAvailable() const {
    // Linux export is available on any platform with cross-compilation support

    // First check if we have embedded Linux libraries
    auto& embedded_manager = GetEmbeddedLibraryManager();
    if (embedded_manager.HasLibrariesForPlatform("linux", "x64")) {
        // We have embedded libraries, so Linux export is available
        return true;
    }

    // Fallback: Check if we have Linux static libraries in thirdparty directory
    std::filesystem::path linux_libs = "thirdparty/linux_x64-static";
    if (std::filesystem::exists(linux_libs)) {
        return true;
    }

    // No Linux libraries available
    return false;
}

ExportResult LinuxExporter::Export(const Project* project, const ExportConfig& config,
                                  ExportProgressCallback progress_callback) {
    ExportResult result;

    try {
        std::filesystem::path output_dir(config.output_directory);
        std::filesystem::create_directories(output_dir);

        if (progress_callback) {
            progress_callback(0.1f, "Preparing Linux export...");
        }

        // Create temporary build directory
        auto temp_dir = output_dir / "temp_linux_build";
        std::filesystem::create_directories(temp_dir);

        if (progress_callback) {
            progress_callback(0.2f, "Building Linux runtime executable...");
        }

        // Build or copy Linux runtime executable
        auto runtime_path = BuildLinuxRuntime(temp_dir);
        if (runtime_path.empty()) {
            result.error_message = "Failed to build Linux runtime executable";
            return result;
        }

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

        // Create bundle file
        auto bundle_path = temp_dir / "assets.bundle";
        if (!bundler.CreateBundle(bundle_path)) {
            result.error_message = "Failed to create asset bundle";
            return result;
        }

        if (progress_callback) {
            progress_callback(0.5f, "Setting up package builder...");
        }

        // Set up Linux package builder
        LinuxPackageBuilder package_builder;

        // Configure package metadata from project and config
        LinuxPackageMetadata metadata = CreatePackageMetadata(project, config);
        package_builder.SetMetadata(metadata);

        // Set directories
        package_builder.SetSourceDirectory(temp_dir);
        package_builder.SetOutputDirectory(output_dir);

        // Add runtime executable
        package_builder.AddFile(runtime_path, "usr/bin/" + GetExecutableName(config), 0755);

        // Add asset bundle
        package_builder.AddFile(bundle_path, "usr/share/" + metadata.name + "/assets.bundle", 0644);

        if (progress_callback) {
            progress_callback(0.6f, "Adding static libraries...");
        }

        // Add all required static libraries
        auto static_libs = GetRequiredStaticLibraries();
        package_builder.AddStaticLibraries(static_libs);

        if (progress_callback) {
            progress_callback(0.7f, "Building Linux packages...");
        }

        // Determine package format from executable name or build all supported formats
        LinuxPackageFormat target_format = DeterminePackageFormat(config.executable_name);

        std::vector<PackageBuildResult> build_results;
        if (target_format == LinuxPackageFormat::All) {
            // Build all supported formats
            build_results = package_builder.BuildAllPackages([&](float progress, const std::string& status) {
                if (progress_callback) {
                    progress_callback(0.7f + (progress * 0.3f), status);
                }
            });
        } else {
            // Build specific format
            auto build_result = package_builder.BuildPackage(target_format, [&](float progress, const std::string& status) {
                if (progress_callback) {
                    progress_callback(0.7f + (progress * 0.3f), status);
                }
            });
            build_results.push_back(build_result);
        }

        // Process build results
        bool any_success = false;
        size_t total_size = 0;

        for (const auto& build_result : build_results) {
            if (build_result.success) {
                any_success = true;
                result.generated_files.push_back(build_result.package_path);
                total_size += build_result.package_size;

                // Set primary output path to first successful build
                if (result.output_path.empty()) {
                    result.output_path = build_result.package_path;
                }
            } else {
                std::cerr << "Failed to build " << PackageFormatToString(build_result.format)
                         << " package: " << build_result.error_message << std::endl;
            }
        }

        if (!any_success) {
            result.error_message = "Failed to build any Linux packages";
            return result;
        }

        result.success = true;
        result.total_size_bytes = total_size;

        // Clean up temporary files
        package_builder.CleanupTempFiles();
        std::filesystem::remove_all(temp_dir);

        if (progress_callback) {
            progress_callback(1.0f, "Linux export completed successfully");
        }

    } catch (const std::exception& e) {
        result.error_message = "Export failed: " + std::string(e.what());
    }

    return result;
}

std::filesystem::path LinuxExporter::CreateAppDir(const std::filesystem::path& output_dir,
                                                 const std::string& app_name) {
    try {
        auto appdir_path = output_dir / (app_name + ".AppDir");
        
        // Create directory structure
        std::filesystem::create_directories(appdir_path);
        std::filesystem::create_directories(appdir_path / "usr" / "bin");
        std::filesystem::create_directories(appdir_path / "usr" / "lib");
        std::filesystem::create_directories(appdir_path / "usr" / "share" / "applications");
        std::filesystem::create_directories(appdir_path / "usr" / "share" / "icons" / "hicolor" / "256x256" / "apps");
        
        return appdir_path;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create AppDir: " << e.what() << std::endl;
        return {};
    }
}

bool LinuxExporter::CopyRuntimeToAppDir(const std::filesystem::path& appdir_path,
                                       const std::string& executable_name) {
    try {
        // Find runtime executable (this would need to be built for Linux)
        std::filesystem::path runtime_path = "build/bin/lupine-runtime"; // Linux executable
        
        if (!std::filesystem::exists(runtime_path)) {
            std::cerr << "Linux runtime executable not found: " << runtime_path << std::endl;
            return false;
        }
        
        auto dest_path = appdir_path / "usr" / "bin" / executable_name;
        std::filesystem::copy_file(runtime_path, dest_path,
                                  std::filesystem::copy_options::overwrite_existing);
        
        // Make executable
        std::filesystem::permissions(dest_path, 
                                   std::filesystem::perms::owner_all | 
                                   std::filesystem::perms::group_read | 
                                   std::filesystem::perms::group_exec |
                                   std::filesystem::perms::others_read | 
                                   std::filesystem::perms::others_exec);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy runtime to AppDir: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxExporter::CopySharedLibraries(const std::filesystem::path& appdir_path) {
    try {
        auto lib_dir = appdir_path / "usr" / "lib";
        auto required_libs = GetRequiredLibraries();
        
        for (const auto& lib_path : required_libs) {
            if (std::filesystem::exists(lib_path)) {
                auto dest_path = lib_dir / lib_path.filename();
                std::filesystem::copy_file(lib_path, dest_path,
                                          std::filesystem::copy_options::overwrite_existing);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy shared libraries: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxExporter::CreateDesktopFile(const std::filesystem::path& appdir_path,
                                     const ExportConfig& config) {
    try {
        auto desktop_path = appdir_path / (config.linux_config.desktop_file_name + ".desktop");
        
        std::ofstream desktop_file(desktop_path);
        if (!desktop_file) {
            return false;
        }
        
        desktop_file << "[Desktop Entry]\n";
        desktop_file << "Type=Application\n";
        desktop_file << "Name=" << config.linux_config.desktop_file_name << "\n";
        desktop_file << "Exec=" << config.executable_name << "\n";
        desktop_file << "Icon=" << config.linux_config.desktop_file_name << "\n";
        desktop_file << "Categories=" << config.linux_config.app_category << ";\n";
        desktop_file << "Terminal=false\n";
        
        desktop_file.close();
        
        // Make executable
        std::filesystem::permissions(desktop_path,
                                   std::filesystem::perms::owner_all |
                                   std::filesystem::perms::group_read |
                                   std::filesystem::perms::group_exec |
                                   std::filesystem::perms::others_read |
                                   std::filesystem::perms::others_exec);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create desktop file: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxExporter::CreateAppRunScript(const std::filesystem::path& appdir_path,
                                      const std::string& executable_name) {
    try {
        auto apprun_path = appdir_path / "AppRun";
        
        std::ofstream apprun_file(apprun_path);
        if (!apprun_file) {
            return false;
        }
        
        apprun_file << "#!/bin/bash\n";
        apprun_file << "HERE=\"$(dirname \"$(readlink -f \"${0}\")\")\" \n";
        apprun_file << "export LD_LIBRARY_PATH=\"${HERE}/usr/lib:${LD_LIBRARY_PATH}\"\n";
        apprun_file << "cd \"${HERE}\"\n";
        apprun_file << "exec \"${HERE}/usr/bin/" << executable_name << "\" \"$@\"\n";
        
        apprun_file.close();
        
        // Make executable
        std::filesystem::permissions(apprun_path,
                                   std::filesystem::perms::owner_all |
                                   std::filesystem::perms::group_read |
                                   std::filesystem::perms::group_exec |
                                   std::filesystem::perms::others_read |
                                   std::filesystem::perms::others_exec);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create AppRun script: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxExporter::CopyApplicationIcon(const std::filesystem::path& appdir_path,
                                       const std::filesystem::path& icon_path,
                                       const std::string& app_name) {
    try {
        if (!std::filesystem::exists(icon_path)) {
            return true; // Not an error if icon doesn't exist
        }
        
        auto icon_dest = appdir_path / "usr" / "share" / "icons" / "hicolor" / "256x256" / "apps" / (app_name + ".png");
        std::filesystem::copy_file(icon_path, icon_dest,
                                  std::filesystem::copy_options::overwrite_existing);
        
        // Also copy to root of AppDir
        auto root_icon = appdir_path / (app_name + ".png");
        std::filesystem::copy_file(icon_path, root_icon,
                                  std::filesystem::copy_options::overwrite_existing);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to copy application icon: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxExporter::BuildAppImage(const std::filesystem::path& appdir_path,
                                 const std::filesystem::path& output_path) {
    try {
        auto appimagetool_path = GetAppImageToolPath();
        if (appimagetool_path.empty()) {
            return false;
        }
        
        // Build command
        std::string command = appimagetool_path.string() + " \"" + appdir_path.string() + "\" \"" + output_path.string() + "\"";
        
        int result = std::system(command.c_str());
        return result == 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to build AppImage: " << e.what() << std::endl;
        return false;
    }
}

bool LinuxExporter::IsAppImageToolAvailable() const {
    return !GetAppImageToolPath().empty();
}

std::filesystem::path LinuxExporter::GetAppImageToolPath() const {
    // Check common locations for appimagetool
    std::vector<std::string> search_paths = {
        "/usr/bin/appimagetool",
        "/usr/local/bin/appimagetool",
        "./appimagetool"
    };
    
    for (const auto& path : search_paths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    
    // Try to find in PATH
    if (std::system("which appimagetool > /dev/null 2>&1") == 0) {
        return "appimagetool";
    }
    
    return {};
}

std::vector<std::filesystem::path> LinuxExporter::GetRequiredLibraries() const {
    std::vector<std::filesystem::path> libs;

    // This method is deprecated - use GetRequiredStaticLibraries() instead
    // Kept for backward compatibility

    return libs;
}

std::filesystem::path LinuxExporter::BuildLinuxRuntime(const std::filesystem::path& output_dir) const {
    // For cross-compilation, we would use a Linux toolchain here
    // For now, check if we have a pre-built Linux runtime

    std::vector<std::string> runtime_search_paths = {
        "build/linux/bin/lupine-runtime",
        "out/linux/lupine-runtime",
        "thirdparty/linux_x64-static/bin/lupine-runtime"
    };

    for (const auto& search_path : runtime_search_paths) {
        if (std::filesystem::exists(search_path)) {
            auto dest_path = output_dir / "lupine-runtime";
            std::filesystem::copy_file(search_path, dest_path,
                                      std::filesystem::copy_options::overwrite_existing);

            // Make executable
            std::filesystem::permissions(dest_path,
                                       std::filesystem::perms::owner_all |
                                       std::filesystem::perms::group_read |
                                       std::filesystem::perms::group_exec |
                                       std::filesystem::perms::others_read |
                                       std::filesystem::perms::others_exec);

            return dest_path;
        }
    }

    // If no pre-built runtime found, we would need to cross-compile here
    // For now, return empty path to indicate failure
    std::cerr << "Linux runtime executable not found. Please build for Linux first." << std::endl;
    return {};
}

std::vector<std::filesystem::path> LinuxExporter::GetRequiredStaticLibraries() const {
    std::vector<std::filesystem::path> static_libs;

    // Try to get libraries from embedded manager first
    auto& embedded_manager = GetEmbeddedLibraryManager();
    if (embedded_manager.HasLibrariesForPlatform("linux", "x64")) {
        // Extract embedded libraries to temporary directory
        auto temp_dir = std::filesystem::temp_directory_path() / "lupine_linux_libs";

        if (embedded_manager.ExtractLibraries("linux", "x64", temp_dir)) {
            // Get list of extracted libraries
            auto available_libs = embedded_manager.GetAvailableLibraries("linux", "x64");

            for (const auto& lib_name : available_libs) {
                auto lib_path = temp_dir / (lib_name + ".a");
                if (std::filesystem::exists(lib_path)) {
                    static_libs.push_back(lib_path);
                }
            }

            if (!static_libs.empty()) {
                std::cout << "Using " << static_libs.size() << " embedded Linux libraries" << std::endl;
                return static_libs;
            }
        }
    }

    // Fallback: Use libraries from thirdparty directory
    std::filesystem::path linux_lib_dir = "thirdparty/linux_x64-static";

    // Core libraries that should be statically linked
    std::vector<std::string> lib_names = {
        // SDL2 libraries
        "sdl2/lib/libSDL2.a",
        "sdl2-image/lib/libSDL2_image.a",
        "sdl2-ttf/lib/libSDL2_ttf.a",
        "sdl2-mixer/lib/libSDL2_mixer.a",

        // Graphics libraries
        "assimp/lib/libassimp.a",
        "glad/lib/libglad.a",

        // Physics libraries
        "box2d/lib/libbox2d.a",
        "bullet3/lib/libBulletDynamics.a",
        "bullet3/lib/libBulletCollision.a",
        "bullet3/lib/libLinearMath.a",

        // Audio libraries
        "libogg/lib/libogg.a",
        "libvorbis/lib/libvorbis.a",
        "libvorbis/lib/libvorbisfile.a",
        "flac/lib/libFLAC.a",
        "opus/lib/libopus.a",

        // Scripting libraries
        "lua/lib/liblua.a",

        // Compression libraries
        "zlib/lib/libz.a",
        "bzip2/lib/libbz2.a",
        "lz4/lib/liblz4.a",
        "zstd/lib/libzstd.a",

        // Image libraries
        "libpng/lib/libpng.a",
        "libjpeg-turbo/lib/libjpeg.a",
        "freetype/lib/libfreetype.a",

        // Data format libraries
        "yaml-cpp/lib/libyaml-cpp.a",
        "pugixml/lib/libpugixml.a"
    };

    for (const auto& lib_name : lib_names) {
        auto lib_path = linux_lib_dir / lib_name;
        if (std::filesystem::exists(lib_path)) {
            static_libs.push_back(lib_path);
        }
    }

    if (!static_libs.empty()) {
        std::cout << "Using " << static_libs.size() << " libraries from thirdparty directory" << std::endl;
    }

    return static_libs;
}

LinuxPackageMetadata LinuxExporter::CreatePackageMetadata(const Project* project, const ExportConfig& config) const {
    LinuxPackageMetadata metadata;

    // Basic metadata
    metadata.name = GetExecutableName(config);
    metadata.version = "1.0.0"; // TODO: Get from project
    metadata.description = "Game created with Lupine Engine";
    metadata.maintainer = "Lupine Engine <support@lupineengine.com>";
    metadata.homepage = "https://lupineengine.com";
    metadata.license = "Proprietary"; // TODO: Get from project
    metadata.category = config.linux_config.app_category;
    metadata.dependencies = config.linux_config.dependencies;
    metadata.desktop_file_name = config.linux_config.desktop_file_name.empty() ? metadata.name : config.linux_config.desktop_file_name;
    metadata.icon_path = "icon.png"; // TODO: Get from project

    // Package-specific metadata
    metadata.deb.section = "games";
    metadata.deb.priority = "optional";
    metadata.deb.architecture = "amd64";

    metadata.rpm.group = "Amusements/Games";
    metadata.rpm.build_arch = "x86_64";
    metadata.rpm.summary = metadata.description;

    metadata.flatpak.app_id = "com.lupineengine." + metadata.name;
    metadata.flatpak.runtime = "org.freedesktop.Platform";
    metadata.flatpak.runtime_version = "23.08";
    metadata.flatpak.sdk = "org.freedesktop.Sdk";
    metadata.flatpak.finish_args = {
        "--share=ipc",
        "--socket=x11",
        "--socket=wayland",
        "--socket=pulseaudio",
        "--device=dri"
    };

    metadata.snap.grade = "stable";
    metadata.snap.confinement = "strict";
    metadata.snap.plugs = {"x11", "wayland", "opengl", "audio-playback", "desktop", "desktop-legacy"};

    return metadata;
}

std::string LinuxExporter::GetExecutableName(const ExportConfig& config) const {
    std::string name = config.executable_name;

    // Remove common Linux package extensions to get base name
    std::vector<std::string> extensions = {".AppImage", ".deb", ".rpm", ".snap", ".tar.gz"};

    for (const auto& ext : extensions) {
        if (name.length() >= ext.length() &&
            name.substr(name.length() - ext.length()) == ext) {
            name = name.substr(0, name.length() - ext.length());
            break;
        }
    }

    return name;
}

// Helper function to replace C++20 ends_with
bool ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

LinuxPackageFormat LinuxExporter::DeterminePackageFormat(const std::string& executable_name) const {
    // Helper lambda for C++17 compatibility (ends_with is C++20)
    auto ends_with = [](const std::string& str, const std::string& suffix) {
        if (suffix.length() > str.length()) return false;
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    };

    if (ends_with(executable_name, ".AppImage")) {
        return LinuxPackageFormat::AppImage;
    } else if (ends_with(executable_name, ".deb")) {
        return LinuxPackageFormat::Deb;
    } else if (ends_with(executable_name, ".rpm")) {
        return LinuxPackageFormat::Rpm;
    } else if (ends_with(executable_name, ".snap")) {
        return LinuxPackageFormat::Snap;
    } else if (ends_with(executable_name, ".tar.gz")) {
        return LinuxPackageFormat::TarGz;
    } else {
        // Default to building all supported formats
        return LinuxPackageFormat::All;
    }
}

EmbeddedLibraryManager& LinuxExporter::GetEmbeddedLibraryManager() const {
    static EmbeddedLibraryManager instance;
    return instance;
}

} // namespace Lupine
