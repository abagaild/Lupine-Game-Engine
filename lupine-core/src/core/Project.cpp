#include "lupine/core/Project.h"
#include "lupine/serialization/ProjectSerializer.h"
#include "lupine/localization/LocalizationManager.h"
#include <filesystem>
#include <iostream>

namespace Lupine {

Project::Project(const std::string& name)
    : m_uuid(UUID::Generate())
    , m_name(name)
    , m_version("1.0.0")
    , m_description("")
    , m_modified(false)
    , m_loaded(false) {
    InitializeDefaultSettings();
}

std::string Project::GetProjectDirectory() const {
    if (m_file_path.empty()) {
        return "";
    }
    
    std::filesystem::path path(m_file_path);
    return path.parent_path().string();
}

void Project::SetSetting(const std::string& key, const ProjectSettingValue& value) {
    m_settings[key] = value;
    MarkModified();
}

const ProjectSettingValue* Project::GetSetting(const std::string& key) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        return &it->second;
    }
    return nullptr;
}

bool Project::LoadFromFile(const std::string& file_path) {
    auto loaded_project = ProjectSerializer::DeserializeFromFile(file_path);
    if (!loaded_project) {
        return false;
    }

    // Copy loaded project data to this project
    m_name = loaded_project->GetName();
    m_version = loaded_project->GetVersion();
    m_description = loaded_project->GetDescription();
    m_main_scene = loaded_project->GetMainScene();
    m_settings = loaded_project->GetAllSettings();

    m_file_path = file_path;

    // Load localization data if it exists
    LoadLocalizationData();

    m_loaded = true;
    MarkSaved();
    return true;
}

bool Project::LoadFromMemory(const std::vector<uint8_t>& data) {
    try {
        // Convert data to string
        std::string json_data(data.begin(), data.end());

        // Use ProjectSerializer to deserialize from string
        auto loaded_project = ProjectSerializer::DeserializeFromString(json_data);
        if (!loaded_project) {
            return false;
        }

        // Copy loaded project data to this project
        m_name = loaded_project->GetName();
        m_version = loaded_project->GetVersion();
        m_description = loaded_project->GetDescription();
        m_main_scene = loaded_project->GetMainScene();
        m_settings = loaded_project->GetAllSettings();

        m_loaded = true;
        MarkSaved();
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to load project from memory: " << e.what() << std::endl;
        return false;
    }
}

bool Project::SaveToFile(const std::string& file_path) {
    if (!file_path.empty()) {
        m_file_path = file_path;
    }

    if (m_file_path.empty()) {
        return false;
    }

    bool success = ProjectSerializer::SerializeToFile(this, m_file_path);
    if (success) {
        // Save localization data
        SaveLocalizationData();
        MarkSaved();
    }
    return success;
}

bool Project::CreateProject(const std::string& directory, const std::string& name) {
    try {
        // Create project directory if it doesn't exist
        std::filesystem::path project_path(directory);
        if (!std::filesystem::exists(project_path)) {
            std::filesystem::create_directories(project_path);
        }
        
        // Create project file
        std::filesystem::path project_file = project_path / (name + ".lupine");
        
        // Create new project instance
        Project project(name);
        project.SetFilePath(project_file.string());
        project.SetMainScene("scenes/main.scene");  // Set default main scene

        // Create default directories
        std::filesystem::create_directories(project_path / "scenes");
        std::filesystem::create_directories(project_path / "scripts");
        std::filesystem::create_directories(project_path / "assets");
        std::filesystem::create_directories(project_path / "assets" / "textures");
        std::filesystem::create_directories(project_path / "assets" / "models");
        std::filesystem::create_directories(project_path / "assets" / "audio");

        // Save project file
        return project.SaveToFile();
    } catch (const std::exception&) {
        return false;
    }
}

void Project::InitializeDefaultSettings() {
    // Display settings
    SetSetting("display/debug_window_scale", 1.0f);
    SetSetting("display/window_width", 1920);
    SetSetting("display/window_height", 1080);
    SetSetting("display/fullscreen", false);
    SetSetting("display/vsync", true);
    SetSetting("display/title", m_name);
    SetSetting("display/scale_type", std::string("letterbox"));

    // High DPI and Resolution settings
    SetSetting("display/enable_high_dpi", true);
    SetSetting("display/render_width", 1920);
    SetSetting("display/render_height", 1080);
    SetSetting("display/debug_render_width", 1920);
    SetSetting("display/debug_render_height", 1080);

    // Rendering settings
    SetSetting("rendering/msaa", 4);
    SetSetting("rendering/anisotropic_filtering", 16);
    SetSetting("rendering/shadow_quality", 2); // 0=low, 1=medium, 2=high
    SetSetting("rendering/stretch_style", std::string("bilinear"));

    // Physics settings
    SetSetting("physics/gravity", -9.81f);
    SetSetting("physics/fixed_timestep", 1.0f / 60.0f);

    // Audio settings
    SetSetting("audio/master_volume", 1.0f);
    SetSetting("audio/music_volume", 0.8f);
    SetSetting("audio/sfx_volume", 1.0f);

    // Input settings
    SetSetting("input/mouse_sensitivity", 1.0f);

    // Editor settings
    SetSetting("editor/undo_depth", 25);

    // Debug settings
    SetSetting("debug/show_fps", false);
    SetSetting("debug/show_wireframe", false);
    SetSetting("debug/show_collision_shapes", false);

    // Localization settings
    SetSetting("localization/default_locale", std::string("en_US"));
    SetSetting("localization/auto_detect_locale", true);
    SetSetting("localization/fallback_to_default", true);
    SetSetting("localization/show_missing_keys", false);
}

void Project::LoadLocalizationData() {
    if (m_file_path.empty()) {
        return;
    }

    // Get project directory
    std::filesystem::path project_path(m_file_path);
    std::filesystem::path localization_file = project_path.parent_path() / "localization.json";

    if (std::filesystem::exists(localization_file)) {
        auto& locManager = LocalizationManager::Instance();
        if (locManager.LoadFromFile(localization_file.string())) {
            std::cout << "Loaded localization data for project: " << m_name << std::endl;

            // Apply localization settings from project
            std::string default_locale = GetSettingValue<std::string>("localization/default_locale", "en_US");

            // Parse locale identifier
            size_t underscore_pos = default_locale.find('_');
            if (underscore_pos != std::string::npos) {
                std::string lang = default_locale.substr(0, underscore_pos);
                std::string region = default_locale.substr(underscore_pos + 1);
                Locale locale(lang, region);
                locManager.SetDefaultLocale(locale);
                locManager.SetCurrentLocale(locale);
            } else {
                Locale locale(default_locale);
                locManager.SetDefaultLocale(locale);
                locManager.SetCurrentLocale(locale);
            }
        }
    } else {
        // Initialize with default localization if no file exists
        auto& locManager = LocalizationManager::Instance();
        locManager.Initialize();
        std::cout << "Initialized default localization for project: " << m_name << std::endl;
    }
}

void Project::SaveLocalizationData() {
    if (m_file_path.empty()) {
        return;
    }

    // Get project directory
    std::filesystem::path project_path(m_file_path);
    std::filesystem::path localization_file = project_path.parent_path() / "localization.json";

    auto& locManager = LocalizationManager::Instance();
    if (locManager.SaveToFile(localization_file.string())) {
        std::cout << "Saved localization data for project: " << m_name << std::endl;

        // Update project settings with current localization settings
        SetSetting("localization/default_locale", locManager.GetDefaultLocale().GetIdentifier());
    }
}

} // namespace Lupine
