 #include "lupine/serialization/ProjectSerializer.h"
#include "lupine/serialization/SerializationUtils.h"
#include "lupine/serialization/JsonUtils.h"
#include "lupine/input/ActionMap.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <filesystem>

namespace Lupine {

bool ProjectSerializer::SerializeToFile(const Project* project, const std::string& file_path) {
    if (!project) {
        std::cerr << "Cannot serialize null project" << std::endl;
        return false;
    }

    try {
        JsonNode projectJson;
        projectJson["project"]["name"] = project->GetName();
        projectJson["project"]["uuid"] = project->GetUUID().ToString();
        projectJson["project"]["version"] = project->GetVersion();
        projectJson["project"]["description"] = project->GetDescription();
        projectJson["project"]["main_scene"] = project->GetMainScene();
        projectJson["project"]["created"] = std::to_string(std::time(nullptr));
        projectJson["project"]["engine_version"] = "1.0.0";

        // Project settings
        JsonNode settings;

        // Window settings
        JsonNode window;
        window["width"] = project->GetSettingValue<int>("display/window_width", 1920);
        window["height"] = project->GetSettingValue<int>("display/window_height", 1080);
        window["title"] = project->GetSettingValue<std::string>("display/title", project->GetName());
        window["resizable"] = true;
        window["fullscreen"] = project->GetSettingValue<bool>("display/fullscreen", false);
        window["enable_high_dpi"] = project->GetSettingValue<bool>("display/enable_high_dpi", true);
        settings["window"] = window;

        // Render resolution settings
        JsonNode render_resolution;
        render_resolution["width"] = project->GetSettingValue<int>("display/render_width", 1920);
        render_resolution["height"] = project->GetSettingValue<int>("display/render_height", 1080);
        settings["render_resolution"] = render_resolution;

        // Rendering settings
        JsonNode rendering;
        rendering["vsync"] = project->GetSettingValue<bool>("display/vsync", true);
        rendering["msaa"] = project->GetSettingValue<int>("rendering/msaa", 4);
        rendering["target_fps"] = 60;
        settings["rendering"] = rendering;

        // Audio settings
        JsonNode audio;
        audio["master_volume"] = project->GetSettingValue<float>("audio/master_volume", 1.0f);
        audio["music_volume"] = project->GetSettingValue<float>("audio/music_volume", 0.8f);
        audio["sfx_volume"] = project->GetSettingValue<float>("audio/sfx_volume", 1.0f);
        settings["audio"] = audio;

        // Input settings
        JsonNode input;
        input["mouse_sensitivity"] = project->GetSettingValue<float>("input/mouse_sensitivity", 1.0f);
        input["invert_mouse"] = false;

        // Add action map if it exists in project settings
        auto action_map_setting = project->GetSetting("input/action_map");
        if (action_map_setting && std::holds_alternative<std::string>(*action_map_setting)) {
            std::string action_map_json = std::get<std::string>(*action_map_setting);
            try {
                auto action_map_data = nlohmann::json::parse(action_map_json);
                JsonNode action_map_node = JsonUtils::Parse(action_map_data.dump());
                input["action_map"] = action_map_node;
            } catch (const std::exception& e) {
                std::cerr << "Error parsing action map JSON: " << e.what() << std::endl;
            }
        }

        settings["input"] = input;

        projectJson["project"]["settings"] = settings;

        // Asset directories
        JsonNode assetDirs;
        assetDirs.value = std::vector<JsonNode>();
        assetDirs.Push(JsonNode("assets/"));
        assetDirs.Push(JsonNode("scenes/"));
        assetDirs.Push(JsonNode("scripts/"));
        assetDirs.Push(JsonNode("textures/"));
        assetDirs.Push(JsonNode("models/"));
        assetDirs.Push(JsonNode("audio/"));
        assetDirs.Push(JsonNode("fonts/"));
        projectJson["project"]["asset_directories"] = assetDirs;

        // Scene list
        JsonNode scenes;
        scenes.value = std::vector<JsonNode>();
        if (!project->GetMainScene().empty()) {
            JsonNode mainScene;
            mainScene["name"] = project->GetMainScene();
            mainScene["path"] = project->GetMainScene(); // Don't add extra "scenes/" or ".scene"
            mainScene["is_main"] = true;
            scenes.Push(mainScene);
        }
        projectJson["project"]["scenes"] = scenes;

        // Custom project settings
        const auto& customSettings = project->GetAllSettings();
        if (!customSettings.empty()) {
            JsonNode customSettingsJson;
            for (const auto& [key, value] : customSettings) {
                std::visit([&customSettingsJson, &key](const auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        customSettingsJson[key] = v;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        customSettingsJson[key] = v;
                    } else if constexpr (std::is_same_v<T, int>) {
                        customSettingsJson[key] = v;
                    } else if constexpr (std::is_same_v<T, float>) {
                        customSettingsJson[key] = v;
                    }
                }, value);
            }
            projectJson["project"]["custom_settings"] = customSettingsJson;
        }

        return JsonUtils::SaveToFile(projectJson, file_path, true);
    } catch (const std::exception& e) {
        std::cerr << "Error serializing project: " << e.what() << std::endl;
        return false;
    }
}

