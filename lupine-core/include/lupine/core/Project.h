#pragma once

#include "lupine/core/UUID.h"
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <cstdint>

// Forward declarations
namespace Lupine {
    struct Locale;
}

namespace Lupine {

/**
 * @brief Project settings value type
 */
using ProjectSettingValue = std::variant<
    bool,
    int,
    float,
    std::string
>;

/**
 * @brief Project class for managing .lupine project files
 * 
 * A project contains project settings, main scene reference,
 * and other project-wide configuration.
 */
class Project {
public:
    /**
     * @brief Constructor
     * @param name Name of the project
     */
    explicit Project(const std::string& name = "New Project");
    
    /**
     * @brief Destructor
     */
    ~Project() = default;
    
    // Disable copy constructor and assignment operator
    Project(const Project&) = delete;
    Project& operator=(const Project&) = delete;
    
    // Enable move constructor and assignment operator
    Project(Project&&) = default;
    Project& operator=(Project&&) = default;
    
    /**
     * @brief Get the project's UUID
     * @return UUID of the project
     */
    const UUID& GetUUID() const { return m_uuid; }
    
    /**
     * @brief Get the project's name
     * @return Name of the project
     */
    const std::string& GetName() const { return m_name; }
    
    /**
     * @brief Set the project's name
     * @param name New name for the project
     */
    void SetName(const std::string& name) { m_name = name; }
    
    /**
     * @brief Get the project's file path
     * @return File path of the project (.lupine file)
     */
    const std::string& GetFilePath() const { return m_file_path; }
    
    /**
     * @brief Set the project's file path
     * @param file_path New file path
     */
    void SetFilePath(const std::string& file_path) { m_file_path = file_path; }
    
    /**
     * @brief Get the project's directory
     * @return Directory containing the project file
     */
    std::string GetProjectDirectory() const;
    
    /**
     * @brief Get the main scene path
     * @return Path to main scene file
     */
    const std::string& GetMainScene() const { return m_main_scene; }
    
    /**
     * @brief Set the main scene path
     * @param scene_path Path to main scene file
     */
    void SetMainScene(const std::string& scene_path) { m_main_scene = scene_path; }
    
    /**
     * @brief Get project version
     * @return Project version string
     */
    const std::string& GetVersion() const { return m_version; }
    
    /**
     * @brief Set project version
     * @param version New version string
     */
    void SetVersion(const std::string& version) { m_version = version; }
    
    /**
     * @brief Get project description
     * @return Project description
     */
    const std::string& GetDescription() const { return m_description; }
    
    /**
     * @brief Set project description
     * @param description New description
     */
    void SetDescription(const std::string& description) { m_description = description; }
    
    /**
     * @brief Set a project setting
     * @param key Setting key
     * @param value Setting value
     */
    void SetSetting(const std::string& key, const ProjectSettingValue& value);
    
    /**
     * @brief Get a project setting
     * @param key Setting key
     * @return Pointer to setting value, nullptr if not found
     */
    const ProjectSettingValue* GetSetting(const std::string& key) const;
    
    /**
     * @brief Get typed project setting value
     * @tparam T Type to get
     * @param key Setting key
     * @param default_value Default value if not found or wrong type
     * @return Setting value or default
     */
    template<typename T>
    T GetSettingValue(const std::string& key, const T& default_value = T{}) const;
    
    /**
     * @brief Get all project settings
     * @return Map of all settings
     */
    const std::unordered_map<std::string, ProjectSettingValue>& GetAllSettings() const { return m_settings; }
    
    /**
     * @brief Check if project has been modified since last save
     * @return True if modified
     */
    bool IsModified() const { return m_modified; }
    
    /**
     * @brief Mark project as modified
     */
    void MarkModified() { m_modified = true; }
    
    /**
     * @brief Mark project as saved (not modified)
     */
    void MarkSaved() { m_modified = false; }
    
    /**
     * @brief Load project from file
     * @param file_path Path to project file (.lupine)
     * @return True if loaded successfully
     */
    bool LoadFromFile(const std::string& file_path);

    /**
     * @brief Load project from memory buffer
     * @param data Project data buffer
     * @return True if loaded successfully
     */
    bool LoadFromMemory(const std::vector<uint8_t>& data);

    /**
     * @brief Check if project is loaded
     * @return True if project has been loaded from file or memory
     */
    bool IsLoaded() const { return m_loaded; }

    /**
     * @brief Save project to file
     * @param file_path Path to save to (optional, uses current path if empty)
     * @return True if saved successfully
     */
    bool SaveToFile(const std::string& file_path = "");
    
    /**
     * @brief Create a new project in directory
     * @param directory Directory to create project in
     * @param name Project name
     * @return True if created successfully
     */
    static bool CreateProject(const std::string& directory, const std::string& name);

private:
    UUID m_uuid;
    std::string m_name;
    std::string m_file_path;
    std::string m_main_scene;
    std::string m_version;
    std::string m_description;
    std::unordered_map<std::string, ProjectSettingValue> m_settings;
    bool m_modified;
    bool m_loaded;
    
    /**
     * @brief Initialize default settings
     */
    void InitializeDefaultSettings();

    /**
     * @brief Load localization data for this project
     */
    void LoadLocalizationData();

    /**
     * @brief Save localization data for this project
     */
    void SaveLocalizationData();
};

// Template implementation
template<typename T>
T Project::GetSettingValue(const std::string& key, const T& default_value) const {
    auto it = m_settings.find(key);
    if (it != m_settings.end()) {
        if (std::holds_alternative<T>(it->second)) {
            return std::get<T>(it->second);
        }

        // Handle type conversions between int and float
        if constexpr (std::is_same_v<T, int>) {
            if (std::holds_alternative<float>(it->second)) {
                return static_cast<int>(std::get<float>(it->second));
            }
        } else if constexpr (std::is_same_v<T, float>) {
            if (std::holds_alternative<int>(it->second)) {
                return static_cast<float>(std::get<int>(it->second));
            }
        }
    }
    return default_value;
}

} // namespace Lupine