std::unique_ptr<Project> ProjectSerializer::DeserializeFromFile(const std::string& file_path) {
    try {
        JsonNode projectJson = JsonUtils::LoadFromFile(file_path);
        return DeserializeFromString(JsonUtils::Stringify(projectJson));
    } catch (const std::exception& e) {
        std::cerr << "Error deserializing project from file: " << e.what() << std::endl;
        return nullptr;
    }
}

std::string ProjectSerializer::SerializeToString(const Project* project) {
    if (!project) {
        return "{}";
    }

    try {
        JsonNode projectJson;
        projectJson["project"]["name"] = project->GetName();
        projectJson["project"]["uuid"] = project->GetUUID().ToString();
        projectJson["project"]["version"] = project->GetVersion();
        projectJson["project"]["description"] = project->GetDescription();
        projectJson["project"]["main_scene"] = project->GetMainScene();
        projectJson["project"]["created"] = std::to_string(std::time(nullptr));
        projectJson["project"]["engine_version"] = "1.0.0";

        // Project settings
        JsonNode settings;

        // Window settings
        JsonNode window;
        window["width"] = project->GetSettingValue<int>("display/window_width", 1920);
        window["height"] = project->GetSettingValue<int>("display/window_height", 1080);
        window["title"] = project->GetSettingValue<std::string>("display/title", project->GetName());
        window["resizable"] = true;
        window["fullscreen"] = project->GetSettingValue<bool>("display/fullscreen", false);
        window["enable_high_dpi"] = project->GetSettingValue<bool>("display/enable_high_dpi", true);
        settings["window"] = window;

        // Render resolution settings
        JsonNode render_resolution;
        render_resolution["width"] = project->GetSettingValue<int>("display/render_width", 1920);
        render_resolution["height"] = project->GetSettingValue<int>("display/render_height", 1080);
        settings["render_resolution"] = render_resolution;

        // Rendering settings
        JsonNode rendering;
        rendering["vsync"] = project->GetSettingValue<bool>("display/vsync", true);
        rendering["msaa"] = project->GetSettingValue<int>("rendering/msaa", 4);
        rendering["target_fps"] = 60;
        settings["rendering"] = rendering;

        // Audio settings
        JsonNode audio;
        audio["master_volume"] = project->GetSettingValue<float>("audio/master_volume", 1.0f);
        audio["music_volume"] = project->GetSettingValue<float>("audio/music_volume", 0.8f);
        audio["sfx_volume"] = project->GetSettingValue<float>("audio/sfx_volume", 1.0f);
        settings["audio"] = audio;

        // Input settings
        JsonNode input;
        input["mouse_sensitivity"] = project->GetSettingValue<float>("input/mouse_sensitivity", 1.0f);
        input["invert_mouse"] = false;

        // Add action map if it exists in project settings
        auto action_map_setting = project->GetSetting("input/action_map");
        if (action_map_setting && std::holds_alternative<std::string>(*action_map_setting)) {
            std::string action_map_json = std::get<std::string>(*action_map_setting);
            try {
                auto action_map_data = nlohmann::json::parse(action_map_json);
                JsonNode action_map_node = JsonUtils::Parse(action_map_data.dump());
                input["action_map"] = action_map_node;
            } catch (const std::exception& e) {
                std::cerr << "Error parsing action map JSON: " << e.what() << std::endl;
            }
        }

        settings["input"] = input;

        projectJson["project"]["settings"] = settings;

        // Asset directories
        JsonNode assetDirs;
        assetDirs.value = std::vector<JsonNode>();
        assetDirs.Push(JsonNode("assets/"));
        assetDirs.Push(JsonNode("scenes/"));
        assetDirs.Push(JsonNode("scripts/"));
        assetDirs.Push(JsonNode("textures/"));
        assetDirs.Push(JsonNode("models/"));
        assetDirs.Push(JsonNode("audio/"));
        assetDirs.Push(JsonNode("fonts/"));
        projectJson["project"]["asset_directories"] = assetDirs;

        // Scene list
        JsonNode scenes;
        scenes.value = std::vector<JsonNode>();
        if (!project->GetMainScene().empty()) {
            JsonNode mainScene;
            mainScene["name"] = project->GetMainScene();
            mainScene["path"] = project->GetMainScene(); // Don't add extra "scenes/" or ".scene"
            mainScene["is_main"] = true;
            scenes.Push(mainScene);
        }
        projectJson["project"]["scenes"] = scenes;

        // Build settings
        JsonNode build;
        JsonNode targetPlatforms;
        targetPlatforms.value = std::vector<JsonNode>();
        targetPlatforms.Push(JsonNode("windows"));
        targetPlatforms.Push(JsonNode("linux"));
        targetPlatforms.Push(JsonNode("macos"));
        build["target_platforms"] = targetPlatforms;
        build["optimization_level"] = "release";
        build["include_debug_info"] = false;
        build["compress_assets"] = true;
        projectJson["project"]["build"] = build;

        // Custom project settings
        const auto& customSettings = project->GetAllSettings();
        if (!customSettings.empty()) {
            JsonNode customSettingsJson;
            for (const auto& [key, value] : customSettings) {
                std::visit([&customSettingsJson, &key](const auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, bool>) {
                        customSettingsJson[key] = v;
                    } else if constexpr (std::is_same_v<T, std::string>) {
                        customSettingsJson[key] = v;
                    } else if constexpr (std::is_same_v<T, int>) {
                        customSettingsJson[key] = v;
                    } else if constexpr (std::is_same_v<T, float>) {
                        customSettingsJson[key] = v;
                    }
                }, value);
            }
            projectJson["project"]["custom_settings"] = customSettingsJson;
        }

        return JsonUtils::Stringify(projectJson, true);
    } catch (const std::exception& e) {
        std::cerr << "Error serializing project to string: " << e.what() << std::endl;
        return "{}";
    }
}

std::unique_ptr<Project> ProjectSerializer::DeserializeFromString(const std::string& json_string) {
    if (json_string.empty()) {
        std::cerr << "Error: Empty content provided for project deserialization" << std::endl;
        return nullptr;
    }

    try {
        JsonNode projectJson = JsonUtils::Parse(json_string);

        if (!projectJson.HasKey("project")) {
            std::cerr << "Invalid project JSON: missing 'project' key" << std::endl;
            return nullptr;
        }

        const JsonNode& projectData = projectJson["project"];

        auto project = std::make_unique<Project>();

        // Set basic properties
        if (projectData.HasKey("name")) {
            project->SetName(projectData["name"].AsString());
        }

        if (projectData.HasKey("version")) {
            project->SetVersion(projectData["version"].AsString());
        }

        if (projectData.HasKey("description")) {
            project->SetDescription(projectData["description"].AsString());
        }

        if (projectData.HasKey("main_scene")) {
            project->SetMainScene(projectData["main_scene"].AsString());
        }

        // Load action map from input settings
        if (projectData.HasKey("settings") && projectData["settings"].HasKey("input") &&
            projectData["settings"]["input"].HasKey("action_map")) {
            try {
                const JsonNode& actionMapNode = projectData["settings"]["input"]["action_map"];
                std::string actionMapJson = JsonUtils::Stringify(actionMapNode);
                project->SetSetting("input/action_map", actionMapJson);
            } catch (const std::exception& e) {
                std::cerr << "Error loading action map from project: " << e.what() << std::endl;
            }
        }

        // Load custom settings
        if (projectData.HasKey("custom_settings")) {
            const JsonNode& customSettings = projectData["custom_settings"];
            if (customSettings.IsObject()) {
                for (const auto& [key, valueNode] : customSettings.AsObject()) {
                    if (valueNode.IsBool()) {
                        project->SetSetting(key, valueNode.AsBool());
                    } else if (valueNode.IsString()) {
                        project->SetSetting(key, valueNode.AsString());
                    } else if (valueNode.IsInt()) {
                        project->SetSetting(key, static_cast<int>(valueNode.AsInt()));
                    } else if (valueNode.IsDouble()) {
                        project->SetSetting(key, static_cast<float>(valueNode.AsDouble()));
                    }
                }
            }
        }

        std::cout << "Successfully deserialized project: " << project->GetName() << std::endl;
        return project;

    } catch (const std::exception& e) {
        std::cerr << "Error deserializing project: " << e.what() << std::endl;
        return nullptr;
    }
}

} // namespace Lupine
